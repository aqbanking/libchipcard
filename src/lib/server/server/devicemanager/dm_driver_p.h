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


#ifndef CHIPCARD_SERVER_DM_DRIVER_P_H
#define CHIPCARD_SERVER_DM_DRIVER_P_H


#include "dm_driver_l.h"


struct LCDM_DRIVER {
  GWEN_LIST_ELEMENT(LCDM_DRIVER);

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
  uint32_t driverId;
  GWEN_PROCESS *process;
  LC_DRIVER_STATUS status;
  uint32_t driverFlags;
  time_t lastStatusChangeTime;
  time_t idleSince;
  time_t pingTime;
  time_t pongTime;

  time_t timeout;

  uint32_t ipcId;

  /* total number of readers assigned to this driver (including inactive
   * ones) */
  int assignedReaders;
  /* number of active readers only */
  uint32_t activeReadersCount;
};






#endif /* CHIPCARD_SERVER_DM_DRIVER_P_H */


