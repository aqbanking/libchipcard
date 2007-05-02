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


#ifndef CHIPCARD_CLIENT_READERPCSC_L_H
#define CHIPCARD_CLIENT_READERPCSC_L_H

#include <gwenhywfar/misc.h>

#include <chipcard/client/card.h>


#define LC_READER_PCSC_MAX_FEATURES 32


typedef struct LC_READER_PCSC LC_READER_PCSC;

GWEN_LIST_FUNCTION_DEFS(LC_READER_PCSC, LC_ReaderPcsc)



LC_READER_PCSC *LC_ReaderPcsc_new(const char *rname);
void LC_ReaderPcsc_free(LC_READER_PCSC *r);

const char *LC_ReaderPcsc_GetReaderName(const LC_READER_PCSC *r);

const char *LC_ReaderPcsc_GetReaderType(const LC_READER_PCSC *r);
void LC_ReaderPcsc_SetReaderType(LC_READER_PCSC *r,
                                 const char *rtype);

LC_CARD *LC_ReaderPcsc_GetCurrentCard(const LC_READER_PCSC *r);
void LC_ReaderPcsc_SetCurrentCard(LC_READER_PCSC *r, LC_CARD *card);

GWEN_TYPE_UINT32 LC_ReaderPcsc_GetFeatureCode(const LC_READER_PCSC *r,
                                              int idx);

void LC_ReaderPcsc_SetFeatureCode(LC_READER_PCSC *r,
                                  int idx,
                                  GWEN_TYPE_UINT32 code);


#endif /* CHIPCARD_CLIENT_READERPCSC_L_H */

