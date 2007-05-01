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


#ifndef CHIPCARD_READER_CTAPI_P_H
#define CHIPCARD_READER_CTAPI_P_H

#include "readerctapi.h"



typedef struct READER_CTAPI READER_CTAPI;
struct READER_CTAPI {
  int ctn;
};

void GWENHYWFAR_CB ReaderCTAPI_freeData(void *bp, void *p);


#endif /* CHIPCARD_READER_CTAPI_P_H */



