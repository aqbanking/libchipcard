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


#ifndef CHIPCARD_SERVER_DRIVER_P_H
#define CHIPCARD_SERVER_DRIVER_P_H


#include <chipcard2-server/server/driver.h>


struct LC_DRIVER {
  GWEN_LIST_ELEMENT(LC_DRIVER);

  /* variables from config file */
  char *driverType;
  char *driverName;
  char *driverDataDir;
  char *logFile;
  char *customerId;
  char *libraryFile;
  int maxReaders;

  GWEN_DB_NODE *driverVars;

  /* runtime variables */
  GWEN_TYPE_UINT32 driverId;
  GWEN_PROCESS *process;
  LC_DRIVER_STATUS status;
  GWEN_TYPE_UINT32 driverFlags;
  time_t lastStatusChangeTime;
  time_t idleSince;
  time_t pingTime;
  time_t pongTime;
  GWEN_TYPE_UINT32 activeReadersCount;
  GWEN_TYPE_UINT32 ipcId;
  int pendingCommandCount;
  int firstNewPort;   /* derived from driverVars */
  int autoPortOffset; /* derived from driverVars */
  int autoPortMode;   /* derived from driverVars */
  int assignedReaders;
};






#endif /* CHIPCARD_SERVER_DRIVER_P_H */


