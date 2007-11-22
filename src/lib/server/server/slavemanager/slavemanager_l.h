/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: clientmanager_l.h 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER_SL_SLAVEMGR_L_H
#define CHIPCARD_SERVER_SL_SLAVEMGR_L_H

typedef struct LCSL_SLAVEMANAGER  LCSL_SLAVEMANAGER;

#include "server_l.h"



LCSL_SLAVEMANAGER *LCSL_SlaveManager_new(LCS_SERVER *server);
void LCSL_SlaveManager_free(LCSL_SLAVEMANAGER *slm);

int LCSL_SlaveManager_HandleRequest(LCSL_SLAVEMANAGER *slm,
                                    uint32_t rid,
                                    const char *name,
                                    GWEN_DB_NODE *dbReq);

int LCSL_SlaveManager_Work(LCSL_SLAVEMANAGER *slm);


int LCSL_SlaveManager_Init(LCSL_SLAVEMANAGER *slm, GWEN_DB_NODE *db);
int LCSL_SlaveManager_Fini(LCSL_SLAVEMANAGER *slm, GWEN_DB_NODE *db);

void LCSL_SlaveManager_DumpState(const LCSL_SLAVEMANAGER *slm);

void LCSL_SlaveManager_ReaderChg(LCSL_SLAVEMANAGER *slm,
                                 uint32_t did,
                                 LCCO_READER *r,
                                 LC_READER_STATUS newSt,
                                 const char *reason);

void LCSL_SlaveManager_NewReader(LCSL_SLAVEMANAGER *slm,
                                 LCCO_READER *r);

void LCSL_SlaveManager_NewCard(LCSL_SLAVEMANAGER *slm, LCCO_CARD *card);

void LCSL_SlaveManager_CardRemoved(LCSL_SLAVEMANAGER *slm, LCCO_CARD *card);


void LCSL_SlaveManager_ConnectionDown(LCSL_SLAVEMANAGER *slm, GWEN_IO_LAYER *nl);



#endif /* CHIPCARD_SERVER_SL_SLAVEMGR_L_H */



