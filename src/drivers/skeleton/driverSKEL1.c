/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driverctapi.c 284 2006-09-22 00:53:00Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


/* SKEL1 -> skeleton
 * SKEL2 -> SKELETON
 * SKEL3 -> Skeleton
 *
 */


#include "driverSKEL1_p.h"
#include "readerSKEL1_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard3/chipcard3.h>

#include <unistd.h>
#include <ctype.h>


GWEN_INHERIT(LCD_DRIVER, DRIVER_SKEL2)


LCD_DRIVER *DriverSKEL3_new(int argc, char **argv) {
  DRIVER_SKEL2 *dct;
  LCD_DRIVER *d;

  d=LCD_Driver_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_SKEL2, dct);
  GWEN_INHERIT_SETDATA(LCD_DRIVER, DRIVER_SKEL2,
                       d, dct,
                       DriverSKEL3_freeData);

  LCD_Driver_SetSendApduFn(d, DriverSKEL3_SendAPDU);
  LCD_Driver_SetConnectSlotFn(d, DriverSKEL3_ConnectSlot);
  LCD_Driver_SetDisconnectSlotFn(d, DriverSKEL3_DisconnectSlot);
  LCD_Driver_SetConnectReaderFn(d, DriverSKEL3_ConnectReader);
  LCD_Driver_SetDisconnectReaderFn(d, DriverSKEL3_DisconnectReader);
  LCD_Driver_SetResetSlotFn(d, DriverSKEL3_ResetSlot);
  LCD_Driver_SetReaderStatusFn(d, DriverSKEL3_ReaderStatus);
  LCD_Driver_SetReaderInfoFn(d, DriverSKEL3_ReaderInfo);
  LCD_Driver_SetExtendReaderFn(d, DriverSKEL3_ExtendReader);
  LCD_Driver_SetGetErrorTextFn(d, DriverSKEL3_GetErrorText);

  return d;
}



void GWENHYWFAR_CB DriverSKEL3_freeData(void *bp, void *p) {
  DRIVER_SKEL2 *dct;

  dct=(DRIVER_SKEL2*)p;
  GWEN_FREE_OBJECT(dct);
}



int DriverSKEL3_Start(LCD_DRIVER *d) {
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  /* do something to prepare the driver */
  DBG_NOTICE(0, "Preparing the server...");

  /* send status report to server */
  if (LCD_Driver_Connect(d, 0, "Library loaded", 0, 0)) {
    DBG_ERROR(0, "Error communicating with the server");
    return -1;
  }
  DBG_INFO(0, "Connected.");
  return 0;
}



const char *DriverSKEL3_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err) {
  DRIVER_SKEL2 *dct;
  static char ebuf[256];

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  ebuf[0]=0;
  snprintf(ebuf, sizeof(ebuf), "Error %08x", err);
  ebuf[sizeof(ebuf)-1]=0;
  return ebuf;
}



GWEN_TYPE_UINT32 DriverSKEL3_SendAPDU(LCD_DRIVER *d,
                                      int toReader,
                                      LCD_READER *r,
                                      LCD_SLOT *slot,
                                      const unsigned char *apdu,
                                      unsigned int apdulen,
                                      unsigned char *buffer,
                                      int *bufferlen){
  DRIVER_SKEL2 *dct;
  const char *lg;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  assert(r);
  assert(apdu);
  assert(apdulen);
  assert(buffer);

  lg=LCD_Reader_GetLogger(r);

  DBG_INFO(lg,
           "Sending command:");
  GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);

  /* TODO: actually send the command */

  DBG_INFO(lg,
           "Received response:");
  GWEN_Text_LogString((const char*)buffer, *bufferlen,
                      lg, GWEN_LoggerLevelInfo);

  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  DRIVER_SKEL2 *dct;
  LCD_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  r=LCD_Slot_GetReader(sl);
  assert(r);
  if (!(LCD_Reader_GetStatus(r) & LCD_READER_STATUS_UP)) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Reader has not been initialized");
    return LC_ERROR_GENERIC;
  }

  if (!(LCD_Slot_GetStatus(sl) & LCD_SLOT_STATUS_CARD_INSERTED)) {
    DBG_INFO(LCD_Reader_GetLogger(r),
             "No card in slot, will not connect");
    return 0;
  }

  DBG_INFO(LCD_Reader_GetLogger(r),
           "Connecting slot %d", LCD_Slot_GetSlotNum(sl));

  /* TODO: actually connect the slot */

  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Disconnecting slot %d", LCD_Slot_GetSlotNum(sl));

  /* TODO: actually disconnect slot */

  //LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
  LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  GWEN_TYPE_UINT32 currStatus;
  int rv;
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
             "Resetting slot");

  /* TODO: This could be implemented in a completely other way, however,
   * we simply disconnect and reconnect the slot here
   */

  currStatus=LCD_Slot_GetStatus(sl);
  rv=LCD_Driver_DisconnectSlot(d, sl);
  DBG_ERROR(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)), "Slot reset");
  GWEN_Socket_Select(0, 0, 0, 1000);
  if (currStatus & LCD_SLOT_STATUS_CARD_CONNECTED) {
    rv|=LCD_Driver_ConnectSlot(d, sl);
  }
  else
    rv=0;
  return rv;
}



GWEN_TYPE_UINT32 DriverSKEL3_ReaderStatus(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  sl=0;
  DBG_DEBUG(LCD_Reader_GetLogger(r),
            "Checking reader status for reader \"%08x\"",
            LCD_Reader_GetReaderId(r));

  /* TODO: actually check the reader status */

  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                        GWEN_BUFFER *buf) {
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Requesting information about reader \"%08x\"",
             LCD_Reader_GetReaderId(r));

  /* TODO: actually retrieve the information */

  return 0;
}




int DriverSKEL3_ExtendReader(LCD_DRIVER *d, LCD_READER *r) {
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  ReaderSKEL3_Extend(r, (dct->nextCtn)++);
  DBG_NOTICE(0, "Created reader with CTN %d",
             ReaderSKEL3_GetCtn(r));
  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_ConnectReader(LCD_DRIVER *d, LCD_READER *r) {
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);
  assert(r);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Initializing CTAPI driver with %d, %d (%04x)",
             ReaderSKEL3_GetCtn(r), LCD_Reader_GetPort(r),
             LCD_Reader_GetPort(r));

  /* TODO: actually connect reader */

  return 0;
}



GWEN_TYPE_UINT32 DriverSKEL3_DisconnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_SKEL2 *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_SKEL2, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Disconnecting reader %s", LCD_Reader_GetName(r));

  /* first disconnect all slots */
  assert(r);
  slotList=LCD_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LCD_Slot_List_First(slotList);
  while(sl) {
    if (!LCD_Driver_DisconnectSlot(d, sl))
      oks++;
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  DBG_INFO(LCD_Reader_GetLogger(r),
           "Deinitializing reader %s", LCD_Reader_GetName(r));

  /* TODO: Actually disconnect the reader */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Could not disconnect any slot");
    return LC_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}









