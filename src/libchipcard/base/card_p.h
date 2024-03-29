/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CARD_P_H
#define CHIPCARD_CLIENT_CARD_P_H


#include "card_l.h"
#include <gwenhywfar/inherit.h>


#define LC_PCSC_MAX_FEATURES 32



struct LC_CARD {
  GWEN_LIST_ELEMENT(LC_CARD)
  GWEN_INHERIT_ELEMENT(LC_CARD)
  LC_CLIENT *client;

  char *readerType;
  char *driverType;
  uint32_t readerFlags;
  char *cardType;
  GWEN_BUFFER *atr;
  GWEN_STRINGLIST *cardTypes;

  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;

  int lastSW1;
  int lastSW2;
  char *lastResult;
  char *lastText;

  GWEN_DB_NODE *dbCommandCache;
  GWEN_XMLNODE *cardNode;
  GWEN_XMLNODE *appNode;
  GWEN_XMLNODE *dfNode;
  GWEN_XMLNODE *efNode;

  /* SCard stuff */
  SCARDHANDLE scardHandle;
  uint32_t featureCode[LC_PCSC_MAX_FEATURES];
  DWORD protocol;
  char *readerName;

  /* ISO stuff */
  LC_CARD_GETPINSTATUS_FN getPinStatusFn;
  LC_CARD_GETINITIALPIN_FN getInitialPinFn;

  LC_CARD_ISOREADBINARY_FN readBinaryFn;
  LC_CARD_ISOWRITEBINARY_FN writeBinaryFn;
  LC_CARD_ISOUPDATEBINARY_FN updateBinaryFn;
  LC_CARD_ISOERASEBINARY_FN eraseBinaryFn;
  LC_CARD_ISOREADRECORD_FN readRecordFn;
  LC_CARD_ISOWRITERECORD_FN writeRecordFn;
  LC_CARD_ISOAPPENDRECORD_FN appendRecordFn;
  LC_CARD_ISOUPDATERECORD_FN updateRecordFn;
  LC_CARD_ISOVERIFYPIN_FN verifyPinFn;
  LC_CARD_ISOMODIFYPIN_FN modifyPinFn;

  LC_CARD_ISOPERFORMVERIFICATION_FN performVerificationFn;
  LC_CARD_ISOPERFORMMODIFICATION_FN performModificationFn;

  LC_CARD_ISOMANAGESE_FN manageSeFn;
  LC_CARD_ISOSIGN_FN signFn;
  LC_CARD_ISOVERIFY_FN verifyFn;
  LC_CARD_ISOENCIPHER_FN encipherFn;
  LC_CARD_ISODECIPHER_FN decipherFn;
  LC_CARD_ISOINTERNALAUTH_FN internalAuthFn;

  int usage;
};




#endif /* CHIPCARD_CLIENT_CARD_P_H */
