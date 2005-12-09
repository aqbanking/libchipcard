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
#undef BUILDING_LIBCHIPCARD2_DLL

#include "readerccid_p.h"


GWEN_INHERIT(LCD_READER, READER_CCID);



void ReaderCCID_Extend(LCD_READER *r) {
  READER_CCID *rc;

  GWEN_NEW_OBJECT(READER_CCID, rc);
  GWEN_INHERIT_SETDATA(LCD_READER, READER_CCID, r, rc,
                       ReaderCCID_FreeData);
}



void ReaderCCID_FreeData(void *bp, void *p) {
  READER_CCID *rc;

  rc=(READER_CCID*)p;
  GWEN_FREE_OBJECT(rc);
}



GWEN_TYPE_UINT32 ReaderCCID_GetFeatureCode(const LCD_READER *r, int f) {
  READER_CCID *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_CCID, r);
  assert(r);

  if (f>=READER_CCID_MAX_FEATURES)
    return 0;

  return rc->featureCode[f];
}



void ReaderCCID_SetFeatureCode(LCD_READER *r, int f, GWEN_TYPE_UINT32 c) {
  READER_CCID *rc;

  assert(r);
  rc=GWEN_INHERIT_GETDATA(LCD_READER, READER_CCID, r);
  assert(r);

  if (f<READER_CCID_MAX_FEATURES)
    rc->featureCode[f]=c;
}




