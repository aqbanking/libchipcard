/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.h 282 2006-09-21 16:52:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER_COMMON_READER_H
#define CHIPCARD_SERVER_COMMON_READER_H

#include "common/devmonitor.h"

#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/db.h>

#include <time.h>
#include <stdio.h>


typedef struct LCCO_READER LCCO_READER;


GWEN_LIST_FUNCTION_DEFS(LCCO_READER, LCCO_Reader)
GWEN_LIST2_FUNCTION_DEFS(LCCO_READER, LCCO_Reader)
GWEN_INHERIT_FUNCTION_DEFS(LCCO_READER)


#include <chipcard3/chipcard3.h>


typedef enum {
  LCCO_Reader_AddressPort=0,
  LCCO_Reader_AddressDevicePath,
  LCCO_Reader_AddressUnknown=-1
} LCCO_READER_ADDRESS;

LCCO_READER_ADDRESS LCCO_ReaderAddress_fromString(const char *s);
const char *LCCO_ReaderAddress_toString(LCCO_READER_ADDRESS at);


LCCO_READER *LCCO_Reader_new();
LCCO_READER *LCCO_Reader_dup(const LCCO_READER *r);
void LCCO_Reader_free(LCCO_READER *r);
void LCCO_Reader_Attach(LCCO_READER *r);

void LCCO_Reader_List2_freeAll(LCCO_READER_LIST2 *rl);

LCCO_READER *LCCO_Reader_fromDb(GWEN_DB_NODE *db);
void LCCO_Reader_toDb(const LCCO_READER *r, GWEN_DB_NODE *db);

LCCO_READER_ADDRESS LCCO_Reader_GetAddressType(const LCCO_READER *r);
void LCCO_Reader_SetAddressType(LCCO_READER *r, LCCO_READER_ADDRESS a);

const char *LCCO_Reader_GetReaderType(const LCCO_READER *r);
void LCCO_Reader_SetReaderType(LCCO_READER *r, const char *s);

const char *LCCO_Reader_GetReaderName(const LCCO_READER *r);
void LCCO_Reader_SetReaderName(LCCO_READER *r, const char *s);


const char *LCCO_Reader_GetDriverName(const LCCO_READER *r);
void LCCO_Reader_SetDriverName(LCCO_READER *r, const char *s);

const char *LCCO_Reader_GetReaderInfo(const LCCO_READER *r);
void LCCO_Reader_SetReaderInfo(LCCO_READER *r, const char *s);

GWEN_TYPE_UINT32 LCCO_Reader_GetReaderId(const LCCO_READER *r);
void LCCO_Reader_SetReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LCCO_Reader_GetDriversReaderId(const LCCO_READER *r);
void LCCO_Reader_SetDriversReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 id);

LC_READER_STATUS LCCO_Reader_GetStatus(const LCCO_READER *r);
void LCCO_Reader_SetStatus(LCCO_READER *r, LC_READER_STATUS st);

int LCCO_Reader_IsAvailable(const LCCO_READER *r);
void LCCO_Reader_SetIsAvailable(LCCO_READER *r, int i);

time_t LCCO_Reader_GetLastStatusChangeTime(const LCCO_READER *r);

unsigned int LCCO_Reader_GetSlots(const LCCO_READER *r);
void LCCO_Reader_SetSlots(LCCO_READER *r, unsigned int i);

unsigned int LCCO_Reader_GetPort(const LCCO_READER *r);
void LCCO_Reader_SetPort(LCCO_READER *r, unsigned int i);

unsigned int LCCO_Reader_GetCtn(const LCCO_READER *r);
void LCCO_Reader_SetCtn(LCCO_READER *r, unsigned int i);

const char *LCCO_Reader_GetShortDescr(const LCCO_READER *r);
void LCCO_Reader_SetShortDescr(LCCO_READER *r, const char *s);

GWEN_TYPE_UINT32 LCCO_Reader_GetFlags(const LCCO_READER *r);
void LCCO_Reader_SetFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f);
void LCCO_Reader_AddFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f);
void LCCO_Reader_SubFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f);

LC_DEVICE_BUSTYPE LCCO_Reader_GetBusType(const LCCO_READER *r);
void LCCO_Reader_SetBusType(LCCO_READER *r, LC_DEVICE_BUSTYPE i);
GWEN_TYPE_UINT32 LCCO_Reader_GetVendorId(const LCCO_READER *r);
void LCCO_Reader_SetVendorId(LCCO_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCCO_Reader_GetProductId(const LCCO_READER *r);
void LCCO_Reader_SetProductId(LCCO_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCCO_Reader_GetBusId(const LCCO_READER *r);
void LCCO_Reader_SetBusId(LCCO_READER *r, GWEN_TYPE_UINT32 i);
GWEN_TYPE_UINT32 LCCO_Reader_GetDeviceId(const LCCO_READER *r);
void LCCO_Reader_SetDeviceId(LCCO_READER *r, GWEN_TYPE_UINT32 i);

const char *LCCO_Reader_GetDevicePath(const LCCO_READER *r);
void LCCO_Reader_SetDevicePath(LCCO_READER *r, const char *s);


const char *LCCO_Reader_GetDevicePathTmpl(const LCCO_READER *r);
void LCCO_Reader_SetDevicePathTmpl(LCCO_READER *r, const char *s);


void LCCO_Reader_Dump(const LCCO_READER *r, FILE *f, int indent);

#endif /* CHIPCARD_SERVER_COMMON_READER_H */


