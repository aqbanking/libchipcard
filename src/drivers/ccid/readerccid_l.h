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


#ifndef CHIPCARD_READER_CCID_L_H
#define CHIPCARD_READER_CCID_L_H

#include "reader_l.h"


void ReaderCCID_Extend(LCD_READER *r);

GWEN_TYPE_UINT32 ReaderCCID_GetVerifyCode(const LCD_READER *r);
void ReaderCCID_SetVerifyCode(LCD_READER *r, GWEN_TYPE_UINT32 c);

GWEN_TYPE_UINT32 ReaderCCID_GetModifyCode(const LCD_READER *r);
void ReaderCCID_SetModifyCode(LCD_READER *r, GWEN_TYPE_UINT32 c);


#endif
