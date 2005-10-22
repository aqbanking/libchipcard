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



#ifndef CHIPCARD_SERVER2_CM_CARDMGR_L_H
#define CHIPCARD_SERVER2_CM_CARDMGR_L_H

typedef struct LCCM_CARDMANAGER LCCM_CARDMANAGER;

#include "cm_card_l.h"
#include "server_l.h"

#include <time.h>



LCCM_CARDMANAGER *LCCM_CardManager_new(LCS_SERVER *server);
void LCCM_CardManager_free(LCCM_CARDMANAGER *cm);

int LCCM_CardManager_Init(LCCM_CARDMANAGER *cm, GWEN_DB_NODE *dbConfig);
int LCCM_CardManager_Fini(LCCM_CARDMANAGER *cm, GWEN_DB_NODE *dbConfig);

void LCCM_CardManager_NewCard(LCCM_CARDMANAGER *cm, LCCO_CARD *card);

void LCCM_CardManager_ReaderDown(LCCM_CARDMANAGER *cm, GWEN_TYPE_UINT32 rid);
void LCCM_CardManager_ClientDown(LCCM_CARDMANAGER *cm, GWEN_TYPE_UINT32 clid);


void LCCM_CardManager_CardRemoved(LCCM_CARDMANAGER *cm,
                                  GWEN_TYPE_UINT32 rid,
                                  int slotNum,
                                  GWEN_TYPE_UINT32 cardNum);

LCCO_CARD *LCCM_CardManager_FindCard(LCCM_CARDMANAGER *cm,
                                     GWEN_TYPE_UINT32 cid);

LCCO_CARD *LCCM_CardManager_GetFirstCard(LCCM_CARDMANAGER *cm);
LCCO_CARD *LCCM_CardManager_GetNextCard(LCCM_CARDMANAGER *cm,
                                        LCCO_CARD *card);


int LCCM_CardManager_RequestLockCard(LCCM_CARDMANAGER *cm,
                                     LCCO_CARD *card,
                                     GWEN_TYPE_UINT32 clid,
                                     int duration,
                                     int maxLocks);

int LCCM_CardManager_CheckLockCardRequest(LCCM_CARDMANAGER *cm,
                                          LCCO_CARD *card,
                                          GWEN_TYPE_UINT32 clid);

int LCCM_CardManager_UnlockCard(LCCM_CARDMANAGER *cm,
                                LCCO_CARD *card,
                                GWEN_TYPE_UINT32 clid);

int LCCM_CardManager_SetCardAdTime(LCCM_CARDMANAGER *cm,
                                   LCCO_CARD *card,
                                   time_t t);

int LCCM_CardManager_CheckAccess(LCCM_CARDMANAGER *cm,
                                 LCCO_CARD *card,
                                 GWEN_TYPE_UINT32 clid);

/**
 * @return 1 if something could be done, 0 otherwise
 */
int LCCM_CardManager_Work(LCCM_CARDMANAGER *cm);


void LCCM_CardManager_DumpState(const LCCM_CARDMANAGER *cm);


#endif /* CHIPCARD_SERVER2_CM_CARDMGR_L_H */

