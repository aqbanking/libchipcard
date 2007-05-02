/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerctapi.h 122 2005-10-22 00:42:09Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_READER_SKEL2_H
#define CHIPCARD_READER_SKEL2_H

#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/reader.h>


int ReaderSKEL3_Extend(LCD_READER *r, int ctn);

int ReaderSKEL3_GetCtn(const LCD_READER *r);
void ReaderSKEL3_SetCtn(LCD_READER *r, int ctn);


#endif /* CHIPCARD_READER_SKEL2_P_H */



