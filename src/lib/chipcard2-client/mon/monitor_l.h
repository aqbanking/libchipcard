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


#include <chipcard2-client/mon/monitor.h>
#include <chipcard2-client/client/notifications.h>
#include <chipcard2-client/client/client.h>


int LCM_Monitor_HandleNotification(LCM_MONITOR *mm,
                                   const LC_NOTIFICATION *n);



#endif

