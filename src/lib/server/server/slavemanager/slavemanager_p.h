/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: clientmanager_p.h 267 2006-09-19 18:36:57Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER_SLAVEMGR_P_H
#define CHIPCARD_SERVER_SLAVEMGR_P_H

#define LCSL_SLAVEMANAGER_DEF_COMMAND_TIMEOUT          60

#define LCSL_SLAVE_STARTTIMEOUT                        10

#include "slavemanager_l.h"
#include "sl_reader_l.h"
#include <gwenhywfar/idlist.h>


struct LCSL_SLAVEMANAGER {
  LCS_SERVER *server;
  GWEN_IPCMANAGER *ipcManager;
  uint32_t ipcId;

  LCCO_READER_LIST2 *slaveReaders;
  int disconnected;

  int commandTimeout;
  uint32_t lastReaderId;
};


int LCSL_SlaveManager__PrepareConnection(LCSL_SLAVEMANAGER *slm,
                                         GWEN_DB_NODE *gr);

int LCSL_SlaveManager__Connect(LCSL_SLAVEMANAGER *slm);
int LCSL_SlaveManager__Disconnect(LCSL_SLAVEMANAGER *slm);
int LCSL_SlaveManager__Work(LCSL_SLAVEMANAGER *slm, int timeout);


int LCSL_SlaveManager_HandleStartReader(LCSL_SLAVEMANAGER *slm,
                                        uint32_t rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq);
int LCSL_SlaveManager_WorkStartReader(GWEN_IPC_REQUEST *req);


int LCSL_SlaveManager_HandleStopReader(LCSL_SLAVEMANAGER *slm,
                                       uint32_t rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq);


int LCSL_SlaveManager_HandleCardCommand(LCSL_SLAVEMANAGER *slm,
                                        uint32_t rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq);
int LCSL_SlaveManager_WorkCardCommand(GWEN_IPC_REQUEST *req);


int LCSL_SlaveManager_HandleCardReset(LCSL_SLAVEMANAGER *slm,
                                      uint32_t rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq);

int LCSL_SlaveManager_HandleSuspendCheck(LCSL_SLAVEMANAGER *slm,
                                         uint32_t rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq);
int LCSL_SlaveManager_HandleResumeCheck(LCSL_SLAVEMANAGER *slm,
                                        uint32_t rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq);


int LCSL_SlaveManager_SendReaderAdd(LCSL_SLAVEMANAGER *slm,
                                    LCCO_READER *r);
int LCSL_SlaveManager_SendReaderDel(LCSL_SLAVEMANAGER *slm,
                                    LCCO_READER *r);

int LCSL_SlaveManager_SendCardInserted(LCSL_SLAVEMANAGER *slm,
                                       LCCO_READER *r,
                                       LCCO_CARD *card);
int LCSL_SlaveManager_SendCardRemoved(LCSL_SLAVEMANAGER *slm,
                                      LCCO_READER *r,
                                      LCCO_CARD *card);




#endif /* CHIPCARD_SERVER_CL_SLAVEMGR_P_H */



