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


#ifndef CHIPCARD_SERVER_DM_DRIVER_L_H
#define CHIPCARD_SERVER_DM_DRIVER_L_H


typedef struct LCDM_DRIVER LCDM_DRIVER;


#include <gwenhywfar/process.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <time.h>

/** driver is a remote driver, not started by the server */
#define LCDM_DRIVER_FLAGS_RUNTIME_MASK  0xffff0000
#define LCDM_DRIVER_FLAGS_AUTO          0x00010000
#define LCDM_DRIVER_FLAGS_REMOTE        0x00020000
#define LCDM_DRIVER_FLAGS_CONFIG        0x00040000

#define LCDM_DRIVER_FLAGS_HAS_VERIFY_FN 0x00000001
#define LCDM_DRIVER_FLAGS_HAS_MODIFY_FN 0x00000002


GWEN_LIST_FUNCTION_DEFS(LCDM_DRIVER, LCDM_Driver);


#include <chipcard2/chipcard2.h>
#include "devicemanager_l.h"


GWEN_TYPE_UINT32 LCDM_Driver_Flag_fromDb(GWEN_DB_NODE *db, const char *name);
int LCDM_Driver_Flag_toDb(GWEN_DB_NODE *db,
                          const char *name,
                          GWEN_TYPE_UINT32 flags);


LCDM_DRIVER *LCDM_Driver_new();
LCDM_DRIVER *LCDM_Driver_fromDb(GWEN_DB_NODE *db);
void LCDM_Driver_free(LCDM_DRIVER *d);

void LCDM_Driver_toDb(const LCDM_DRIVER *d, GWEN_DB_NODE *db);

int LCDM_Driver_GetMaxReaders(const LCDM_DRIVER *d);
void LCDM_Driver_SetMaxReaders(LCDM_DRIVER *d, int maxReaders);

const char *LCDM_Driver_GetDriverType(const LCDM_DRIVER *d);
void LCDM_Driver_SetDriverType(LCDM_DRIVER *d, const char *s);

const char *LCDM_Driver_GetDriverName(const LCDM_DRIVER *d);
void LCDM_Driver_SetDriverName(LCDM_DRIVER *d, const char *s);

const char *LCDM_Driver_GetDriverDataDir(const LCDM_DRIVER *d);
void LCDM_Driver_SetDriverDataDir(LCDM_DRIVER *d, const char *s);

const char *LCDM_Driver_GetCustomerId(const LCDM_DRIVER *d);
void LCDM_Driver_SetCustomerId(LCDM_DRIVER *d, const char *s);

const char *LCDM_Driver_GetLibraryFile(const LCDM_DRIVER *d);
void LCDM_Driver_SetLibraryFile(LCDM_DRIVER *d, const char *s);

const char *LCDM_Driver_GetLogFile(const LCDM_DRIVER *d);
void LCDM_Driver_SetLogFile(LCDM_DRIVER *d, const char *s);

GWEN_TYPE_UINT32 LCDM_Driver_GetDriverId(const LCDM_DRIVER *d);

GWEN_PROCESS *LCDM_Driver_GetProcess(const LCDM_DRIVER *d);
void LCDM_Driver_SetProcess(LCDM_DRIVER *d, GWEN_PROCESS *p);

LC_DRIVER_STATUS LCDM_Driver_GetStatus(const LCDM_DRIVER *d);
void LCDM_Driver_SetStatus(LCDM_DRIVER *d, LC_DRIVER_STATUS st);

time_t LCDM_Driver_GetLastStatusChangeTime(const LCDM_DRIVER *d);
time_t LCDM_Driver_GetIdleSince(const LCDM_DRIVER *d);

GWEN_TYPE_UINT32 LCDM_Driver_GetActiveReadersCount(const LCDM_DRIVER *d);
void LCDM_Driver_ResetActiveReadersCount(LCDM_DRIVER *d);
void LCDM_Driver_IncActiveReadersCount(LCDM_DRIVER *d, int count);
void LCDM_Driver_DecActiveReadersCount(LCDM_DRIVER *d, int count);

GWEN_TYPE_UINT32 LCDM_Driver_GetAssignedReadersCount(const LCDM_DRIVER *d);
void LCDM_Driver_ResetAssignedReadersCount(LCDM_DRIVER *d);
void LCDM_Driver_IncAssignedReadersCount(LCDM_DRIVER *d);
void LCDM_Driver_DecAssignedReadersCount(LCDM_DRIVER *d);

GWEN_TYPE_UINT32 LCDM_Driver_GetDriverFlags(const LCDM_DRIVER *d);
void LCDM_Driver_SetDriverFlags(LCDM_DRIVER *d, GWEN_TYPE_UINT32 fl);
void LCDM_Driver_AddDriverFlags(LCDM_DRIVER *d, GWEN_TYPE_UINT32 fl);
void LCDM_Driver_SubDriverFlags(LCDM_DRIVER *d, GWEN_TYPE_UINT32 fl);

GWEN_TYPE_UINT32 LCDM_Driver_GetIpcId(const LCDM_DRIVER *d);
void LCDM_Driver_SetIpcId(LCDM_DRIVER *d, GWEN_TYPE_UINT32 id);

time_t LCDM_Driver_GetPingTime(const LCDM_DRIVER *d);
void LCDM_Driver_SetPingTime(LCDM_DRIVER *d, time_t t);
time_t LCDM_Driver_GetPongTime(const LCDM_DRIVER *d);
void LCDM_Driver_SetPongTime(LCDM_DRIVER *d, time_t t);

void LCDM_Driver_SetTimeout(LCDM_DRIVER *d, int secs);
int LCDM_Driver_CheckTimeout(const LCDM_DRIVER *d);

GWEN_DB_NODE *LCDM_Driver_GetDriverVars(const LCDM_DRIVER *d);

void LCDM_Driver_Dump(const LCDM_DRIVER *d, FILE *f, int indent);

#endif /* CHIPCARD_SERVER_DM_DRIVER_L_H */


