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


#ifndef LC_MON_MONITOR_L_H
#define LC_MON_MONITOR_L_H


#include <chipcard/client/mon/monitor.h>
#include <chipcard/client/client.h>
#include <chipcard/client/notifications.h>


LCM_MONITOR *LCM_Monitor_new();
void LCM_Monitor_free(LCM_MONITOR *mm);

int LCM_Monitor_HandleNotification(LCM_MONITOR *mm,
                                   const LC_NOTIFICATION *n);



#endif

