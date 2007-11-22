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


#ifndef LC_MON_DRIVER_P_H
#define LC_MON_DRIVER_P_H

#include "driver_l.h"


struct LCM_DRIVER {
  GWEN_LIST_ELEMENT(LCM_DRIVER)
  uint32_t serverId;
  char *driverId;
  char *driverType;
  char *driverName;
  char *libraryFile;
  char *status;
  GWEN_BUFFER *logBuffer;
  time_t lastChangeTime;
};








#endif

