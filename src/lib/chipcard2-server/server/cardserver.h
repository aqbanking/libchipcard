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


#ifndef CHIPCARD_SERVER_CARDSERVER_H
#define CHIPCARD_SERVER_CARDSERVER_H


typedef struct LC_CARDSERVER LC_CARDSERVER;

#include <gwenhywfar/ipc.h>
#include <chipcard2-server/server/driver.h>
#include <chipcard2-server/server/reader.h>
#include <chipcard2-server/server/request.h>


LC_CARDSERVER *LC_CardServer_new(const char *dataDir);
void LC_CardServer_free(LC_CARDSERVER *cs);

int LC_CardServer_ReadConfig(LC_CARDSERVER *cs, GWEN_DB_NODE *db);


/* TODO: Move to private header file */
int LC_CardServer_StartDriver(LC_CARDSERVER *cs, LC_DRIVER *d);
int LC_CardServer_CheckDriver(LC_CARDSERVER *cs, LC_DRIVER *d);

int LC_CardServer_StartReader(LC_CARDSERVER *cs, LC_READER *r);
int LC_CardServer_CheckReader(LC_CARDSERVER *cs, LC_READER *r);


int LC_CardServer_CheckReaderCommandStatus(LC_CARDSERVER *cs,
                                           LC_READER *r,
                                           const char *rspName);

int LC_CardServer_Work(LC_CARDSERVER *cs);


GWEN_TYPE_UINT32 LC_CardServer_GetIpcId(const LC_CARDSERVER *cs);
void LC_CardServer_SetIpcId(LC_CARDSERVER *cs, GWEN_TYPE_UINT32 id);



int LC_CardServer_GetUSBDevices(GWEN_DB_NODE *dbKnownDrivers,
                                GWEN_DB_NODE *dbReaders);


int LC_CardServer_GetClientCount(const LC_CARDSERVER *cs);

#endif /* CHIPCARD_SERVER_CARDSERVER_H */




