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


#ifndef LC_MON_SERVER_H
#define LC_MON_SERVER_H



typedef struct LCM_SERVER LCM_SERVER;

#include <chipcard2/chipcard2.h>
#include <chipcard2-client/mon/reader.h>
#include <chipcard2-client/mon/driver.h>
#include <chipcard2-client/mon/service.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/buffer.h>




GWEN_LIST_FUNCTION_DEFS(LCM_SERVER, LCM_Server)
GWEN_LIST2_FUNCTION_DEFS(LCM_SERVER, LCM_Server)


LCM_SERVER *LCM_Server_new(GWEN_TYPE_UINT32 serverId);
void LCM_Server_free(LCM_SERVER *ms);

/**
 * This is the server id assigned by the client. This id is unique within
 * a LC_CLIENT. It is used to group the drivers, readers etc under their
 * server. Therefore every LCM object holds this id.
 */
GWEN_TYPE_UINT32 LCM_Server_GetServerId(const LCM_SERVER *ms);

/**
 * This is the id the server assigned to us (since we are a client to the
 * server).
 */
const char *LCM_Server_GetClientId(const LCM_SERVER *ms);
void LCM_Server_SetClientId(LCM_SERVER *ms, const char *s);

/**
 * Returns the list of drivers the monitor knows of the server.
 */
LCM_DRIVER_LIST *LCM_Server_GetDrivers(const LCM_SERVER *ms);

/**
 * Returns the list of readers the monitor knows of the server.
 */
LCM_READER_LIST *LCM_Server_GetReaders(const LCM_SERVER *ms);


/**
 * Returns the list of services the monitor knows of the server.
 */
LCM_SERVICE_LIST *LCM_Server_GetServices(const LCM_SERVER *ms);

#endif

