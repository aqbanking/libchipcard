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


#ifndef CHIPCARD_SERVER_DRIVER_H
#define CHIPCARD_SERVER_DRIVER_H


typedef struct LC_DRIVER LC_DRIVER;


#include <gwenhywfar/process.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <time.h>

/** driver is a remote driver, not started by the server */
#define LC_DRIVER_FLAGS_AUTO   0x00000001
#define LC_DRIVER_FLAGS_REMOTE 0x00000002


GWEN_LIST_FUNCTION_DEFS(LC_DRIVER, LC_Driver);


typedef enum {
  LC_DriverStatusDown=0,
  LC_DriverStatusStarted,
  LC_DriverStatusUp,
  LC_DriverStatusStopping,
  LC_DriverStatusAborted,
  LC_DriverStatusDisabled,
  LC_DriverStatusUnknown=999
} LC_DRIVER_STATUS;



LC_DRIVER *LC_Driver_new();
LC_DRIVER *LC_Driver_FromDb(GWEN_DB_NODE *db);
void LC_Driver_free(LC_DRIVER *d);

void LC_Driver_ToDb(const LC_DRIVER *d, GWEN_DB_NODE *db);

int LC_Driver_GetMaxReaders(const LC_DRIVER *d);
void LC_Driver_SetMaxReaders(LC_DRIVER *d, int maxReaders);

const char *LC_Driver_GetDriverType(const LC_DRIVER *d);
void LC_Driver_SetDriverType(LC_DRIVER *d, const char *s);

const char *LC_Driver_GetDriverName(const LC_DRIVER *d);
void LC_Driver_SetDriverName(LC_DRIVER *d, const char *s);

const char *LC_Driver_GetDriverDataDir(const LC_DRIVER *d);
void LC_Driver_SetDriverDataDir(LC_DRIVER *d, const char *s);

const char *LC_Driver_GetCustomerId(const LC_DRIVER *d);
void LC_Driver_SetCustomerId(LC_DRIVER *d, const char *s);

const char *LC_Driver_GetLibraryFile(const LC_DRIVER *d);
void LC_Driver_SetLibraryFile(LC_DRIVER *d, const char *s);

const char *LC_Driver_GetLogFile(const LC_DRIVER *d);
void LC_Driver_SetLogFile(LC_DRIVER *d, const char *s);

GWEN_TYPE_UINT32 LC_Driver_GetDriverId(const LC_DRIVER *d);

GWEN_PROCESS *LC_Driver_GetProcess(const LC_DRIVER *d);
void LC_Driver_SetProcess(LC_DRIVER *d, GWEN_PROCESS *p);

LC_DRIVER_STATUS LC_Driver_GetStatus(const LC_DRIVER *d);
void LC_Driver_SetStatus(LC_DRIVER *d, LC_DRIVER_STATUS st);

time_t LC_Driver_GetLastStatusChangeTime(const LC_DRIVER *d);
time_t LC_Driver_GetIdleSince(const LC_DRIVER *d);

GWEN_TYPE_UINT32 LC_Driver_GetActiveReadersCount(const LC_DRIVER *d);
void LC_Driver_ResetActiveReadersCount(LC_DRIVER *d);
void LC_Driver_IncActiveReadersCount(LC_DRIVER *d);
void LC_Driver_DecActiveReadersCount(LC_DRIVER *d);

GWEN_TYPE_UINT32 LC_Driver_GetAssignedReadersCount(const LC_DRIVER *d);
void LC_Driver_ResetAssignedReadersCount(LC_DRIVER *d);
void LC_Driver_IncAssignedReadersCount(LC_DRIVER *d);
void LC_Driver_DecAssignedReadersCount(LC_DRIVER *d);

GWEN_TYPE_UINT32 LC_Driver_GetDriverFlags(const LC_DRIVER *d);
void LC_Driver_SetDriverFlags(LC_DRIVER *d, GWEN_TYPE_UINT32 fl);
void LC_Driver_AddDriverFlags(LC_DRIVER *d, GWEN_TYPE_UINT32 fl);
void LC_Driver_SubDriverFlags(LC_DRIVER *d, GWEN_TYPE_UINT32 fl);

GWEN_TYPE_UINT32 LC_Driver_GetIpcId(const LC_DRIVER *d);
void LC_Driver_SetIpcId(LC_DRIVER *d, GWEN_TYPE_UINT32 id);


time_t LC_Driver_GetPingTime(const LC_DRIVER *d);
void LC_Driver_SetPingTime(LC_DRIVER *d, time_t t);
time_t LC_Driver_GetPongTime(const LC_DRIVER *d);
void LC_Driver_SetPongTime(LC_DRIVER *d, time_t t);

GWEN_DB_NODE *LC_Driver_GetDriverVars(const LC_DRIVER *d);


int LC_Driver_GetPendingCommandCount(const LC_DRIVER *d);
void LC_Driver_IncPendingCommandCount(LC_DRIVER *d);
void LC_Driver_DecPendingCommandCount(LC_DRIVER *d);

int LC_Driver_GetFirstNewPort(const LC_DRIVER *d);
int LC_Driver_GetAutoPortOffset(const LC_DRIVER *d);


void LC_Driver_Dump(const LC_DRIVER *d, FILE *f, int indent);

#endif /* CHIPCARD_SERVER_DRIVER_H */


