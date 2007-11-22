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

#include "reader.h"


struct LCD_READER {
  GWEN_LIST_ELEMENT(LCD_READER)
  GWEN_INHERIT_ELEMENT(LCD_READER)
  uint32_t readerId;
  uint32_t driversReaderId;
  char *name;
  int port;
  char *devicePath;
  uint32_t readerFlags;
  uint32_t driverFlags;
  uint32_t cardNum;
  uint32_t status;
  LCD_SLOT_LIST *slots;
  char *logger;

  char *readerType;

  uint32_t errorCount;
};





#endif /* CHIPCARD_DRIVER_READER_P_H */


