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


#ifndef CHIPCARD_READER_CCID_P_H
#define CHIPCARD_READER_CCID_P_H

#include "readerccid_l.h"


typedef struct READER_CCID READER_CCID;
struct READER_CCID {
  GWEN_TYPE_UINT32 verifyCode;
  GWEN_TYPE_UINT32 modifyCode;
};
void ReaderCCID_FreeData(void *bp, void *p);


#endif
