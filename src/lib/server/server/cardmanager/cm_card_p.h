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



#ifndef CHIPCARD_SERVER2_CM_CARD_P_H
#define CHIPCARD_SERVER2_CM_CARD_P_H

#include "cm_card_l.h"
#include "lockrequest_l.h"


typedef struct LCCM_CARD LCCM_CARD;
struct LCCM_CARD {
  LCS_LOCKREQUEST_LIST *requestList;
  LCS_LOCKREQUEST *currentRequest;
  LC_CARD_STATUS lastStatus;
  time_t unusedSince;
  int readerIsInUse;
  time_t lastAdTime;
};


void GWENHYWFAR_CB LCCM_Card_FreeData(void *bp, void *p);

LCS_LOCKREQUEST *LCCM_Card_FindRequestByClientId(LCCO_CARD *cd,
                                                 uint32_t clid);

int LCCM_Card_CountClientRequests(const LCCO_CARD *cd, uint32_t clid);


#endif /* CHIPCARD_SERVER2_CM_CARD_H */

