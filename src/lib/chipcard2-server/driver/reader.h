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


#ifndef CHIPCARD_DRIVER_READER_H
#define CHIPCARD_DRIVER_READER_H


#define LC_READER_FLAGS_KEYPAD  0x00010000
#define LC_READER_FLAGS_DISPLAY 0x00020000
#define LC_READER_FLAGS_NOINFO  0x00040000

#define LC_READER_STATUS_UP  0x00000001


#include <gwenhywfar/misc.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/inherit.h>


typedef struct LC_READER LC_READER;
GWEN_LIST_FUNCTION_DEFS(LC_READER, LC_Reader);
GWEN_INHERIT_FUNCTION_DEFS(LC_READER);

#include <chipcard2-server/driver/slot.h>


LC_READER *LC_Reader_new(GWEN_TYPE_UINT32 readerId,
                         const char *name,
                         int port,
                         unsigned int slots,
                         GWEN_TYPE_UINT32 flags);
void LC_Reader_free(LC_READER *r);

GWEN_TYPE_UINT32 LC_Reader_GetReaderId(const LC_READER *r);
const char *LC_Reader_GetName(const LC_READER *r);
int LC_Reader_GetPort(const LC_READER *r);

GWEN_TYPE_UINT32 LC_Reader_GetStatus(const LC_READER *r);
void LC_Reader_SetStatus(LC_READER *r, GWEN_TYPE_UINT32 s);
void LC_Reader_AddStatus(LC_READER *r, GWEN_TYPE_UINT32 s);
void LC_Reader_SubStatus(LC_READER *r, GWEN_TYPE_UINT32 s);

GWEN_TYPE_UINT32 LC_Reader_GetReaderFlags(const LC_READER *r);


GWEN_TYPE_UINT32 LC_Reader_GetDriverFlags(const LC_READER *r);
void LC_Reader_SetDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s);
void LC_Reader_AddDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s);
void LC_Reader_SubDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s);

LC_SLOT_LIST *LC_Reader_GetSlots(const LC_READER *r);
LC_SLOT *LC_Reader_FindSlot(const LC_READER *r, unsigned int slotnum);

const char *LC_Reader_GetLogger(const LC_READER *r);
void LC_Reader_SetLogger(LC_READER *r, const char *logDomain);

#endif /* CHIPCARD_DRIVER_READER_H */


