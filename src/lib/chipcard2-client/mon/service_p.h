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


#ifndef LC_MON_SERVICE_P_H
#define LC_MON_SERVICE_P_H


#include "service_l.h"

struct LCM_SERVICE {
  GWEN_LIST_ELEMENT(LCM_SERVICE)
  GWEN_TYPE_UINT32 serviceId;
  char *serviceName;
  GWEN_TYPE_UINT32 serverId;
  char *status;
  GWEN_BUFFER *logBuffer;
  time_t lastChangeTime;

};


#endif

