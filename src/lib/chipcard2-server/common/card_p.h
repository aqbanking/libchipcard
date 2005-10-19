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



#ifndef CHIPCARD_SERVER_COMMON_CARD_P_H
#define CHIPCARD_SERVER_COMMON_CARD_P_H

#include "card.h"


struct LCCO_CARD {
  GWEN_LIST_ELEMENT(LCCO_CARD)
  GWEN_INHERIT_ELEMENT(LCCO_CARD)

  /* runtime variables */
  GWEN_TYPE_UINT32 cardId;
  LC_CARD_STATUS status;
  LC_CARD_TYPE cardType;
  GWEN_STRINGLIST *cardTypes;

  char *readerType;
  char *driverType;
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 readerFlags;
  GWEN_TYPE_UINT32 readersCardId;
  int slotNum;

  GWEN_BUFFER *atr;

  GWEN_TYPE_UINT32 lockId;

  int usage;
};




#endif /* CHIPCARD_SERVER_COMMON_CARD_P_H */





