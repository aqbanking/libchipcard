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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "readerctapi_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <chipcard2/chipcard2.h>

#include <unistd.h>
#include <ctype.h>


GWEN_INHERIT(LC_READER, READER_CTAPI)




LC_READER *ReaderCTAPI_new(GWEN_TYPE_UINT32 readerId,
                           const char *name,
                           int port,
                           unsigned int slots,
                           GWEN_TYPE_UINT32 flags,
                           int ctn){
  LC_READER *r;
  READER_CTAPI *rc;

  r=LC_Reader_new(readerId, name, port, slots, flags);
  assert(r);
  GWEN_NEW_OBJECT(READER_CTAPI, rc);
  GWEN_INHERIT_SETDATA(LC_READER, READER_CTAPI, r, rc, ReaderCTAPI_freeData);

  rc->ctn=ctn;
  return r;
}



void ReaderCTAPI_freeData(void *bp, void *p){
  READER_CTAPI *rc;

  rc=(READER_CTAPI*)p;

  free(rc);
}



int ReaderCTAPI_GetCtn(const LC_READER *r){
  READER_CTAPI *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LC_READER, READER_CTAPI, r);
  assert(rc);

  return rc->ctn;
}



void ReaderCTAPI_SetCtn(LC_READER *r, int ctn){
  READER_CTAPI *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LC_READER, READER_CTAPI, r);
  assert(rc);

  rc->ctn=ctn;
}








