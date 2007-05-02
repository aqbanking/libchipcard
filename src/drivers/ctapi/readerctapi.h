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


#ifndef CHIPCARD_READER_CTAPI_H
#define CHIPCARD_READER_CTAPI_H

#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/reader.h>


int ReaderCTAPI_Extend(LCD_READER *r, int ctn);

int ReaderCTAPI_GetCtn(const LCD_READER *r);
void ReaderCTAPI_SetCtn(LCD_READER *r, int ctn);


#endif /* CHIPCARD_READER_CTAPI_P_H */



