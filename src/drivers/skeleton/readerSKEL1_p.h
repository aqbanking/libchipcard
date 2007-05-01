/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerctapi_p.h 23 2005-01-24 23:54:15Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_READER_SKEL2_P_H
#define CHIPCARD_READER_SKEL2_P_H

#include "readerSKEL1.h"



typedef struct READER_SKEL2 READER_SKEL2;
struct READER_SKEL2 {
  int ctn;
};

void GWENHYWFAR_CB ReaderSKEL3_freeData(void *bp, void *p);


#endif /* CHIPCARD_READER_SKEL2_P_H */



