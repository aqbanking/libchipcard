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


#ifndef LC_MON_READER_P_H
#define LC_MON_READER_P_H


#include "reader_l.h"


struct LCM_READER {
  GWEN_LIST_ELEMENT(LCM_READER)
  GWEN_TYPE_UINT32 serverId;
  char *readerId;
  char *driverId;
  char *readerType;
  char *readerName;
  char *shortDescr;
  int readerPort;
  GWEN_TYPE_UINT32 readerFlags;

  char *status;
  time_t lastChangeTime;
  GWEN_BUFFER *logBuffer;
};




#endif

