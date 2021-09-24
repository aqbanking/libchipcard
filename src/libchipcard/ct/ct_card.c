/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
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
#include <gwenhywfar/ct_be.h>
#include <gwenhywfar/cryptdefs.h>
#include <gwenhywfar/gui.h>



static int _getPinInGivenEncoding(GWEN_CRYPT_TOKEN *ct,
                                  LC_CARD *hcard,
                                  int pid,
                                  GWEN_CRYPT_PINTYPE pinType,
                                  GWEN_CRYPT_PINENCODING pe,
                                  uint32_t flags,
                                  unsigned char *buffer,
                                  unsigned int minLength,
                                  unsigned int maxLength,
                                  unsigned int *pinLength,
                                  uint32_t guiid);
static int _checkPinStatus(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard, uint32_t pinId);
static int _verifyPinViaKeypad(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                              GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo,
                              uint32_t guiid);
static int _verifyPinViaKeyboard(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                                GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo,
                                uint32_t guiid);
static int _modifyPinViaKeypad(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                               GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo,
                               uint32_t guiid);
static int _modifyPinViaKeyboard(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
				 GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo, int initial,
				 uint32_t guiid);
static void _dumpPinInfo(const LC_PININFO *pinInfo);
static int _handlePinEntryError(LC_CARD *hcard, int triesLeft);





int LC_Crypt_Token_VerifyPin(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard, GWEN_CRYPT_PINTYPE pinType, uint32_t guiid)
{
  LC_PININFO *pinInfo;
  int rv;

  assert(hcard);

  if (pinType==GWEN_Crypt_PinType_Manage) {
    pinInfo=LC_Card_GetPinInfoByName(hcard, "eg_pin");
  }
  else {
    pinInfo=LC_Card_GetPinInfoByName(hcard, "ch_pin");
  }
  assert(pinInfo);

  rv=LC_Crypt_Token_VerifyPinWithPinInfo(ct, hcard, pinType, pinInfo, guiid);
  LC_PinInfo_free(pinInfo);

  return rv;
}



int LC_Crypt_Token_VerifyPinWithPinInfo(GWEN_CRYPT_TOKEN *ct,
                                        LC_CARD *hcard,
                                        GWEN_CRYPT_PINTYPE pinType,
                                        const LC_PININFO *pinInfo,
                                        uint32_t guiid)
{
  int res;

  assert(hcard);
  assert(pinInfo);

  if (0) /* debug */
    _dumpPinInfo(pinInfo);

  res=_checkPinStatus(ct, hcard, LC_PinInfo_GetId(pinInfo));
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }

  if ((pinType!=GWEN_Crypt_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_READER_FLAGS_KEYPAD) &&
      !(GWEN_Crypt_Token_GetModes(ct) & GWEN_CRYPT_TOKEN_MODE_FORCE_PIN_ENTRY)) {
    /* use reader's keypad */
    res=_verifyPinViaKeypad(ct, hcard, pinType, pinInfo, guiid);
    if (res<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }
  else {
    /* use PC's keyboard */
    res=_verifyPinViaKeyboard(ct, hcard, pinType, pinInfo, guiid);
    if (res<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }

  return 0;
}



int LC_Crypt_Token_ModifyPin(GWEN_CRYPT_TOKEN *ct,
			     LC_CARD *hcard,
			     GWEN_CRYPT_PINTYPE pinType,
			     int initial,
			     uint32_t guiid)
{
  int res;
  LC_PININFO *pinInfo;

  assert(hcard);

  if (pinType==GWEN_Crypt_PinType_Manage)
    pinInfo=LC_Card_GetPinInfoByName(hcard, "eg_pin");
  else if (pinType==GWEN_Crypt_PinType_Access)
    pinInfo=LC_Card_GetPinInfoByName(hcard, "ch_pin");
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown pin type \"%d\"", pinType);
    return GWEN_ERROR_INVALID;
  }
  assert(pinInfo);

  res=LC_Crypt_Token_ModifyPinWithPinInfo(ct, hcard, pinType, pinInfo, initial, guiid);
  LC_PinInfo_free(pinInfo);

  return res;
}



int LC_Crypt_Token_ModifyPinWithPinInfo(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
					GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo, int initial,
					uint32_t guiid)
{
  int res;
  assert(hcard);
  assert(pinInfo);

  if (LC_PinInfo_GetAllowChange(pinInfo)==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Change of Pin is not allowed for this card");
    return GWEN_ERROR_INVALID;
  }

  res=_checkPinStatus(ct, hcard, LC_PinInfo_GetId(pinInfo));
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }

  if (!initial && (pinType!=GWEN_Crypt_PinType_Manage) && (LC_Card_GetReaderFlags(hcard) & LC_READER_FLAGS_KEYPAD)) {
    /* use reader's keypad */
    res=_modifyPinViaKeypad(ct, hcard, pinType, pinInfo, guiid);
    if (res<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }
  else {
    /* use PC's keyboard */
    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    res=_modifyPinViaKeyboard(ct, hcard, pinType, pinInfo, initial, guiid);
    if (res<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }

  return 0;
}



int _getPinInGivenEncoding(GWEN_CRYPT_TOKEN *ct,
                           LC_CARD *hcard,
                           int pid,
                           GWEN_CRYPT_PINTYPE pinType,
                           GWEN_CRYPT_PINENCODING pinEncoding,
                           uint32_t flags,
                           unsigned char *buffer,
                           unsigned int minLength,
                           unsigned int maxLength,
                           unsigned int *pinLength,
                           uint32_t guiid)
{
  int rv;

  rv=GWEN_Crypt_Token_GetPin(ct, pinType, pinEncoding, flags, buffer, minLength, maxLength, pinLength, guiid);
  if (rv==GWEN_ERROR_DEFAULT_VALUE) {
    int res;

    res=LC_Card_GetInitialPin(hcard, pid, buffer, maxLength, pinLength);
    if (res) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return GWEN_ERROR_IO;
    }

    if (pinEncoding!=GWEN_Crypt_PinEncoding_Ascii) {
      rv=GWEN_Crypt_TransformPin(GWEN_Crypt_PinEncoding_Ascii, pinEncoding, buffer, maxLength, pinLength);
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



int _modifyPinViaKeypad(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
			GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo,
			uint32_t guiid)
{
  int res;
  uint32_t bid;
  int triesLeft=-1;

  /* tell the user about pin verification */
  bid=GWEN_Crypt_Token_BeginEnterPin(ct, pinType, guiid);
  if (bid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
    return GWEN_ERROR_USER_ABORTED;
  }

  res=LC_Card_IsoPerformModification(hcard, 0, pinInfo, &triesLeft);
  if (res<0) {
    /* tell the user about end of pin verification */
    GWEN_Crypt_Token_EndEnterPin(ct, pinType, 0, bid);
    return _handlePinEntryError(hcard, triesLeft);
  }
  else {
    /* PIN ok */
    DBG_INFO(LC_LOGDOMAIN, "Pin ok");
    GWEN_Crypt_Token_EndEnterPin(ct, pinType, 1, bid);
    return 0;
  }
}



int _modifyPinViaKeyboard(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                          GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo, int initial,
                          uint32_t guiid)
{
  int res;
  unsigned char pinBuffer1[64];
  unsigned char pinBuffer2[64];
  unsigned int pinLength1;
  unsigned int pinLength2;
  int mres;
  int pinMaxLen;
  uint32_t pflags=0;
  GWEN_CRYPT_PINENCODING pinEncoding;
  int triesLeft=-1;

  memset(pinBuffer1, 0, sizeof(pinBuffer1));
  memset(pinBuffer2, 0, sizeof(pinBuffer2));

  /* get old pin */
  pinEncoding=LC_PinInfo_GetEncoding(pinInfo);
  if (pinType==GWEN_Crypt_PinType_Manage)
    pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
  pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
  pinLength1=0;
  pinMaxLen=LC_PinInfo_GetMaxLength(pinInfo);
  if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer1)-1)
    pinMaxLen=sizeof(pinBuffer1)-1;

  if (initial) {
    res=LC_Card_GetInitialPin(hcard, LC_PinInfo_GetId(pinInfo), pinBuffer1, pinMaxLen, &pinLength1);
    if (res) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      mres=GWEN_ERROR_IO;
    }
    else
      mres=0;
  }
  else
    mres=_getPinInGivenEncoding(ct,
                                hcard,
                                LC_PinInfo_GetId(pinInfo),
                                pinType,
                                pinEncoding,
                                pflags,
                                pinBuffer1,
                                LC_PinInfo_GetMinLength(pinInfo),
                                pinMaxLen,
                                &pinLength1,
                                guiid);
  if (mres!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
    memset(pinBuffer1, 0, sizeof(pinBuffer1));
    memset(pinBuffer2, 0, sizeof(pinBuffer2));
    return mres;
  }

  if (pinLength1<pinMaxLen && LC_PinInfo_GetFiller(pinInfo)) {
    int i;
    unsigned char c;

    c=(unsigned char)LC_PinInfo_GetFiller(pinInfo);
    for (i=pinLength1; i<pinMaxLen; i++)
      pinBuffer1[i]=c;
    pinLength1=pinMaxLen;
  }

  /* get new pin */
  if (pinType==GWEN_Crypt_PinType_Manage)
    pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
  pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
  pflags|=GWEN_GUI_INPUT_FLAGS_CONFIRM;
  pinLength2=0;
  pinMaxLen=LC_PinInfo_GetMaxLength(pinInfo);
  if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer2)-1)
    pinMaxLen=sizeof(pinBuffer2)-1;
  mres=_getPinInGivenEncoding(ct,
                              hcard,
                              LC_PinInfo_GetId(pinInfo),
                              pinType,
                              pinEncoding,
                              pflags,
                              pinBuffer2,
                              LC_PinInfo_GetMinLength(pinInfo),
                              pinMaxLen,
                              &pinLength2,
                              guiid);
  if (mres!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
    memset(pinBuffer1, 0, sizeof(pinBuffer1));
    memset(pinBuffer2, 0, sizeof(pinBuffer2));
    return mres;
  }

  if (pinLength2<pinMaxLen && LC_PinInfo_GetFiller(pinInfo)) {
    int i;
    unsigned char c;

    c=(unsigned char)LC_PinInfo_GetFiller(pinInfo);
    for (i=pinLength2; i<pinMaxLen; i++)
      pinBuffer2[i]=c;
    pinLength2=pinMaxLen;
  }

  DBG_INFO(LC_LOGDOMAIN, "Modifying the PIN");
  res=LC_Card_IsoModifyPin(hcard, 0, pinInfo, pinBuffer1, pinLength1, pinBuffer2, pinLength2, &triesLeft);
  if (res<0) {
    return _handlePinEntryError(hcard, triesLeft);
  } // if not ok
  else {
    DBG_INFO(LC_LOGDOMAIN, "PIN ok");
  }

  return 0;
}



void _dumpPinInfo(const LC_PININFO *pinInfo)
{
  if (pinInfo) {
    GWEN_DB_NODE *dbDEBUG;

    dbDEBUG=GWEN_DB_Group_new("PinInfo");
    LC_PinInfo_toDb(pinInfo, dbDEBUG);
    GWEN_DB_Dump(dbDEBUG, 2);
    GWEN_DB_Group_free(dbDEBUG);
  }
}



int _checkPinStatus(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard, uint32_t pinId)
{
  int res;
  int maxErrors;
  int currentErrors;

  assert(hcard);

  res=LC_Card_GetPinStatus(hcard, pinId, &maxErrors, &currentErrors);
  if (res<0) {
    if (res==GWEN_ERROR_NOT_SUPPORTED) {
      DBG_INFO(LC_LOGDOMAIN, "Unable to read pin status for pin %02x (not supported), not checking.", pinId);
      return 0;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Unable to read status of pin %x (%d)", pinId, res);
      return res;
    }
  }

  if ((currentErrors!=maxErrors) && !(GWEN_Crypt_Token_GetModes(ct) & GWEN_CRYPT_TOKEN_MODE_FORCE_PIN_ENTRY)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad pin entered at least once before, aborting");
    return GWEN_ERROR_ABORTED;
  }

  return 0;
}



int _verifyPinViaKeypad(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                        GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo,
                        uint32_t guiid)
{
  int res;
  uint32_t bid;
  int triesLeft=-1;

  /* tell the user about pin verification */
  bid=GWEN_Crypt_Token_BeginEnterPin(ct, pinType, guiid);
  if (bid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
    return GWEN_ERROR_USER_ABORTED;
  }

  res=LC_Card_IsoPerformVerification(hcard, 0, pinInfo, &triesLeft);
  if (res<0) {
    /* tell the user about end of pin verification */
    GWEN_Crypt_Token_EndEnterPin(ct, pinType, 0, bid);
    return _handlePinEntryError(hcard, triesLeft);
  } /* if not ok */
  else {
    /* PIN ok */
    DBG_INFO(LC_LOGDOMAIN, "Pin ok");
    GWEN_Crypt_Token_EndEnterPin(ct, pinType, 1, bid);
  }

  return 0;
}



int _verifyPinViaKeyboard(GWEN_CRYPT_TOKEN *ct,
                          LC_CARD *hcard,
                          GWEN_CRYPT_PINTYPE pinType,
                          const LC_PININFO *pinInfo,
                          uint32_t guiid)
{
  int res;
  unsigned char pinBuffer[64];
  int mres;
  int pinMaxLen;
  unsigned int pinLength;
  unsigned int origPinLength;
  uint32_t pflags=0;
  GWEN_CRYPT_PINENCODING pinEncoding;
  int triesLeft=-1;

  memset(pinBuffer, 0, sizeof(pinBuffer));

  pinEncoding=LC_PinInfo_GetEncoding(pinInfo);
  if (pinType==GWEN_Crypt_PinType_Manage)
    pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
  pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
  pinLength=0;
  pinMaxLen=LC_PinInfo_GetMaxLength(pinInfo);
  if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer)-1)
    pinMaxLen=sizeof(pinBuffer)-1;
  mres=_getPinInGivenEncoding(ct,
                              hcard,
                              LC_PinInfo_GetId(pinInfo),
                              pinType,
                              pinEncoding,
                              pflags,
                              pinBuffer,
                              LC_PinInfo_GetMinLength(pinInfo),
                              pinMaxLen,
                              &pinLength,
                              guiid);
  if (mres!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
    memset(pinBuffer, 0, sizeof(pinBuffer));
    return mres;
  }
  origPinLength=pinLength;

  if (pinLength<pinMaxLen && LC_PinInfo_GetFiller(pinInfo)) {
    int i;
    unsigned char c;

    c=(unsigned char)LC_PinInfo_GetFiller(pinInfo);
    for (i=pinLength; i<pinMaxLen; i++)
      pinBuffer[i]=c;
    pinLength=pinMaxLen;
  }

  DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");
  res=LC_Card_IsoVerifyPin(hcard, 0, pinInfo, pinBuffer, pinLength, &triesLeft);
  if (res<0) {
    GWEN_Crypt_Token_SetPinStatus(ct, pinType, pinEncoding, pflags, pinBuffer, origPinLength, 0, guiid);
    return _handlePinEntryError(hcard, triesLeft);
  } // if not ok
  else {
    DBG_INFO(LC_LOGDOMAIN, "PIN ok");
    /* set pin status */
    GWEN_Crypt_Token_SetPinStatus(ct, pinType, pinEncoding, pflags, pinBuffer, origPinLength, 1, guiid);
  }

  return 0;
}



int _handlePinEntryError(LC_CARD *hcard, int triesLeft)
{
  DBG_ERROR(LC_LOGDOMAIN,
            "sw1=%02x sw2=%02x (%s)",
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
  else if (LC_Card_GetLastSW1(hcard)==0x69 && LC_Card_GetLastSW2(hcard)==0x83) {
    DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
    return GWEN_ERROR_IO;
  }
  else if (LC_Card_GetLastSW1(hcard)==0x64 && LC_Card_GetLastSW2(hcard)==0x01) {
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

  return 0;
}









