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

#include <chipcard2-server/driver/slot.h>



struct LC_SLOT {
  GWEN_LIST_ELEMENT(LC_SLOT);
  LC_READER *reader;
  unsigned int slotNum;
  GWEN_TYPE_UINT32 cardNum;
  GWEN_BUFFER *atr;
  GWEN_TYPE_UINT32 status;
  GWEN_TYPE_UINT32 lastStatus;
  GWEN_TYPE_UINT32 flags;
  time_t lastStatusChange;
  GWEN_TYPE_UINT32 protocolInfo;
};


#endif /* CHIPCARD_DRIVER_SLOT_P_H */


