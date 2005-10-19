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


#ifndef CHIPCARD_SERVER_DM_READER_L_H
#define CHIPCARD_SERVER_DM_READER_L_H


typedef struct LCDM_READER LCDM_READER;

#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/common/devmonitor.h>

#include <time.h>


GWEN_LIST_FUNCTION_DEFS(LCDM_READER, LCDM_Reader);

#include <chipcard2/chipcard2.h>
#include "devicemanager_l.h"
#include "dm_driver_l.h"


LCDM_READER *LCDM_Reader_new(LCDM_DRIVER *d);
void LCDM_Reader_free(LCDM_READER *r);

LCDM_READER *LCDM_Reader_fromDb(LCDM_DRIVER *d, GWEN_DB_NODE *db);
void LCDM_Reader_toDb(const LCDM_READER *r, GWEN_DB_NODE *db);

LCDM_DRIVER *LCDM_Reader_GetDriver(const LCDM_READER *r);

const char *LCDM_Reader_GetReaderType(const LCDM_READER *r);
void LCDM_Reader_SetReaderType(LCDM_READER *r, const char *s);

const char *LCDM_Reader_GetReaderName(const LCDM_READER *r);
void LCDM_Reader_SetReaderName(LCDM_READER *r, const char *s);


const char *LCDM_Reader_GetReaderInfo(const LCDM_READER *r);
void LCDM_Reader_SetReaderInfo(LCDM_READER *r, const char *s);

GWEN_TYPE_UINT32 LCDM_Reader_GetReaderId(const LCDM_READER *r);

GWEN_TYPE_UINT32 LCDM_Reader_GetDriversReaderId(const LCDM_READER *r);
void LCDM_Reader_SetDriversReaderId(LCDM_READER *r, GWEN_TYPE_UINT32 id);

LC_READER_STATUS LCDM_Reader_GetStatus(const LCDM_READER *r);
void LCDM_Reader_SetStatus(LCDM_READER *r, LC_READER_STATUS st);

int LCDM_Reader_IsAvailable(const LCDM_READER *r);
void LCDM_Reader_SetIsAvailable(LCDM_READER *r, int i);

time_t LCDM_Reader_GetLastStatusChangeTime(const LCDM_READER *r);
time_t LCDM_Reader_GetIdleSince(const LCDM_READER *r);

void LCDM_Reader_SetTimeout(LCDM_READER *r, int secs);
int LCDM_Reader_CheckTimeout(const LCDM_READER *r);

GWEN_TYPE_UINT32 LCDM_Reader_GetUsageCount(const LCDM_READER *r);
void LCDM_Reader_IncUsageCount(LCDM_READER *r, int count);
void LCDM_Reader_DecUsageCount(LCDM_READER *r, int count);

unsigned int LCDM_Reader_GetSlots(const LCDM_READER *r);
void LCDM_Reader_SetSlots(LCDM_READER *r, unsigned int i);

unsigned int LCDM_Reader_GetPort(const LCDM_READER *r);
void LCDM_Reader_SetPort(LCDM_READER *r, unsigned int i);

const char *LCDM_Reader_GetShortDescr(const LCDM_READER *r);
void LCDM_Reader_SetShortDescr(LCDM_READER *r, const char *s);

GWEN_TYPE_UINT32 LCDM_Reader_GetFlags(const LCDM_READER *r);
void LCDM_Reader_SetFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f);
void LCDM_Reader_AddFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f);
void LCDM_Reader_SubFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f);

LC_DEVICE_BUSTYPE LCDM_Reader_GetBusType(const LCDM_READER *r);
void LCDM_Reader_SetBusType(LCDM_READER *r, LC_DEVICE_BUSTYPE i);
GWEN_TYPE_UINT32 LCDM_Reader_GetVendorId(const LCDM_READER *r);
void LCDM_Reader_SetVendorId(LCDM_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCDM_Reader_GetProductId(const LCDM_READER *r);
void LCDM_Reader_SetProductId(LCDM_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCDM_Reader_GetBusId(const LCDM_READER *r);
void LCDM_Reader_SetBusId(LCDM_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCDM_Reader_GetDeviceId(const LCDM_READER *r);
void LCDM_Reader_SetDeviceId(LCDM_READER *r, GWEN_TYPE_UINT32 i);

GWEN_TYPE_UINT32 LCDM_Reader_GetCurrentRequestId(const LCDM_READER *r);
void LCDM_Reader_SetCurrentRequestId(LCDM_READER *r, GWEN_TYPE_UINT32 rid);


void LCDM_Reader_Dump(const LCDM_READER *r, FILE *f, int indent);


#endif /* CHIPCARD_SERVER_DM_READER_L_H */


