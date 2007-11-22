/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerccid_p.h 154 2005-12-09 00:52:35Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_READER_IFD_P_H
#define CHIPCARD_READER_IFD_P_H

#include "readerifd_l.h"


#define READER_IFD_MAX_FEATURES 32

typedef struct READER_IFD READER_IFD;
struct READER_IFD {
  uint32_t featureCode[READER_IFD_MAX_FEATURES];
};
void GWENHYWFAR_CB ReaderIFD_FreeData(void *bp, void *p);



#endif
