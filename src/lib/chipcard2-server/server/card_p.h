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



#ifndef CHIPCARD_SERVER_CARD_P_H
#define CHIPCARD_SERVER_CARD_P_H

#include <gwenhywfar/idlist.h>
#include "card_l.h"


struct LC_CARD {
  GWEN_LIST_ELEMENT(LC_CARD);

  /* runtime variables */
  GWEN_TYPE_UINT32 cardId;
  LC_CARD_STATUS status;
  LC_READER *reader;
  unsigned int slot;
  LC_CARD_TYPE type;
  GWEN_TYPE_UINT32 readersCardId;
  GWEN_STRINGLIST *types;

  LC_CLIENT *client;
  time_t busySince;
  GWEN_BUFFER *atr;
  GWEN_IDLIST *waitingClients;
  int waitingClientCount;
  LC_CARDCONTEXT *cardContext;
};




#endif /* CHIPCARD_SERVER_CARD_P_H */





