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


#ifndef CHIPCARD_DRIVER_READER_P_H
#define CHIPCARD_DRIVER_READER_P_H


#include <gwenhywfar/misc.h>

#include <chipcard2-server/driver/reader.h>


struct LC_READER {
  GWEN_LIST_ELEMENT(LC_READER)
  GWEN_INHERIT_ELEMENT(LC_READER)
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 driversReaderId;
  char *name;
  int port;
  GWEN_TYPE_UINT32 readerFlags;
  GWEN_TYPE_UINT32 driverFlags;
  GWEN_TYPE_UINT32 cardNum;
  GWEN_TYPE_UINT32 status;
  LC_SLOT_LIST *slots;
  char *logger;

  char *readerType;
};





#endif /* CHIPCARD_DRIVER_READER_P_H */


