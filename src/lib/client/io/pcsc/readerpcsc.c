/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "readerpcsc_p.h"
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>



GWEN_LIST_FUNCTIONS(LC_READER_PCSC, LC_ReaderPcsc)



LC_READER_PCSC *LC_ReaderPcsc_new(const char *rname) {
  LC_READER_PCSC *r;

  assert(rname);
  GWEN_NEW_OBJECT(LC_READER_PCSC, r);
  GWEN_LIST_INIT(LC_READER_PCSC, r);
  r->readerName=strdup(rname);

  return r;
}



void LC_ReaderPcsc_free(LC_READER_PCSC *r) {
  if (r) {
    GWEN_LIST_FINI(LC_READER_PCSC, r);
    free(r->readerName);
    free(r->readerType);
    GWEN_FREE_OBJECT(r);
  }
}



const char *LC_ReaderPcsc_GetReaderName(const LC_READER_PCSC *r) {
  assert(r);
  return r->readerName;
}



const char *LC_ReaderPcsc_GetReaderType(const LC_READER_PCSC *r) {
  assert(r);
  return r->readerType;
}



void LC_ReaderPcsc_SetReaderType(LC_READER_PCSC *r,
                                 const char *rtype) {
  assert(r);
  free(r->readerType);
  if (rtype)
    r->readerType=strdup(rtype);
  else
    r->readerType=0;
}



LC_CARD *LC_ReaderPcsc_GetCurrentCard(const LC_READER_PCSC *r) {
  assert(r);
  return r->currentCard;
}



void LC_ReaderPcsc_SetCurrentCard(LC_READER_PCSC *r, LC_CARD *card) {
  assert(r);
  r->currentCard=card;
}



uint32_t LC_ReaderPcsc_GetFeatureCode(const LC_READER_PCSC *r,
                                              int idx) {
  assert(r);
  assert(idx<LC_READER_PCSC_MAX_FEATURES);
  return r->featureCode[idx];
}



void LC_ReaderPcsc_SetFeatureCode(LC_READER_PCSC *r,
                                  int idx,
                                  uint32_t code) {
  assert(r);
  assert(idx<LC_READER_PCSC_MAX_FEATURES);
  r->featureCode[idx]=code;
}








