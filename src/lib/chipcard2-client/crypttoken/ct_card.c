/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ct_card.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


int LC_CryptToken_ResultToError(LC_CLIENT_RESULT res) {
  int rv;

  switch(res) {
  case LC_Client_ResultOk:
    rv=0;
    break;
  case LC_Client_ResultWait:
    rv=GWEN_ERROR_TIMEOUT;
    break;
  case LC_Client_ResultIpcError:
  case LC_Client_ResultCmdError:
  case LC_Client_ResultDataError:
    rv=GWEN_ERROR_CT_IO_ERROR;
    break;

  case LC_Client_ResultAborted:
    rv=GWEN_ERROR_USER_ABORTED;
    break;

  case LC_Client_ResultInvalid:
    rv=GWEN_ERROR_INVALID;
    break;

  case LC_Client_ResultNoData:
    rv=GWEN_ERROR_NO_DATA;
    break;

  case LC_Client_ResultCardRemoved:
    rv=GWEN_ERROR_CT_REMOVED;
    break;

  case LC_Client_ResultNotSupported:
    rv=GWEN_ERROR_CT_NOT_SUPPORTED;
    break;

  case LC_Client_ResultInternal:
  case LC_Client_ResultGeneric:
  default:
    rv=GWEN_ERROR_GENERIC;
    break;
  }

  return rv;
}


static
GWEN_CRYPTTOKEN_PINENCODING
LC_CryptToken__TransEncoding(LC_PININFO_ENCODING pe) {
  switch(pe) {
  case LC_PinInfo_EncodingNone:
    return GWEN_CryptToken_PinEncoding_None;
  case LC_PinInfo_EncodingBin:
    return GWEN_CryptToken_PinEncoding_Bin;
  case LC_PinInfo_EncodingBcd:
    return GWEN_CryptToken_PinEncoding_BCD;
  case LC_PinInfo_EncodingAscii:
    return GWEN_CryptToken_PinEncoding_ASCII;
  case LC_PinInfo_EncodingFpin2:
    return GWEN_CryptToken_PinEncoding_FPIN2;
  default:
    return GWEN_CryptToken_PinEncoding_Unknown;
  }
}



static
int LC_CryptToken__GetPin(GWEN_PLUGIN_MANAGER *pluginManager,
                          GWEN_CRYPTTOKEN *ct,
                          LC_CARD *hcard,
                          int pid,
                          GWEN_CRYPTTOKEN_PINTYPE pt,
                          GWEN_CRYPTTOKEN_PINENCODING pe,
                          GWEN_TYPE_UINT32 flags,
                          unsigned char *buffer,
                          unsigned int minLength,
                          unsigned int maxLength,
                          unsigned int *pinLength) {
  int rv;

  rv=GWEN_CryptManager_GetPin(pluginManager,
                              ct,
                              pt, pe, flags,
                              buffer,
                              minLength, maxLength,
                              pinLength);
  if (rv==GWEN_ERROR_CT_DEFAULT_PIN) {
    LC_CLIENT_RESULT res;

    res=LC_Card_GetInitialPin(hcard, pid, buffer, maxLength,
                              pinLength);
    if (res) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return GWEN_ERROR_CT_IO_ERROR;
    }

    if (pe!=GWEN_CryptToken_PinEncoding_ASCII) {
      rv=GWEN_CryptToken_TransformPin(GWEN_CryptToken_PinEncoding_ASCII,
                                      pe,
                                      buffer,
                                      maxLength,
                                      pinLength);
      if (rv) {
        DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
        return rv;
      }
    }
  }
  else if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  return 0;
}



static
int LC_CryptToken__ChangePin(GWEN_PLUGIN_MANAGER *pluginManager,
                             GWEN_CRYPTTOKEN *ct,
                             LC_CARD *hcard,
                             GWEN_CRYPTTOKEN_PINTYPE pt,
                             int initial) {
  LC_CLIENT_RESULT res;
  LC_PININFO *pi;
  int maxErrors;
  int currentErrors;

  assert(hcard);

  if (pt==GWEN_CryptToken_PinType_Manage)
    pi=LC_Card_GetPinInfoByName(hcard, "eg_pin");
  else
    pi=LC_Card_GetPinInfoByName(hcard, "ch_pin");
  assert(pi);

  if (LC_PinInfo_GetAllowChange(pi)==0) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Change of Pin is not allowed for this card");
    LC_PinInfo_free(pi);
    return GWEN_ERROR_INVALID;
  }

  res=LC_Card_GetPinStatus(hcard,
                           LC_PinInfo_GetId(pi),
                           &maxErrors,
                           &currentErrors);
  if (res!=LC_Client_ResultNotSupported) {
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Unable to read status of pin %x (%d)",
                LC_PinInfo_GetId(pi),
                res);
      LC_PinInfo_free(pi);
      return LC_CryptToken_ResultToError(res);
    }

    if ((currentErrors!=maxErrors) &&
        (GWEN_CryptToken_GetFlags(ct) & GWEN_CRYPTTOKEN_FLAGS_FORCE_PIN_ENTRY)){
      DBG_ERROR(LC_LOGDOMAIN,
                "Bad pin entered at least once before, aborting");
      LC_PinInfo_free(pi);
      return GWEN_ERROR_ABORTED;
    }
  }

  if (!initial && (pt!=GWEN_CryptToken_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_CARD_READERFLAGS_KEYPAD)) {
    int mres;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN,"Terminal has a keypad, will ask for pin.");
    /* tell the user about pin verification */
    mres=GWEN_CryptManager_BeginEnterPin(pluginManager,
                                         ct,
                                         pt);
    if (mres) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
      LC_PinInfo_free(pi);
      return mres;
    }

    res=LC_Card_IsoPerformModification(hcard, 0, pi, &triesLeft);

    if (res!=LC_Client_ResultOk) {
      /* tell the user about end of pin verification */
      GWEN_CryptManager_EndEnterPin(pluginManager,
                                    ct,
                                    pt, 0);
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));
      LC_PinInfo_free(pi);

      if (LC_Card_GetLastSW1(hcard)==0x63) {
	switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:   /* unknown error */
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        DBG_ERROR(LC_LOGDOMAIN, "Aborted by user");
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        return GWEN_ERROR_CT_IO_ERROR;
      }
    } /* if not ok */
    else {
      /* PIN ok */
      DBG_INFO(LC_LOGDOMAIN, "Pin ok");
      GWEN_CryptManager_EndEnterPin(pluginManager,
                                    ct,
                                    pt, 1);
    }
  } /* if hasKeyPad */
  else {
    unsigned char pinBuffer1[64];
    unsigned char pinBuffer2[64];
    unsigned int pinLength1;
    unsigned int pinLength2;
    int mres;
    int pinMaxLen;
    GWEN_TYPE_UINT32 pflags=0;
    GWEN_CRYPTTOKEN_PINENCODING pe;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer1, 0, sizeof(pinBuffer1));
    memset(pinBuffer2, 0, sizeof(pinBuffer2));

    pe=LC_CryptToken__TransEncoding(LC_PinInfo_GetEncoding(pi));
    if (pt==GWEN_CryptToken_PinType_Manage)
      pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_NUMERIC;
    pinLength1=0;
    pinMaxLen=LC_PinInfo_GetMaxLength(pi);
    if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer1)-1)
      pinMaxLen=sizeof(pinBuffer1)-1;
    if (initial) {
      LC_CLIENT_RESULT res;

      res=LC_Card_GetInitialPin(hcard,
                                LC_PinInfo_GetId(pi),
                                pinBuffer1, pinMaxLen,
                                &pinLength1);
      if (res) {
        DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
        mres=GWEN_ERROR_CT_IO_ERROR;
      }
      else
        mres=0;
    }
    else
      mres=LC_CryptToken__GetPin(pluginManager,
                                 ct,
                                 hcard,
                                 LC_PinInfo_GetId(pi),
                                 pt,
                                 pe,
                                 pflags,
                                 pinBuffer1,
                                 LC_PinInfo_GetMinLength(pi),
                                 pinMaxLen,
                                 &pinLength1);
    if (mres!=0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer1, 0, sizeof(pinBuffer1));
      memset(pinBuffer2, 0, sizeof(pinBuffer2));
      LC_PinInfo_free(pi);
      return mres;
    }

    if (pinLength1<pinMaxLen && LC_PinInfo_GetFiller(pi)) {
      int i;
      unsigned char c;

      c=(unsigned char)LC_PinInfo_GetFiller(pi);
      for (i=pinLength1; i<pinMaxLen; i++)
        pinBuffer1[i]=c;
      pinLength1=pinMaxLen;
    }

    /* get new pin */
    if (pt==GWEN_CryptToken_PinType_Manage)
      pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_NUMERIC;
    pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_CONFIRM;
    pinLength2=0;
    pinMaxLen=LC_PinInfo_GetMaxLength(pi);
    if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer2)-1)
      pinMaxLen=sizeof(pinBuffer2)-1;
    mres=LC_CryptToken__GetPin(pluginManager,
                               ct,
                               hcard,
                               LC_PinInfo_GetId(pi),
                               pt,
                               pe,
                               pflags,
                               pinBuffer2,
                               LC_PinInfo_GetMinLength(pi),
                               pinMaxLen,
                               &pinLength2);
    if (mres!=0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer1, 0, sizeof(pinBuffer1));
      memset(pinBuffer2, 0, sizeof(pinBuffer2));
      LC_PinInfo_free(pi);
      return mres;
    }

    if (pinLength2<pinMaxLen && LC_PinInfo_GetFiller(pi)) {
      int i;
      unsigned char c;

      c=(unsigned char)LC_PinInfo_GetFiller(pi);
      for (i=pinLength2; i<pinMaxLen; i++)
        pinBuffer2[i]=c;
      pinLength2=pinMaxLen;
    }

    DBG_INFO(LC_LOGDOMAIN, "Modifying the PIN");
    res=LC_Card_IsoModifyPin(hcard,
                             0, pi,
                             pinBuffer1,
                             pinLength1,
                             pinBuffer2,
                             pinLength2,
                             &triesLeft);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));
      LC_PinInfo_free(pi);

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        /* TODO: Set Pin status */
        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        /* TODO: Set Pin status */
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
        return GWEN_ERROR_CT_IO_ERROR;
      }
    } // if not ok
    else {
      DBG_INFO(LC_LOGDOMAIN, "PIN ok");
      /* TODO: Set Pin Status */
    }
  } // if no keyPad
  LC_PinInfo_free(pi);

  return 0;
}



static
int LC_CryptToken__EnterPin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt) {
  LC_CLIENT_RESULT res;
  LC_PININFO *pi;
  int maxErrors;
  int currentErrors;

  assert(hcard);

  if (pt==GWEN_CryptToken_PinType_Manage) {
    pi=LC_Card_GetPinInfoByName(hcard, "eg_pin");
  }
  else {
    pi=LC_Card_GetPinInfoByName(hcard, "ch_pin");
  }
  assert(pi);

#if 0
  if (pi) {
    GWEN_DB_NODE *dbDEBUG;

    dbDEBUG=GWEN_DB_Group_new("PinInfo");
    LC_PinInfo_toDb(pi, dbDEBUG);
    GWEN_DB_Dump(dbDEBUG, stderr, 2);
    GWEN_DB_Group_free(dbDEBUG);
  }
#endif

  res=LC_Card_GetPinStatus(hcard,
                           LC_PinInfo_GetId(pi),
                           &maxErrors,
                           &currentErrors);
  if (res!=LC_Client_ResultNotSupported) {
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Unable to read status of pin %x (%d)",
                LC_PinInfo_GetId(pi),
                res);
      LC_PinInfo_free(pi);
      return LC_CryptToken_ResultToError(res);
    }

    if ((currentErrors!=maxErrors) &&
        !(GWEN_CryptToken_GetModes(ct) &
          GWEN_CRYPTTOKEN_MODES_FORCE_PIN_ENTRY)
       ){
      DBG_ERROR(LC_LOGDOMAIN,
                "Bad pin entered at least once before, aborting");
      LC_PinInfo_free(pi);
      return GWEN_ERROR_ABORTED;
    }
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Unable to read pin status for pin %02x",
              LC_PinInfo_GetId(pi));
  }

  if ((pt!=GWEN_CryptToken_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_CARD_READERFLAGS_KEYPAD)) {
    int mres;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN,"Terminal has a keypad, will ask for pin.");
    /* tell the user about pin verification */
    mres=GWEN_CryptManager_BeginEnterPin(pluginManager,
                                         ct,
                                         pt);
    if (mres) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
      LC_PinInfo_free(pi);
      return mres;
    }

    res=LC_Card_IsoPerformVerification(hcard, 0, pi, &triesLeft);

    if (res!=LC_Client_ResultOk) {
      /* tell the user about end of pin verification */
      GWEN_CryptManager_EndEnterPin(pluginManager,
                                    ct,
                                    pt, 0);
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));
      LC_PinInfo_free(pi);

      if (LC_Card_GetLastSW1(hcard)==0x63) {
	switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:   /* unknown error */
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        DBG_ERROR(LC_LOGDOMAIN, "Aborted by user");
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        if (triesLeft>=0) {
          switch (triesLeft) {
          case 0: /* no error left */
            return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
          case 1: /* one left */
            return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
          case 2: /* two left */
            return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
          default:   /* unknown count */
            return GWEN_ERROR_CT_BAD_PIN;
          } // switch
        }

        return GWEN_ERROR_CT_IO_ERROR;
      }
    } /* if not ok */
    else {
      /* PIN ok */
      DBG_INFO(LC_LOGDOMAIN, "Pin ok");
      GWEN_CryptManager_EndEnterPin(pluginManager,
                                    ct,
                                    pt, 1);
    }
  } /* if hasKeyPad */
  else {
    unsigned char pinBuffer[64];
    int mres;
    int pinMaxLen;
    unsigned int pinLength;
    unsigned int origPinLength;
    GWEN_TYPE_UINT32 pflags=0;
    GWEN_CRYPTTOKEN_PINENCODING pe;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer, 0, sizeof(pinBuffer));

    pe=LC_CryptToken__TransEncoding(LC_PinInfo_GetEncoding(pi));
    if (pt==GWEN_CryptToken_PinType_Manage)
      pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_NUMERIC;
    pinLength=0;
    pinMaxLen=LC_PinInfo_GetMaxLength(pi);
    if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer)-1)
      pinMaxLen=sizeof(pinBuffer)-1;
    mres=LC_CryptToken__GetPin(pluginManager,
                               ct,
                               hcard,
                               LC_PinInfo_GetId(pi),
                               pt,
                               pe,
                               pflags,
                               pinBuffer,
                               LC_PinInfo_GetMinLength(pi),
                               pinMaxLen,
                               &pinLength);
    if (mres!=0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer, 0, sizeof(pinBuffer));
      LC_PinInfo_free(pi);
      return mres;
    }
    origPinLength=pinLength;

    if (pinLength<pinMaxLen && LC_PinInfo_GetFiller(pi)) {
      int i;
      unsigned char c;

      c=(unsigned char)LC_PinInfo_GetFiller(pi);
      for (i=pinLength; i<pinMaxLen; i++)
        pinBuffer[i]=c;
      pinLength=pinMaxLen;
    }

    DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");
    res=LC_Card_IsoVerifyPin(hcard,
                             0,
                             pi,
                             pinBuffer,
                             pinLength,
                             &triesLeft);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));

      LC_PinInfo_free(pi);

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        /* set pin status */
        GWEN_CryptManager_SetPinStatus(pluginManager,
                                       ct,
                                       pt,
                                       pe,
                                       pflags,
                                       pinBuffer,
                                       origPinLength,
                                       0);

        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        /* set pin status */
        GWEN_CryptManager_SetPinStatus(pluginManager,
                                       ct,
                                       pt,
                                       pe,
                                       pflags,
                                       pinBuffer,
                                       origPinLength,
                                       0);
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        if (triesLeft>=0) {
          /* set pin status */
          GWEN_CryptManager_SetPinStatus(pluginManager,
                                         ct,
                                         pt,
                                         pe,
                                         pflags,
                                         pinBuffer,
                                         origPinLength,
                                         0);
          switch (triesLeft) {
          case 0: /* no error left */
            return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
          case 1: /* one left */
            return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
          case 2: /* two left */
            return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
          default:   /* unknown count */
            return GWEN_ERROR_CT_BAD_PIN;
          } // switch
        }
        DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
        return GWEN_ERROR_CT_IO_ERROR;
      }
    } // if not ok
    else {
      DBG_INFO(LC_LOGDOMAIN, "PIN ok");
      /* set pin status */
      GWEN_CryptManager_SetPinStatus(pluginManager,
                                     ct,
                                     pt,
                                     pe,
                                     pflags,
                                     pinBuffer,
                                     origPinLength,
                                     1);
    }
  } // if no keyPad
  LC_PinInfo_free(pi);

  return 0;
}



int LC_CryptToken_VerifyPin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt) {
  int rv;

  /* enter pin */
  rv=LC_CryptToken__EnterPin(pluginManager, ct, hcard, pt);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "Error in pin input");
    return rv;
  }

  return 0;
}



int LC_CryptToken_ChangePin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt,
                            int initial) {
  int rv;

  if (pt!=GWEN_CryptToken_PinType_Access &&
      pt==GWEN_CryptToken_PinType_Manage) {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown pin type \"%s\"",
              GWEN_CryptToken_PinType_toString(pt));
    return GWEN_ERROR_INVALID;
  }

  /* enter pin */
  rv=LC_CryptToken__ChangePin(pluginManager, ct, hcard, pt, initial);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "Error in pin input");
    return rv;
  }

  return 0;
}




