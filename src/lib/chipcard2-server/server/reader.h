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


#ifndef CHIPCARD_SERVER_READER_H
#define CHIPCARD_SERVER_READER_H


typedef struct LC_READER LC_READER;

#define LC_READER_FLAGS_KEYPAD  0x00010000
#define LC_READER_FLAGS_DISPLAY 0x00020000


#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <chipcard2-server/server/driver.h>
#include <chipcard2-server/server/request.h>

#include <time.h>


GWEN_LIST_FUNCTION_DEFS(LC_READER, LC_Reader);


typedef enum {
  LC_ReaderStatusDown=0,
  LC_ReaderStatusWaitForDriver,
  LC_ReaderStatusWaitForReaderUp,
  LC_ReaderStatusWaitForReaderDown,
  LC_ReaderStatusUp,
  LC_ReaderStatusAborted,
  LC_ReaderStatusDisabled,
  LC_ReaderStatusUnknown=999
} LC_READER_STATUS;



LC_READER *LC_Reader_new(LC_DRIVER *d);
LC_READER *LC_Reader_FromDb(LC_DRIVER *d, GWEN_DB_NODE *db);
void LC_Reader_free(LC_READER *r);

void LC_Reader_ToDb(const LC_READER *r, GWEN_DB_NODE *db);

LC_DRIVER *LC_Reader_GetDriver(const LC_READER *r);

const char *LC_Reader_GetReaderType(const LC_READER *r);
void LC_Reader_SetReaderType(LC_READER *r, const char *s);

const char *LC_Reader_GetReaderName(const LC_READER *r);
void LC_Reader_SetReaderName(LC_READER *r, const char *s);


const char *LC_Reader_GetReaderInfo(const LC_READER *r);
void LC_Reader_SetReaderInfo(LC_READER *r, const char *s);

GWEN_TYPE_UINT32 LC_Reader_GetReaderId(const LC_READER *r);

LC_READER_STATUS LC_Reader_GetStatus(const LC_READER *r);
void LC_Reader_SetStatus(LC_READER *r, LC_READER_STATUS st);

int LC_Reader_IsAvailable(const LC_READER *r);
void LC_Reader_SetIsAvailable(LC_READER *r, int i);

time_t LC_Reader_GetLastStatusChangeTime(const LC_READER *r);
time_t LC_Reader_GetIdleSince(const LC_READER *r);
time_t LC_Reader_GetCommandTime(const LC_READER *r);

GWEN_TYPE_UINT32 LC_Reader_GetUsageCount(const LC_READER *r);
void LC_Reader_IncUsageCount(LC_READER *r);
void LC_Reader_DecUsageCount(LC_READER *r);

unsigned int LC_Reader_GetSlots(const LC_READER *r);
void LC_Reader_SetSlots(LC_READER *r, unsigned int i);

unsigned int LC_Reader_GetPort(const LC_READER *r);
void LC_Reader_SetPort(LC_READER *r, unsigned int i);

const char *LC_Reader_GetShortDescr(const LC_READER *r);
void LC_Reader_SetShortDescr(LC_READER *r, const char *s);

GWEN_TYPE_UINT32 LC_Reader_GetFlags(const LC_READER *r);
void LC_Reader_SetFlags(LC_READER *r, GWEN_TYPE_UINT32 f);
void LC_Reader_AddFlags(LC_READER *r, GWEN_TYPE_UINT32 f);
void LC_Reader_SubFlags(LC_READER *r, GWEN_TYPE_UINT32 f);

GWEN_TYPE_UINT32 LC_Reader_GetCurrentRequestId(const LC_READER *r);
void LC_Reader_SetCurrentRequestId(LC_READER *r, GWEN_TYPE_UINT32 rid);

int LC_Reader_HasNextRequest(const LC_READER *r);
LC_REQUEST *LC_Reader_GetNextRequest(LC_READER *r);
void LC_Reader_AddRequest(LC_READER *r, LC_REQUEST *rq);
void LC_Reader_ClearRequests(LC_READER *r);


GWEN_TYPE_UINT32 LC_Reader_GetVendorId(const LC_READER *r);
void LC_Reader_SetVendorId(LC_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LC_Reader_GetProductId(const LC_READER *r);
void LC_Reader_SetProductId(LC_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LC_Reader_GetBusId(const LC_READER *r);
void LC_Reader_SetBusId(LC_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LC_Reader_GetDeviceId(const LC_READER *r);
void LC_Reader_SetDeviceId(LC_READER *r, GWEN_TYPE_UINT32 i);

const char *LC_Reader_GetComType(const LC_READER *r);
void LC_Reader_SetComType(LC_READER *r, const char *s);

LC_READER *LC_Reader_Instantiate(LC_DRIVER *d, LC_READER *r);


int LC_Reader_GetWantRestart(const LC_READER *r);
void LC_Reader_SetWantRestart(LC_READER *r, int wantRestart);


void LC_Reader_Dump(const LC_READER *r, FILE *f, int indent);


#endif /* CHIPCARD_SERVER_READER_H */


