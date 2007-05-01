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


#ifndef CHIPCARD_CLIENT_READERPCSC_P_H
#define CHIPCARD_CLIENT_READERPCSC_P_H


#include "readerpcsc_l.h"


struct LC_READER_PCSC {
  GWEN_LIST_ELEMENT(LC_READER_PCSC)
  char *readerName;
  char *readerType;
  LC_CARD *currentCard;

  GWEN_TYPE_UINT32 featureCode[LC_READER_PCSC_MAX_FEATURES];

};

#endif /* CHIPCARD_CLIENT_READERPCSC_P_H */

