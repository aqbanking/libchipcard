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
#include <chipcard2-server/driver/reader.h>


LC_READER *ReaderCTAPI_new(GWEN_TYPE_UINT32 readerId,
                           const char *name,
                           int port,
                           unsigned int slots,
                           GWEN_TYPE_UINT32 flags,
                           int ctn);

int ReaderCTAPI_GetCtn(const LC_READER *r);
void ReaderCTAPI_SetCtn(LC_READER *r, int ctn);


#endif /* CHIPCARD_READER_CTAPI_P_H */



