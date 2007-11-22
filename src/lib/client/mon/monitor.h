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

/** @addtogroup chipcardc_mon
 * @short Monitoring server activities
 */
/*@{*/

typedef struct LCM_MONITOR LCM_MONITOR;

#include <chipcard/chipcard.h>
#include <chipcard/client/mon/server.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <time.h>


CHIPCARD_API
LCM_SERVER_LIST *LCM_Monitor_GetServers(const LCM_MONITOR *mm);


CHIPCARD_API
time_t LCM_Monitor_GetLastChangeTime(const LCM_MONITOR *mm);


CHIPCARD_API
LCM_SERVER *LCM_Monitor_FindServer(const LCM_MONITOR *mm,
                                   uint32_t serverId);


CHIPCARD_API
LCM_DRIVER *LCM_Monitor_FindDriver(const LCM_MONITOR *mm,
                                   uint32_t serverId,
                                   const char *driverId);

CHIPCARD_API
LCM_READER *LCM_Monitor_FindReader(const LCM_MONITOR *mm,
                                   uint32_t serverId,
                                   const char *readerId);


/*@}*/ /* defgroup */

#endif

