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


#ifndef LC_MON_MONITOR_H
#define LC_MON_MONITOR_H



typedef struct LCM_MONITOR LCM_MONITOR;

#include <chipcard2/chipcard2.h>
#include <chipcard2-client/mon/server.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <time.h>


LCM_MONITOR *LCM_Monitor_new();
void LCM_Monitor_free(LCM_MONITOR *mm);

LCM_SERVER_LIST *LCM_Monitor_GetServers(const LCM_MONITOR *mm);


time_t LCM_Monitor_GetLastChangeTime(const LCM_MONITOR *mm);


LCM_SERVER *LCM_Monitor_FindServer(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId);


LCM_DRIVER *LCM_Monitor_FindDriver(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId,
                                   const char *driverId);

LCM_READER *LCM_Monitor_FindReader(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId,
                                   const char *readerId);


#endif

