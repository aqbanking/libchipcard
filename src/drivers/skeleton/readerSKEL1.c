/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerctapi.c 216 2006-09-08 01:58:35Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "readerSKEL1_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <chipcard/chipcard.h>

#include <unistd.h>
#include <ctype.h>


GWEN_INHERIT(LCD_READER, READER_SKEL2)




int ReaderSKEL3_Extend(LCD_READER *r, int ctn){
  READER_SKEL2 *rc;

  assert(r);
  GWEN_NEW_OBJECT(READER_SKEL2, rc);
  GWEN_INHERIT_SETDATA(LCD_READER, READER_SKEL2, r, rc, ReaderSKEL3_freeData);

  rc->ctn=ctn;
  return 0;
}



void GWENHYWFAR_CB ReaderSKEL3_freeData(void *bp, void *p){
  READER_SKEL2 *rc;

  rc=(READER_SKEL2*)p;

  GWEN_FREE_OBJECT(rc);
}



int ReaderSKEL3_GetCtn(const LCD_READER *r){
  READER_SKEL2 *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_SKEL2, r);
  assert(rc);

  return rc->ctn;
}



void ReaderSKEL3_SetCtn(LCD_READER *r, int ctn){
  READER_SKEL2 *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_SKEL2, r);
  assert(rc);

  rc->ctn=ctn;
}








