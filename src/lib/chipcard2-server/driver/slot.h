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


#ifndef CHIPCARD_DRIVER_SLOT_H
#define CHIPCARD_DRIVER_SLOT_H


#define LC_SLOT_STATUS_CARD_INSERTED   0x00010000
#define LC_SLOT_STATUS_CARD_CONNECTED  0x00020000
#define LC_SLOT_STATUS_DISABLED        0x00040000

#define LC_SLOT_FLAGS_PROCESSORCARD    0x00010000
#define LC_SLOT_FLAGS_HASDISPLAY       0x00020000
#define LC_SLOT_FLAGS_HASKEYPAD        0x00040000


#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>
#include <time.h>

typedef struct LC_SLOT LC_SLOT;

GWEN_LIST_FUNCTION_DEFS(LC_SLOT, LC_Slot);

#include <chipcard2-server/driver/reader.h>


LC_SLOT *LC_Slot_new(LC_READER *r, unsigned int slotNum);
void LC_Slot_free(LC_SLOT *sl);

GWEN_TYPE_UINT32 LC_Slot_GetStatus(const LC_SLOT *sl);
void LC_Slot_SetStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s);
void LC_Slot_AddStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s);
void LC_Slot_SubStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s);

GWEN_TYPE_UINT32 LC_Slot_GetFlags(const LC_SLOT *sl);
void LC_Slot_SetFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s);
void LC_Slot_AddFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s);
void LC_Slot_SubFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s);

GWEN_TYPE_UINT32 LC_Slot_GetLastStatus(const LC_SLOT *sl);
void LC_Slot_SetLastStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s);

time_t LC_Slot_GetLastStatusChange(const LC_SLOT *sl);

LC_READER *LC_Slot_GetReader(const LC_SLOT *sl);
unsigned int LC_Slot_GetSlotNum(const LC_SLOT *sl);

GWEN_TYPE_UINT32 LC_Slot_GetCardNum(const LC_SLOT *sl);
void LC_Slot_SetCardNum(LC_SLOT *sl, GWEN_TYPE_UINT32 i);

GWEN_BUFFER *LC_Slot_GetAtr(const LC_SLOT *sl);
void LC_Slot_SetAtr(LC_SLOT *sl, GWEN_BUFFER *atr);

GWEN_TYPE_UINT32 LC_Slot_GetProtocolInfo(const LC_SLOT *sl);
void LC_Slot_SetProtocolInfo(LC_SLOT *sl, GWEN_TYPE_UINT32 i);



#endif /* CHIPCARD_DRIVER_SLOT_H */


