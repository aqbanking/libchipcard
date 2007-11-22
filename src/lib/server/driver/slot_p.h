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


#ifndef CHIPCARD_DRIVER_SLOT_P_H
#define CHIPCARD_DRIVER_SLOT_P_H

#include "slot.h"



struct LCD_SLOT {
  GWEN_LIST_ELEMENT(LCD_SLOT);
  LCD_READER *reader;
  unsigned int slotNum;
  uint32_t cardNum;
  GWEN_BUFFER *atr;
  uint32_t status;
  uint32_t lastStatus;
  uint32_t flags;
  time_t lastStatusChange;
  uint32_t protocolInfo;
};


#endif /* CHIPCARD_DRIVER_SLOT_P_H */


