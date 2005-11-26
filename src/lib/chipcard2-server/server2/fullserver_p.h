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



#ifndef CHIPCARD_SERVER2_FULLSERVER_P_H
#define CHIPCARD_SERVER2_FULLSERVER_P_H


#include "fullserver_l.h"


typedef struct LCS_FULLSERVER LCS_FULLSERVER;
struct LCS_FULLSERVER {
  LCCL_CLIENTMANAGER *clientManager;
  LCCM_CARDMANAGER *cardManager;
  LCCMD_COMMANDMANAGER *commandManager;
  LCSV_SERVICEMANAGER *serviceManager;

  LCS_SERVER_DRIVER_CHG_FN driverChgFn;
  LCS_SERVER_READER_CHG_FN readerChgFn;
  LCS_SERVER_NEWCARD_FN newCardFn;
  LCS_SERVER_CARDREMOVED_FN cardRemovedFn;
  LCS_SERVER_HANDLEREQUEST_FN handleRequestFn;
  LCS_SERVER_CONNECTION_DOWN_FN connectionDownFn;
  LCS_SERVER_SERVICE_CHG_FN serviceChgFn;

};

void LCS_FullServer_FreeData(void *bp, void *p);


int LCS_FullServer_Init(LCS_SERVER *cs, GWEN_DB_NODE *db);
int LCS_FullServer_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db);




void LCS_FullServer_DriverChg(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 did,
                              const char *driverType,
                              const char *driverName,
                              const char *libraryFile,
                              LC_DRIVER_STATUS newSt,
                              const char *reason);


void LCS_FullServer_ReaderChg(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 did,
                              GWEN_TYPE_UINT32 rid,
                              const char *readerType,
                              const char *readerName,
                              const char *readerInfo,
                              LC_READER_STATUS newSt,
                              const char *reason);



void LCS_FullServer_NewCard(LCS_SERVER *cs, LCCO_CARD *card);



void LCS_FullServer_CardRemoved(LCS_SERVER *cs,
                                GWEN_TYPE_UINT32 rid,
                                int slotNum,
                                GWEN_TYPE_UINT32 cardNum);

void LCS_FullServer_ConnectionDown(LCS_SERVER *cs, GWEN_NETLAYER *conn);

int LCS_FullServer_HandleRequest(LCS_SERVER *cs,
                                 GWEN_TYPE_UINT32 rid,
                                 const char *name,
                                 GWEN_DB_NODE *dbReq);

void LCS_FullServer_ServiceChg(LCS_SERVER *cs,
                               GWEN_TYPE_UINT32 did,
                               const char *serviceType,
                               const char *serviceName,
                               LC_SERVICE_STATUS newSt,
                               const char *reason);


#endif /* CHIPCARD_SERVER2_FULLSERVER_P_H */

