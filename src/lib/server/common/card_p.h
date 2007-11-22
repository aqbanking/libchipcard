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
  uint32_t cardId;
  LC_CARD_STATUS status;
  LC_CARD_TYPE cardType;
  GWEN_STRINGLIST *cardTypes;

  char *readerType;
  char *driverType;
  uint32_t readerId;
  uint32_t readerFlags;
  uint32_t readersCardId;
  uint32_t driversReaderId;
  int slotNum;
  uint32_t cardNum;

  GWEN_BUFFER *atr;

  uint32_t lockId;

  int usage;
};




#endif /* CHIPCARD_SERVER_COMMON_CARD_P_H */





