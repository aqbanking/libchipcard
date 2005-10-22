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


#ifndef CHIPCARD_DRIVER_READER_L_H
#define CHIPCARD_DRIVER_READER_L_H


#define LCD_READER_STATUS_UP  0x00000001


#include <gwenhywfar/misc.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/inherit.h>


typedef struct LCD_READER LCD_READER;
GWEN_LIST_FUNCTION_DEFS(LCD_READER, LCD_Reader);
GWEN_INHERIT_FUNCTION_DEFS(LCD_READER);

#include "slot_l.h"


LCD_READER *LCD_Reader_new(GWEN_TYPE_UINT32 readerId,
                         const char *name,
                         int port,
                         unsigned int slots,
                         GWEN_TYPE_UINT32 flags);
void LCD_Reader_free(LCD_READER *r);

GWEN_TYPE_UINT32 LCD_Reader_GetReaderId(const LCD_READER *r);
void LCD_Reader_SetReaderId(LCD_READER *r, GWEN_TYPE_UINT32 id);

const char *LCD_Reader_GetName(const LCD_READER *r);
int LCD_Reader_GetPort(const LCD_READER *r);

const char *LCD_Reader_GetReaderType(const LCD_READER *r);
void LCD_Reader_SetReaderType(LCD_READER *r, const char *s);

GWEN_TYPE_UINT32 LCD_Reader_GetDriversReaderId(const LCD_READER *r);
void LCD_Reader_SetDriversReaderId(LCD_READER *r, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LCD_Reader_GetStatus(const LCD_READER *r);
void LCD_Reader_SetStatus(LCD_READER *r, GWEN_TYPE_UINT32 s);
void LCD_Reader_AddStatus(LCD_READER *r, GWEN_TYPE_UINT32 s);
void LCD_Reader_SubStatus(LCD_READER *r, GWEN_TYPE_UINT32 s);

GWEN_TYPE_UINT32 LCD_Reader_GetReaderFlags(const LCD_READER *r);


GWEN_TYPE_UINT32 LCD_Reader_GetDriverFlags(const LCD_READER *r);
void LCD_Reader_SetDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s);
void LCD_Reader_AddDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s);
void LCD_Reader_SubDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s);

LCD_SLOT_LIST *LCD_Reader_GetSlots(const LCD_READER *r);
LCD_SLOT *LCD_Reader_FindSlot(const LCD_READER *r, unsigned int slotnum);

const char *LCD_Reader_GetLogger(const LCD_READER *r);
void LCD_Reader_SetLogger(LCD_READER *r, const char *logDomain);

#endif /* CHIPCARD_DRIVER_READER_L_H */


