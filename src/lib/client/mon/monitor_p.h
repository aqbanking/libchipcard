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


#ifndef LC_MON_MONITOR_P_H
#define LC_MON_MONITOR_P_H


#include "monitor_l.h"


struct LCM_MONITOR {
  LCM_SERVER_LIST *servers;
  time_t lastChangeTime;
};


void LCM_Monitor__LogToBuffer(GWEN_BUFFER *buf, const char *s);


int LCM_Monitor_HandleDriverNotification(LCM_MONITOR *mm,
                                         LCM_SERVER *ms,
                                         const LC_NOTIFICATION *n);
int LCM_Monitor_HandleReaderNotification(LCM_MONITOR *mm,
                                         LCM_SERVER *ms,
                                         const LC_NOTIFICATION *n);



#endif

