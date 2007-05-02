/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: slot_l.h 207 2006-09-07 23:54:18Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_SLOT_H
#define CHIPCARD_DRIVER_SLOT_H


#define LCD_SLOT_STATUS_CARD_INSERTED   0x00010000
#define LCD_SLOT_STATUS_CARD_CONNECTED  0x00020000
#define LCD_SLOT_STATUS_DISABLED        0x00040000

#define LCD_SLOT_FLAGS_PROCESSORCARD    0x00010000
#define LCD_SLOT_FLAGS_HASDISPLAY       0x00020000
#define LCD_SLOT_FLAGS_HASKEYPAD        0x00040000

#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>
#include <time.h>

#include <chipcard/chipcard.h>

typedef struct LCD_SLOT LCD_SLOT;
GWEN_LIST_FUNCTION_LIB_DEFS(LCD_SLOT, LCD_Slot, CHIPCARD_API);

#include <chipcard/server/driver/reader.h>



CHIPCARD_API
LCD_SLOT *LCD_Slot_new(LCD_READER *r, unsigned int slotNum);

CHIPCARD_API
void LCD_Slot_free(LCD_SLOT *sl);


CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Slot_GetStatus(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);

CHIPCARD_API
void LCD_Slot_AddStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);

CHIPCARD_API
void LCD_Slot_SubStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);


CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Slot_GetFlags(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);

CHIPCARD_API
void LCD_Slot_AddFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);

CHIPCARD_API
void LCD_Slot_SubFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);


CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Slot_GetLastStatus(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetLastStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s);


CHIPCARD_API
time_t LCD_Slot_GetLastStatusChange(const LCD_SLOT *sl);


CHIPCARD_API
LCD_READER *LCD_Slot_GetReader(const LCD_SLOT *sl);

CHIPCARD_API
unsigned int LCD_Slot_GetSlotNum(const LCD_SLOT *sl);


CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Slot_GetCardNum(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetCardNum(LCD_SLOT *sl, GWEN_TYPE_UINT32 i);


CHIPCARD_API
GWEN_BUFFER *LCD_Slot_GetAtr(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetAtr(LCD_SLOT *sl, GWEN_BUFFER *atr);


CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Slot_GetProtocolInfo(const LCD_SLOT *sl);

CHIPCARD_API
void LCD_Slot_SetProtocolInfo(LCD_SLOT *sl, GWEN_TYPE_UINT32 i);



#endif /* CHIPCARD_DRIVER_SLOT_H */


