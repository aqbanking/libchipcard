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


#ifndef CHIPCARD_SERVER_SV_SERVICE_L_H
#define CHIPCARD_SERVER_SV_SERVICE_L_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/process.h>
#include <gwenhywfar/misc.h>

#include <time.h>


typedef struct LCSV_SERVICE LCSV_SERVICE;

GWEN_LIST_FUNCTION_DEFS(LCSV_SERVICE, LCSV_Service)

#include <chipcard3/chipcard3.h>


LCSV_SERVICE *LCSV_Service_new();
void LCSV_Service_free(LCSV_SERVICE *sv);

LCSV_SERVICE *LCSV_Service_fromDb(GWEN_DB_NODE *db);
void LCSV_Service_toDb(const LCSV_SERVICE *sv, GWEN_DB_NODE *db);

const char *LCSV_Service_GetServiceType(const LCSV_SERVICE *sv);
void LCSV_Service_SetServiceType(LCSV_SERVICE *sv, const char *s);

const char *LCSV_Service_GetServiceName(const LCSV_SERVICE *sv);
void LCSV_Service_SetServiceName(LCSV_SERVICE *sv, const char *s);

const char *LCSV_Service_GetLogFile(const LCSV_SERVICE *sv);
void LCSV_Service_SetLogFile(LCSV_SERVICE *sv, const char *s);

const char *LCSV_Service_GetDataDir(const LCSV_SERVICE *sv);
void LCSV_Service_SetDataDir(LCSV_SERVICE *sv, const char *s);

GWEN_TYPE_UINT32 LCSV_Service_GetServiceId(const LCSV_SERVICE *sv);

GWEN_TYPE_UINT32 LCSV_Service_GetFlags(const LCSV_SERVICE *sv);
void LCSV_Service_SetFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl);
void LCSV_Service_AddFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl);
void LCSV_Service_SubFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl);

GWEN_PROCESS *LCSV_Service_GetProcess(const LCSV_SERVICE *sv);
void LCSV_Service_SetProcess(LCSV_SERVICE *sv, GWEN_PROCESS *p);

LC_SERVICE_STATUS LCSV_Service_GetStatus(const LCSV_SERVICE *sv);
void LCSV_Service_SetStatus(LCSV_SERVICE *sv, LC_SERVICE_STATUS st);

GWEN_TYPE_UINT32 LCSV_Service_GetIpcId(const LCSV_SERVICE *sv);
void LCSV_Service_SetIpcId(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LCSV_Service_GetInterestedClients(const LCSV_SERVICE *sv);
void LCSV_Service_IncInterestedClients(LCSV_SERVICE *sv);
void LCSV_Service_DecInterestedClients(LCSV_SERVICE *sv);

GWEN_TYPE_UINT32 LCSV_Service_GetActiveClients(const LCSV_SERVICE *sv);
void LCSV_Service_IncActiveClients(LCSV_SERVICE *sv);
void LCSV_Service_DecActiveClients(LCSV_SERVICE *sv);

time_t LCSV_Service_GetLastStatusChangeTime(const LCSV_SERVICE *sv);
time_t LCSV_Service_GetIdleSince(const LCSV_SERVICE *sv);

GWEN_TYPE_UINT32 LCSV_Service_GetCurrentRequestId(const LCSV_SERVICE *sv);
void LCSV_Service_SetCurrentRequestId(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 rid);

void LCSV_Service_SetTimeout(LCSV_SERVICE *sv, int secs);
int LCSV_Service_CheckTimeout(const LCSV_SERVICE *sv);



#endif /* CHIPCARD_SERVER_SV_SERVICE_L_H */



