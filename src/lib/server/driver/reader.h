/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: reader_l.h 324 2006-10-02 11:51:34Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_READER_H
#define CHIPCARD_DRIVER_READER_H


#define LCD_READER_STATUS_UP  0x00000001


#include <gwenhywfar/misc.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/inherit.h>

#include <chipcard/chipcard.h>


typedef struct LCD_READER LCD_READER;
GWEN_LIST_FUNCTION_LIB_DEFS(LCD_READER, LCD_Reader, CHIPCARD_API);
GWEN_INHERIT_FUNCTION_LIB_DEFS(LCD_READER, CHIPCARD_API);

#include <chipcard/server/driver/slot.h>



CHIPCARD_API
LCD_READER *LCD_Reader_new(uint32_t readerId,
                           const char *name,
                           int port,
                           unsigned int slots,
                           uint32_t flags);

CHIPCARD_API
void LCD_Reader_free(LCD_READER *r);


CHIPCARD_API
uint32_t LCD_Reader_GetReaderId(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetReaderId(LCD_READER *r, uint32_t id);


CHIPCARD_API
const char *LCD_Reader_GetName(const LCD_READER *r);

CHIPCARD_API
int LCD_Reader_GetPort(const LCD_READER *r);


CHIPCARD_API
const char *LCD_Reader_GetDevicePath(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetDevicePath(LCD_READER *r, const char *s);


CHIPCARD_API
const char *LCD_Reader_GetReaderType(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetReaderType(LCD_READER *r, const char *s);


CHIPCARD_API
uint32_t LCD_Reader_GetDriversReaderId(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetDriversReaderId(LCD_READER *r, uint32_t id);


CHIPCARD_API
uint32_t LCD_Reader_GetStatus(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetStatus(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_AddStatus(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_SubStatus(LCD_READER *r, uint32_t s);


CHIPCARD_API
uint32_t LCD_Reader_GetReaderFlags(const LCD_READER *r);



CHIPCARD_API
uint32_t LCD_Reader_GetDriverFlags(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetDriverFlags(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_AddDriverFlags(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_SubDriverFlags(LCD_READER *r, uint32_t s);


CHIPCARD_API
uint32_t LCD_Reader_GetReaderFlags(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetReaderFlags(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_AddReaderFlags(LCD_READER *r, uint32_t s);

CHIPCARD_API
void LCD_Reader_SubReaderFlags(LCD_READER *r, uint32_t s);

CHIPCARD_API
LCD_SLOT_LIST *LCD_Reader_GetSlots(const LCD_READER *r);

CHIPCARD_API
LCD_SLOT *LCD_Reader_FindSlot(const LCD_READER *r, unsigned int slotnum);

CHIPCARD_API
const char *LCD_Reader_GetLogger(const LCD_READER *r);

CHIPCARD_API
void LCD_Reader_SetLogger(LCD_READER *r, const char *logDomain);


CHIPCARD_API
uint32_t LCD_Reader_GetErrorCount(const LCD_READER *r);

CHIPCARD_API
uint32_t LCD_Reader_IncErrorCount(LCD_READER *r);

CHIPCARD_API
void LCD_Reader_ResetErrorCount(LCD_READER *r);

#endif /* CHIPCARD_DRIVER_READER_H */


