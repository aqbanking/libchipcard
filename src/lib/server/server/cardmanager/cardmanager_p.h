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



#ifndef CHIPCARD_SERVER2_CM_CARDMGR_P_H
#define CHIPCARD_SERVER2_CM_CARDMGR_P_H

#define LCCM_CARDMANAGER_DEF_UNUSED_TIMEOUT           10
#define LCCM_CARDMANAGER_DEF_MINIMUM_KEEP_TIME        10

#include "cardmanager_l.h"


struct LCCM_CARDMANAGER {
  LCS_SERVER *server;
  LCDM_DEVICEMANAGER *deviceManager;

  LCCO_CARD_LIST *cards;
  int unusedTimeout;
  int minimumKeepTime;
};


static
void LCCM_CardManager__RemoveCardsInSlots(LCCM_CARDMANAGER *cm,
                                          uint32_t rid,
                                          int slotNum);

static
void LCCM_CardManager__RemoveCardsInReader(LCCM_CARDMANAGER *cm,
                                           uint32_t rid);



#endif /* CHIPCARD_SERVER2_CM_CARDMGR_P_H */

