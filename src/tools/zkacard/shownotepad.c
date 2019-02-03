/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Sat Jun 23 2018
 copyright   : (C) 2018 by Martin Preuss, Stefan Bayer
 email       : martin@libchipcard.de, stefan.bayer@stefanbayer.net


 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "global.h"

#include <gwenhywfar/text.h>
#include <gwenhywfar/args.h>
#include <gwenhywfar/gui.h>
#include <chipcard/client.h>
#include <chipcard/cards/zkacard.h>
#include <chipcard/ct/ct_card.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define LC_CT_ZKA_NUM_CONTEXT 31

static uint32_t BeginEnterPin(GWEN_UNUSED GWEN_CRYPT_PINTYPE pt,
                              uint32_t gid)
{
  char buffer[512];

  buffer[0]=0;
  buffer[sizeof(buffer)-1]=0;

  snprintf(buffer, sizeof(buffer)-1, "%s",
           I18N("Please enter your PIN into the card reader."
                "<html>"
                "Please enter your PIN into the card reader."
                "</html>"));
  return GWEN_Gui_ShowBox(GWEN_GUI_SHOWBOX_FLAGS_BEEP,
                          I18N("Secure PIN Input"),
                          buffer, gid);
}

static int EndEnterPin(GWEN_UNUSED GWEN_CRYPT_PINTYPE pt,
                       GWEN_UNUSED int ok,
                       uint32_t id)
{

  GWEN_Gui_HideBox(id);

  return 0;
}

static
int EnterPinWithPinInfo(LC_CARD *hcard,
                        GWEN_CRYPT_PINTYPE pt,
                        const LC_PININFO *pi,
                        uint32_t guiid)
{
  LC_CLIENT_RESULT res;
  int maxErrors;
  int currentErrors;

  assert(hcard);
  assert(pi);



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
      return LC_Crypt_Token_ResultToError(res);
    }

    if ((currentErrors!=maxErrors)) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Bad pin entered at least once before, aborting");
      return GWEN_ERROR_ABORTED;
    }
  }
  else {
    DBG_INFO(LC_LOGDOMAIN,
             "Unable to read pin status for pin %02x (not supported)",
             LC_PinInfo_GetId(pi));
  }

  if ((pt!=GWEN_Crypt_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_READER_FLAGS_KEYPAD)) {
    uint32_t bid;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "Terminal has a keypad, will ask for pin.");
    /* tell the user about pin verification */
    bid=BeginEnterPin(pt, guiid);
    if (bid==0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
      return GWEN_ERROR_GENERIC;
    }

    res=LC_Card_IsoPerformVerification(hcard, 0, pi, &triesLeft);

    if (res!=LC_Client_ResultOk) {
      /* tell the user about end of pin verification */
      EndEnterPin(pt, 0, bid);
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_BAD_PIN_2_LEFT;
        default:   /* unknown error */
          return GWEN_ERROR_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_IO;
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
            return GWEN_ERROR_BAD_PIN_0_LEFT;
          case 1: /* one left */
            return GWEN_ERROR_BAD_PIN_1_LEFT;
          case 2: /* two left */
            return GWEN_ERROR_BAD_PIN_2_LEFT;
          default:   /* unknown count */
            return GWEN_ERROR_BAD_PIN;
          } // switch
        }

        return GWEN_ERROR_IO;
      }
    } /* if not ok */
    else {
      /* PIN ok */
      DBG_INFO(LC_LOGDOMAIN, "Pin ok");
      EndEnterPin(pt, 1, bid);
    }
  } /* if hasKeyPad */
  else {
#if 1
    DBG_ERROR(LC_LOGDOMAIN, "Card Reader without key pad!\nCould not verify pin!\n");
    return GWEN_ERROR_IO;
#else
    unsigned char pinBuffer[64];
    int mres;
    int pinMaxLen;
    unsigned int pinLength;
    unsigned int origPinLength;
    uint32_t pflags=0;
    GWEN_CRYPT_PINENCODING pe;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer, 0, sizeof(pinBuffer));

    pe=LC_PinInfo_GetEncoding(pi);
    if (pt==GWEN_Crypt_PinType_Manage)
      pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
    pinLength=0;
    pinMaxLen=LC_PinInfo_GetMaxLength(pi);
    if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer)-1)
      pinMaxLen=sizeof(pinBuffer)-1;
    mres=LC_Crypt_Token__GetPin(ct,
                                hcard,
                                LC_PinInfo_GetId(pi),
                                pt,
                                pe,
                                pflags,
                                pinBuffer,
                                LC_PinInfo_GetMinLength(pi),
                                pinMaxLen,
                                &pinLength,
                                guiid);
    if (mres!=0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer, 0, sizeof(pinBuffer));
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

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        /* set pin status */
        GWEN_Crypt_Token_SetPinStatus(ct,
                                      pt,
                                      pe,
                                      pflags,
                                      pinBuffer,
                                      origPinLength,
                                      0,
                                      guiid);

        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_BAD_PIN_2_LEFT;
        default:
          return GWEN_ERROR_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        /* set pin status */
        GWEN_Crypt_Token_SetPinStatus(ct,
                                      pt,
                                      pe,
                                      pflags,
                                      pinBuffer,
                                      origPinLength,
                                      0,
                                      guiid);
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_IO;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        if (triesLeft>=0) {
          /* set pin status */
          GWEN_Crypt_Token_SetPinStatus(ct,
                                        pt,
                                        pe,
                                        pflags,
                                        pinBuffer,
                                        origPinLength,
                                        0,
                                        guiid);
          switch (triesLeft) {
          case 0: /* no error left */
            return GWEN_ERROR_BAD_PIN_0_LEFT;
          case 1: /* one left */
            return GWEN_ERROR_BAD_PIN_1_LEFT;
          case 2: /* two left */
            return GWEN_ERROR_BAD_PIN_2_LEFT;
          default:   /* unknown count */
            return GWEN_ERROR_BAD_PIN;
          } // switch
        }
        DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
        return GWEN_ERROR_IO;
      }
    } // if not ok
    else {
      DBG_INFO(LC_LOGDOMAIN, "PIN ok");
      /* set pin status */
      GWEN_Crypt_Token_SetPinStatus(ct,
                                    pt,
                                    pe,
                                    pflags,
                                    pinBuffer,
                                    origPinLength,
                                    1,
                                    guiid);
    }
#endif
  } // if no keyPad

  return 0;
}


int showNotepad(GWEN_DB_NODE *dbArgs, int argc, char **argv)
{

  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbNotePad;
  GWEN_DB_NODE *dbRecord;
  int rv;
  int keyNumber;
  int j;
  const char *s;
  int i;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;
  const GWEN_CRYPT_TOKEN_CONTEXT *cctx;
  uint8_t cnt;
  LC_CARD *hcard=0;
  LC_CLIENT *lc;
  uint8_t v=0;
  uint8_t haveAccessPin=0;
  const LC_PININFO *pi;
  uint8_t guuid=0;

  const GWEN_ARGS args[]= {
    {
      GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
      GWEN_ArgsType_Int,            /* type */
      "help",                       /* name */
      0,                            /* minnum */
      0,                            /* maxnum */
      "h",                          /* short option */
      "help",                       /* long option */
      "Show this help screen",      /* short description */
      "Show this help screen"       /* long description */
    }
  };

  db=GWEN_DB_GetGroup(dbArgs, GWEN_DB_FLAGS_DEFAULT, "local");
  rv=GWEN_Args_Check(argc, argv, 1,
                     0 /*GWEN_ARGS_MODE_ALLOW_FREEPARAM*/,
                     args,
                     db);

  if (rv==GWEN_ARGS_RESULT_ERROR) {
    fprintf(stderr, "ERROR: Could not parse arguments\n");
    return 1;
  }
  else if (rv==GWEN_ARGS_RESULT_HELP) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
    if (GWEN_Args_Usage(args, ubuf, GWEN_ArgsOutType_Txt)) {
      fprintf(stderr, "ERROR: Could not create help string\n");
      return 1;
    }
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }



  /* get public key info */

  lc=LC_Client_new("zkacard", ZKACARDTOOL_PROGRAM_VERSION);
  res=LC_Client_Init(lc);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "Init");
    return RETURNVALUE_SETUP;
  }

  res=LC_Client_Start(lc);
  if (res!=LC_Client_ResultOk) {
    showError(hcard, res, "StartWait");
    return RETURNVALUE_WORK;
  }

  for (i=0;; i++) {
    const GWEN_STRINGLIST *sl;
    GWEN_STRINGLISTENTRY *se;
    uint8_t found = 0;
    if (v>0) {
      fprintf(stderr, "Waiting for card...\n");
    }
    res=LC_Client_GetNextCard(lc, &hcard, 20);
    if (res!=LC_Client_ResultOk) {
      showError(hcard, res, "GetNextCard");
      return RETURNVALUE_WORK;
    }
    else {
      if (v>0) {
        fprintf(stderr, "Found a card.\n");
      }
      break;
    }
#if 0
    sl=LC_Card_GetCardTypes(hcard);
    se=GWEN_StringList_FirstEntry(sl);
    while (se) {
      const char *s=GWEN_StringListEntry_Data(se);
      if (strcasecmp(s, "zkacard")==0) {
        found = 1;
        break;
      }
      GWEN_StringListEntry_Next(se);
    }
    if (found)
      break;
#endif



    if (v>0) {
      fprintf(stderr, "Not a zka card, releasing.\n");
    }
    res=LC_Client_ReleaseCard(lc, hcard);
    if (res!=LC_Client_ResultOk) {
      showError(hcard, res, "ReleaseCard");
      return RETURNVALUE_WORK;
    }
    LC_Card_free(hcard);

    if (i>15) {
      fprintf(stderr, "ERROR: No card found.\n");
      return RETURNVALUE_WORK;
    }
  } /* for */
  rv=LC_ZkaCard_ExtendCard(hcard);
  /* open card */
  if (v>0)
    fprintf(stderr, "Opening card.\n");
  res=LC_Card_Open(hcard);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr,
            "ERROR: Error executing command CardOpen (%d).\n",
            res);
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card is a memory card as expected.\n");

  pi=LC_ZkaCard_GetPinInfo(hcard, 3);
  if (pi == NULL)
    pi=LC_Card_GetPinInfoByName(hcard, "ch_pin");

  while (!haveAccessPin) {
    int rv;

    /* enter pin */
    if (pi)
      rv=EnterPinWithPinInfo(hcard,
                             GWEN_Crypt_PinType_Access,
                             pi,
                             0);
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Error in PIN input");
      return rv;
    }
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in PIN input");
      return rv;
    }
    else
      haveAccessPin=1;
  } /* while !havepin */


  res=LC_Card_SelectMf(hcard);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting MF (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectDf(hcard, "DF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting DF_NOTEPAD (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectEf(hcard, "EF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting EF_NOTEPAD (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  dbNotePad=GWEN_DB_Group_new("notepad");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  for (i=1; i<31; i++) {
    /* read record */
    DBG_INFO(LC_LOGDOMAIN, "Reading entry %d", i);
    res=LC_Card_IsoReadRecord(hcard, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, i, mbuf);
    GWEN_Buffer_Rewind(mbuf);
    if (res!=LC_Client_ResultOk) {
      if (LC_Card_GetLastSW1(hcard)==0x6a &&
          LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_INFO(LC_LOGDOMAIN, "All records read (%d)", i-1);
        break;
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Error reading record %d of EF_NOTEPAD (%d)", i, res);
        if (i>1)
          break;
        GWEN_Buffer_free(mbuf);
        return GWEN_ERROR_IO;
      }
    }
    else {
      GWEN_DB_NODE *dbEntry;

      /* parse record */
      if (GWEN_Buffer_GetBytesLeft(mbuf)) {
        DBG_INFO(LC_LOGDOMAIN, "Parsing entry %d", i);
        dbEntry=GWEN_DB_Group_new("entry");
        GWEN_Buffer_Rewind(mbuf);
        res=LC_Card_ParseRecord(hcard, i, mbuf, dbEntry);
        if (res!=LC_Client_ResultOk) {
          DBG_ERROR(LC_LOGDOMAIN, "Error parsing record %d of EF_NOTEPAD (%d)", i, res);
          if (i>1)
            break;
          GWEN_DB_Group_free(dbEntry);
          GWEN_Buffer_free(mbuf);
          return GWEN_ERROR_IO;
        }

        /* add new entry */
        DBG_INFO(LC_LOGDOMAIN, "Adding entry %d", i);
        GWEN_DB_AddGroup(dbNotePad, dbEntry);
      }
      else {
        DBG_INFO(LC_LOGDOMAIN, "Entry %d is empty", i);
      }
    }
    GWEN_Buffer_Reset(mbuf);
  }
  GWEN_Buffer_free(mbuf);
  cnt=0;
  dbRecord=GWEN_DB_GetFirstGroup(dbNotePad);
  printf("Notepad\n\n");

  while (dbRecord && cnt<LC_CT_ZKA_NUM_CONTEXT) {
    GWEN_DB_NODE *dbCtx;

    dbCtx=GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry/hbciData");
    if (dbCtx) {
      printf("HBCI Context #%d\n\n", cnt);
      GWEN_DB_Dump(dbCtx, 2);
      printf("\n\n");
      cnt++;
    }
    else {
      printf("Other Application Data\n");
      GWEN_DB_Dump(dbRecord, 2);
      printf("\n\n");
    }
    dbRecord=GWEN_DB_GetNextGroup(dbRecord);
  }


  GWEN_DB_Group_free(dbNotePad);
  /* close card */
  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(hcard);
  if (res!=LC_Client_ResultOk) {
    showError(hcard, res, "CardClose");
    return RETURNVALUE_WORK;
  }
  else if (v>1)
    fprintf(stderr, "Card closed.\n");

  if (v>0)
    fprintf(stderr, "Releasing card.\n");
  res=LC_Client_ReleaseCard(lc, hcard);
  if (res!=LC_Client_ResultOk) {
    showError(hcard, res, "ReleaseCard");
    return RETURNVALUE_WORK;
  }
  LC_Card_free(hcard);

  /* finished */
  if (v>1)
    fprintf(stderr, "Finished.\n");
  return 0;

  res=LC_Client_Fini(lc);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "Init");
  }

  LC_Client_free(lc);

  return 0;
}


