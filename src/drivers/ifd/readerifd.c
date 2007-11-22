/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: readerccid.c 154 2005-12-09 00:52:35Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "readerifd_p.h"


GWEN_INHERIT(LCD_READER, READER_IFD);



void ReaderIFD_Extend(LCD_READER *r) {
  READER_IFD *rc;

  GWEN_NEW_OBJECT(READER_IFD, rc);
  GWEN_INHERIT_SETDATA(LCD_READER, READER_IFD, r, rc,
                       ReaderIFD_FreeData);
}



void GWENHYWFAR_CB ReaderIFD_FreeData(void *bp, void *p) {
  READER_IFD *rc;

  rc=(READER_IFD*)p;
  GWEN_FREE_OBJECT(rc);
}



uint32_t ReaderIFD_GetFeatureCode(const LCD_READER *r, int f) {
  READER_IFD *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_IFD, r);
  assert(r);

  if (f>=READER_IFD_MAX_FEATURES)
    return 0;

  return rc->featureCode[f];
}



void ReaderIFD_SetFeatureCode(LCD_READER *r, int f, uint32_t c) {
  READER_IFD *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_IFD, r);
  assert(r);

  if (f<READER_IFD_MAX_FEATURES)
    rc->featureCode[f]=c;
}




