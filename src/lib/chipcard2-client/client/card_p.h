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


#ifndef CHIPCARD_CLIENT_CARD_P_H
#define CHIPCARD_CLIENT_CARD_P_H


#include "card_l.h"
#include <gwenhywfar/inherit.h>


struct LC_CARD {
  GWEN_LIST_ELEMENT(LC_CARD)
  GWEN_INHERIT_ELEMENT(LC_CARD)
  LC_CLIENT *client;
  GWEN_TYPE_UINT32 cardId;
  char *readerType;
  char *driverType;
  GWEN_TYPE_UINT32 readerFlags;
  char *cardType;
  GWEN_TYPE_UINT32 serverId;
  GWEN_BUFFER *atr;

  LC_CARDCONTEXT *context;

  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;

  int lastSW1;
  int lastSW2;
  char *lastResult;
  char *lastText;

  GWEN_DB_NODE *driverVars;
};

LC_CLIENT_RESULT LC_Card__Open(LC_CARD *card);
LC_CLIENT_RESULT LC_Card__Close(LC_CARD *card);


#endif /* CHIPCARD_CLIENT_CARD_P_H */


