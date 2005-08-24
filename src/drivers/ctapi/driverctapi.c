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

#include "driverctapi_p.h"
#include "readerctapi.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard2/chipcard2.h>

#include <unistd.h>
#include <ctype.h>


GWEN_INHERIT(LC_DRIVER, DRIVER_CTAPI)


LC_DRIVER *DriverCTAPI_new(int argc, char **argv) {
  DRIVER_CTAPI *dct;
  LC_DRIVER *d;
  int rv;

  d=LC_Driver_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_CTAPI, dct);
  GWEN_INHERIT_SETDATA(LC_DRIVER, DRIVER_CTAPI,
                       d, dct,
                       DriverCTAPI_freeData);

  LC_Driver_SetSendApduFn(d, DriverCTAPI_SendAPDU);
  LC_Driver_SetConnectSlotFn(d, DriverCTAPI_ConnectSlot);
  LC_Driver_SetDisconnectSlotFn(d, DriverCTAPI_DisconnectSlot);
  LC_Driver_SetConnectReaderFn(d, DriverCTAPI_ConnectReader);
  LC_Driver_SetDisconnectReaderFn(d, DriverCTAPI_DisconnectReader);
  LC_Driver_SetResetSlotFn(d, DriverCTAPI_ResetSlot);
  LC_Driver_SetReaderStatusFn(d, DriverCTAPI_ReaderStatus);
  LC_Driver_SetReaderInfoFn(d, DriverCTAPI_ReaderInfo);
  LC_Driver_SetCreateReaderFn(d, DriverCTAPI_CreateReader);
  LC_Driver_SetGetErrorTextFn(d, DriverCTAPI_GetErrorText);

  rv=LC_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LC_Driver_free(d);
    return 0;
  }
  return d;
}



void DriverCTAPI_free(DRIVER_CTAPI *dct) {
  if (dct) {
    GWEN_LibLoader_free(dct->libLoader);
    GWEN_FREE_OBJECT(dct);
  }
}



void DriverCTAPI_freeData(void *bp, void *p) {
  DRIVER_CTAPI *dct;

  dct=(DRIVER_CTAPI*)p;
  DriverCTAPI_free(dct);
}



int DriverCTAPI_Start(LC_DRIVER *d) {
  GWEN_ERRORCODE err;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  GWEN_LibLoader_free(dct->libLoader);
  dct->libLoader=GWEN_LibLoader_new();
  err=GWEN_LibLoader_OpenLibrary(dct->libLoader,
                                 LC_Driver_GetLibraryFile(d));
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LC_Driver_Connect(d, "ERROR", "Loading library")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LC_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_init",
                             (void*)&dct->initFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LC_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LC_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_data",
                             (void*)&dct->dataFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LC_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LC_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_close",
                             (void*)&dct->closeFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LC_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LC_Driver_Disconnect(d);
    return -1;
  }

  /* send status report to server */
  if (LC_Driver_Connect(d, "OK", "Library loaded")) {
    DBG_ERROR(0, "Error communicating with the server");
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    return -1;
  }
  DBG_INFO(0, "Connected.");
  return 0;
}



const char *DriverCTAPI_GetErrorText(LC_DRIVER *d, GWEN_TYPE_UINT32 err) {
  const char *s;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  switch ((char)(err & 0xff)) {
  case 0:
    s="Success";
    break;
  case -1:
    s="Invalid port number";
    break;
  case -8:
    s="CT error";
    break;
  case -10:
    s="Transmission error (please check file access rights)";
    break;
  case -11:
    s="Memory allocation error inside CTAPI driver";
    break;
  case -128:
    s="HTSI error (whatever that means, its an interal CTAPI error)";
    break;
  case -2:
    s="File IO error inside CTAPI driver";
    break;
  case DRIVER_CTAPI_ERROR_BAD_RESPONSE:
    s="Bad response from CTAPI driver";
    break;
  case DRIVER_CTAPI_ERROR_NO_SLOTS_CONNECTED:
    s="Could not connect any slot";
    break;
  case DRIVER_CTAPI_ERROR_NO_SLOTS_DISCONNECTED:
    s="Could not disconnect any slot";
    break;
  case DRIVER_CTAPI_ERROR_NO_SLOTS_AVAILABLE:
    s="No slots available";
    break;
  case DRIVER_CTAPI_ERROR_GENERIC:
    s="Generic error";
    break;
  default:
    s="Unknow error code";
  };
  return s;
}


int DriverCTAPI_TransformDAD(int i) {
  /* Since DAD of value 1 means "CT" (i.e. the terminal itself is the
   * destination) the cards 1 to 14 are encoded otherwise:
   *  Card 0 = 0
   *  Card 1 = 2 (!)
   *  Card 2=  3 ...
   */
  if (i>0)
    i++;
  return i;
}



GWEN_TYPE_UINT32 DriverCTAPI_SendAPDU(LC_DRIVER *d,
                                      int toReader,
                                      LC_READER *r,
                                      LC_SLOT *slot,
                                      const unsigned char *apdu,
                                      unsigned int apdulen,
                                      unsigned char *buffer,
                                      int *bufferlen){
  char retval;
  unsigned char dad;
  unsigned char sad;
  unsigned short lr;
  DRIVER_CTAPI *dct;
  const char *lg;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  assert(r);
  assert(apdu);
  assert(apdulen>3);
  assert(buffer);

  sad=LC_DRIVERCTAPI_SAD_HOST;
  if (toReader)
    dad=LC_DRIVERCTAPI_DAD_CT;
  else {
    if (slot)
      dad=DriverCTAPI_TransformDAD(LC_Slot_GetSlotNum(slot));
    else
      dad=LC_DRIVERCTAPI_DAD_CT;
  }

  lg=LC_Reader_GetLogger(r);

  DBG_INFO(lg,
           "Sending command:");
  GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);
  DBG_DEBUG(lg,
            "CTN=%d, SAD=%d, DAD=%d, CLA=%02x, INS=%02x, P1=%02x, P2=%02x",
            ReaderCTAPI_GetCtn(r), sad, dad,
            apdu[0], apdu[1], apdu[2], apdu[3]);
  lr=*bufferlen;
  DBG_NOTICE(lg, "Calling dataFn");
  retval=dct->dataFn(ReaderCTAPI_GetCtn(r),
                     &dad,
                     &sad,
                     apdulen,
                     (unsigned char*)apdu,
                     &lr,
                     buffer);
  DBG_NOTICE(lg, "Returned from dataFn (%d)", retval);
  if (retval!=0) {
    DBG_ERROR(lg,
              "CTAPI error on \"CT_data\": %d", retval);
    return (unsigned char)retval;
  }
  if (lr<2 || lr>258) {
    DBG_ERROR(lg,
              "Bad response size (%d)",lr);
    return DRIVER_CTAPI_ERROR_BAD_RESPONSE;
  }

  DBG_INFO(lg,
           "Received response:");
  GWEN_Text_LogString((const char*)buffer, lr, lg, GWEN_LoggerLevelInfo);

  if ((unsigned char)buffer[lr-2]!=0x90) {
    if ((unsigned char)buffer[lr-2]!=0x62) {
      DBG_INFO(lg,
               "CTAPI: Error: SW1=%02x, SW2=%02x "
               "(CLA=%02x, INS=%02x, P1=%02x, P2=%02x)",
               (unsigned char)buffer[lr-2],
               (unsigned char)buffer[lr-1],
               apdu[0], apdu[1], apdu[2], apdu[3]);
    }
    else {
      DBG_NOTICE(lg,
                 "CTAPI: Error: SW1=%02x, SW2=%02x "
                 "(CLA=%02x, INS=%02x, P1=%02x, P2=%02x)",
                 (unsigned char)buffer[lr-2],
                 (unsigned char)buffer[lr-1],
                 apdu[0], apdu[1], apdu[2], apdu[3]);
    }
  }

  *bufferlen=lr;
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ConnectSlot(LC_DRIVER *d, LC_SLOT *sl) {
  unsigned char apdu[]={0x20, 0x12, 0x01, 0x01, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  DRIVER_CTAPI *dct;
  LC_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  r=LC_Slot_GetReader(sl);
  assert(r);
  if (!(LC_Reader_GetStatus(r) & LC_READER_STATUS_UP)) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Reader has not been initialized");
    return DRIVER_CTAPI_ERROR_READER_INIT;
  }

  if (!(LC_Slot_GetStatus(sl) & LC_SLOT_STATUS_CARD_INSERTED)) {
    DBG_INFO(LC_Reader_GetLogger(r),
             "No card in slot, will not connect");
    return 0;
  }

  DBG_INFO(LC_Reader_GetLogger(r),
           "Connecting slot %d", LC_Slot_GetSlotNum(sl));
  apdu[2]=LC_Slot_GetSlotNum(sl)+1;

  lr=sizeof(responseBuffer);
  if (LC_Driver_SendAPDU(d, 1, r, sl, apdu, sizeof(apdu),
                         responseBuffer,&lr)){
    return -1;
  }

  /* copy ATR */
  if (lr>2) {
    GWEN_BUFFER *atr;

    atr=GWEN_Buffer_new(0, lr-2, 0, 1);
    GWEN_Buffer_AppendBytes(atr, responseBuffer, lr-2);
    LC_Slot_SetAtr(sl, atr);
  }
  if (responseBuffer[lr-2]==0x90) {
    if (responseBuffer[lr-1]==1)
      LC_Slot_AddFlags(sl, LC_SLOT_FLAGS_PROCESSORCARD);
    else
      LC_Slot_SubFlags(sl, LC_SLOT_FLAGS_PROCESSORCARD);
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_INFO(LC_Reader_GetLogger(r),
             "CTAPI: Soft error SW1=%02x, SW2=%02x",
             (unsigned char)responseBuffer[lr-2],
             (unsigned char)responseBuffer[lr-1]);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_DisconnectSlot(LC_DRIVER *d, LC_SLOT *sl) {
  unsigned char apdu[]={0x20, 0x14, 0x01, 0x00, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_INFO(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
           "Disconnecting slot %d", LC_Slot_GetSlotNum(sl));

  apdu[2]=LC_Slot_GetSlotNum(sl)+1;
  lr=sizeof(responseBuffer);
  if (LC_Driver_SendAPDU(d, 1, LC_Slot_GetReader(sl), sl, apdu, sizeof(apdu),
                         responseBuffer,&lr)){
    return -1;
  }

  if (responseBuffer[lr-2]==0x90) {
    DBG_NOTICE(0, "Card disconnected");
  }
  else {
    DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
               "CTAPI: Soft error SW1=%02x, SW2=%02x",
               (unsigned char)responseBuffer[lr-2],
               (unsigned char)responseBuffer[lr-1]);
  }
  //LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
  LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ResetSlot(LC_DRIVER *d, LC_SLOT *sl) {
  GWEN_TYPE_UINT32 currStatus;
  int rv;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
             "Resetting slot");
  currStatus=LC_Slot_GetStatus(sl);
  rv=LC_Driver_DisconnectSlot(d, sl);
  DBG_ERROR(LC_Reader_GetLogger(LC_Slot_GetReader(sl)), "Slot reset");
  GWEN_Socket_Select(0, 0, 0, 1000);
  if (currStatus & LC_SLOT_STATUS_CARD_CONNECTED) {
    rv|=LC_Driver_ConnectSlot(d, sl);
  }
  else
    rv=0;
  return rv;
}



GWEN_TYPE_UINT32 DriverCTAPI_ReaderStatus(LC_DRIVER *d, LC_READER *r) {
  unsigned char apdu[]={0x20, 0x13, 0x00, 0x80, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  LC_SLOT *sl;
  LC_SLOT_LIST *slList;
  unsigned char *p;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  sl=0;
  DBG_DEBUG(LC_Reader_GetLogger(r),
            "Checking reader status for reader \"%08x\"",
            LC_Reader_GetReaderId(r));

  lr=sizeof(responseBuffer);
  if (LC_Driver_SendAPDU(d, 1,
                         r,
                         sl, apdu, sizeof(apdu), responseBuffer,&lr)){
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Error sending APDU");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_AVAILABLE;
  }

  /* check whether a complete tag is returned */
  if (responseBuffer[0]==0x80)
    /* a full TLV is returned, skip T and L */
    p=responseBuffer+2;
  else
    p=responseBuffer;

  slList=LC_Reader_GetSlots(r);
  sl=LC_Slot_List_First(slList);
  while(sl) {
    int slotNum;

    slotNum=LC_Slot_GetSlotNum(sl);
    if (slotNum>=lr-2) {
      DBG_ERROR(LC_Reader_GetLogger(r),
                "No response for slot %d", slotNum);
    }
    else {
      if ((p[slotNum] & 0x1)) {
        DBG_DEBUG(LC_Reader_GetLogger(r),
                  "Slot %d has a card inserted", slotNum);
        LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
      }
      else {
        DBG_DEBUG(LC_Reader_GetLogger(r),
                  "Slot %d has no card inserted", slotNum);
        LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
      }
      if (((p[slotNum] & 0xc)==0x4)) {
        DBG_DEBUG(LC_Reader_GetLogger(r),
                  "Slot %d is connected", slotNum);
        LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
      }
      else if (((p[slotNum] & 0xc)==0x0) && (p[slotNum] & 0x1)) {
        DBG_DEBUG(LC_Reader_GetLogger(r),
                  "No connection info, assuming slot %d is connected",
                  slotNum);
        LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
      }
      else {
        DBG_DEBUG(LC_Reader_GetLogger(r),
                  "Slot %d is not connected", slotNum);
        LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
      }
    }
    sl=LC_Slot_List_Next(sl);
  } /* while */

  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ReaderInfo(LC_DRIVER *d, LC_READER *r,
                                        GWEN_BUFFER *buf) {
  unsigned char apdu[]={0x20, 0x13, 0x00, 0x46, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  unsigned char *p;
  int i;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LC_Reader_GetLogger(r),
             "Requesting information about reader \"%08x\"",
             LC_Reader_GetReaderId(r));

  lr=sizeof(responseBuffer);
  if (LC_Driver_SendAPDU(d, 1,
                         r, 0, apdu, sizeof(apdu), responseBuffer, &lr)){
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Error sending APDU");
    return DRIVER_CTAPI_ERROR_GENERIC;
  }

  if (lr<17) {
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Too short response when requesting reader information");
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LC_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    return DRIVER_CTAPI_ERROR_BAD_RESPONSE;
  }

  /* check whether a complete tag is returned */
  if (responseBuffer[0]!=0x46 ||
      responseBuffer[1]!=lr-4) {
    DBG_WARN(LC_Reader_GetLogger(r),
             "No/bad tag received when requesting reader information (%02x)",
             responseBuffer[0]);
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LC_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    p=responseBuffer;
  }
  else
    p=responseBuffer+2;

  /* CTM (CT manufacturer) */
  if (!isspace(*p)) {
    GWEN_Buffer_AppendString(buf, "Manufacturer=");
    for (i=0; i<5; i++) {
      if (isspace(p[i]))
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
  }

  /* CTT (CT type) */
  p+=5;
  if (!isspace(*p)) {
    GWEN_Buffer_AppendString(buf, ";Terminal=");
    for (i=0; i<5; i++) {
      if (isspace(p[i]))
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
  }

  /* CTSV (CT software version) */
  p+=5;
  if (!isspace(*p)) {
    GWEN_Buffer_AppendString(buf, ";Version=");
    for (i=0; i<5; i++) {
      if (isspace(p[i]))
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
  }

  return 0;
}



LC_READER *DriverCTAPI_CreateReader(LC_DRIVER *d,
                                    GWEN_TYPE_UINT32 readerId,
                                    const char *name,
                                    int port,
                                    unsigned int slots,
                                    GWEN_TYPE_UINT32 flags){
  LC_READER *r;
  DRIVER_CTAPI *dct;

  DBG_ERROR(0, "Creating reader...");

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  r=ReaderCTAPI_new(readerId, name, port, slots, flags,
                    (dct->nextCtn)++);
  DBG_NOTICE(0, "Created reader with CTN %d",
             ReaderCTAPI_GetCtn(r));
  return r;
}



GWEN_TYPE_UINT32 DriverCTAPI_ConnectReader(LC_DRIVER *d, LC_READER *r) {
#if 0
  LC_SLOT *sl;
  LC_SLOT_LIST *slotList;
  unsigned int oks;
#endif
  char rvd;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);
  assert(r);

  DBG_NOTICE(LC_Reader_GetLogger(r),
             "Initializing CTAPI driver with %d, %d (%04x)",
             ReaderCTAPI_GetCtn(r), LC_Reader_GetPort(r),
             LC_Reader_GetPort(r));
  rvd=dct->initFn(ReaderCTAPI_GetCtn(r), LC_Reader_GetPort(r));
  if (rvd!=0) {
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Could not init reader \"%s\" at port %d : %d",
              LC_Reader_GetName(r),
              LC_Reader_GetPort(r),
              rvd);
    LC_Reader_SubStatus(r, LC_READER_STATUS_UP);
    return rvd;
  }
  LC_Reader_AddStatus(r, LC_READER_STATUS_UP);

#if 0
  slotList=LC_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LC_Slot_List_First(slotList);
  while(sl) {
    if (!LC_Driver_ConnectSlot(d, sl))
      oks++;
    sl=LC_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Could not connect any slot");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_CONNECTED;
  }
#endif
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_DisconnectReader(LC_DRIVER *d, LC_READER *r) {
  LC_SLOT *sl;
  LC_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_CTAPI *dct;
  char rvd;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LC_Reader_GetLogger(r),
             "Disconnecting reader %s", LC_Reader_GetName(r));

  assert(r);
  slotList=LC_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LC_Slot_List_First(slotList);
  while(sl) {
    if (!LC_Driver_DisconnectSlot(d, sl))
      oks++;
    sl=LC_Slot_List_Next(sl);
  } /* while */

  DBG_INFO(LC_Reader_GetLogger(r),
           "Deinitializing reader %s", LC_Reader_GetName(r));
  rvd=dct->closeFn(ReaderCTAPI_GetCtn(r));
  if (rvd!=0) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Could not deinit reader \"%s\"",
              LC_Reader_GetName(r));
    return rvd;
  }
  else {
    DBG_INFO(LC_Reader_GetLogger(r),
             "Deinitializing reader %s: done", LC_Reader_GetName(r));
  }

  if (!oks) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Could not disconnect any slot");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}







