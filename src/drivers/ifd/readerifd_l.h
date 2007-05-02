/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerccid_l.h 154 2005-12-09 00:52:35Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_READER_IFD_L_H
#define CHIPCARD_READER_IFD_L_H

#include <chipcard3/server/driver/reader.h>


void ReaderIFD_Extend(LCD_READER *r);

GWEN_TYPE_UINT32 ReaderIFD_GetFeatureCode(const LCD_READER *r, int f);
void ReaderIFD_SetFeatureCode(LCD_READER *r, int f, GWEN_TYPE_UINT32 c);


#endif