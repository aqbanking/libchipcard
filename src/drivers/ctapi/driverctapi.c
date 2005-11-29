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


GWEN_INHERIT(LCD_DRIVER, DRIVER_CTAPI)


LCD_DRIVER *DriverCTAPI_new(int argc, char **argv) {
  DRIVER_CTAPI *dct;
  LCD_DRIVER *d;
  int rv;

  d=LCD_Driver_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_CTAPI, dct);
  GWEN_INHERIT_SETDATA(LCD_DRIVER, DRIVER_CTAPI,
                       d, dct,
                       DriverCTAPI_freeData);

  LCD_Driver_SetSendApduFn(d, DriverCTAPI_SendAPDU);
  LCD_Driver_SetConnectSlotFn(d, DriverCTAPI_ConnectSlot);
  LCD_Driver_SetDisconnectSlotFn(d, DriverCTAPI_DisconnectSlot);
  LCD_Driver_SetConnectReaderFn(d, DriverCTAPI_ConnectReader);
  LCD_Driver_SetDisconnectReaderFn(d, DriverCTAPI_DisconnectReader);
  LCD_Driver_SetResetSlotFn(d, DriverCTAPI_ResetSlot);
  LCD_Driver_SetReaderStatusFn(d, DriverCTAPI_ReaderStatus);
  LCD_Driver_SetReaderInfoFn(d, DriverCTAPI_ReaderInfo);
  LCD_Driver_SetCreateReaderFn(d, DriverCTAPI_CreateReader);
  LCD_Driver_SetGetErrorTextFn(d, DriverCTAPI_GetErrorText);

  rv=LCD_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LCD_Driver_free(d);
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



int DriverCTAPI_Start(LCD_DRIVER *d) {
  GWEN_ERRORCODE err;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  GWEN_LibLoader_free(dct->libLoader);
  dct->libLoader=GWEN_LibLoader_new();
  err=GWEN_LibLoader_OpenLibrary(dct->libLoader,
                                 LCD_Driver_GetLibraryFile(d));
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, "ERROR", "Loading library")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_init",
                             (void*)&dct->initFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_data",
                             (void*)&dct->dataFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "CT_close",
                             (void*)&dct->closeFn);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, "ERROR", "Resolving symbols")) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  /* send status report to server */
  if (LCD_Driver_Connect(d, "OK", "Library loaded")) {
    DBG_ERROR(0, "Error communicating with the server");
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    return -1;
  }
  DBG_INFO(0, "Connected.");
  return 0;
}



const char *DriverCTAPI_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err) {
  const char *s;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
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



GWEN_TYPE_UINT32 DriverCTAPI_SendAPDU(LCD_DRIVER *d,
                                      int toReader,
                                      LCD_READER *r,
                                      LCD_SLOT *slot,
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
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  assert(r);
  assert(apdu);
  assert(apdulen);
  assert(buffer);

  sad=LCD_DRIVERCTAPI_SAD_HOST;
  if (toReader<0)
    dad=LCD_DRIVERCTAPI_DAD_CT;
  else if (toReader>0)
    dad=toReader;
  else {
    if (slot)
      dad=DriverCTAPI_TransformDAD(LCD_Slot_GetSlotNum(slot));
    else
      dad=LCD_DRIVERCTAPI_DAD_CT;
  }

  lg=LCD_Reader_GetLogger(r);

  DBG_INFO(lg,
           "Sending command:");
  GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);
  DBG_DEBUG(lg,
            "CTN=%d, SAD=%d, DAD=%d, "
            "CLA=%02x, INS=%02x, P1=%02x, P2=%02x (%d bytes)",
            ReaderCTAPI_GetCtn(r), sad, dad,
            apdu[0],
            (apdulen>1)?apdu[1]:0,
            (apdulen>2)?apdu[2]:0,
            (apdulen>3)?apdu[3]:0,
            apdulen);
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
  if (lr>258) {
    DBG_ERROR(lg,
              "Bad response size (%d)",lr);
    return DRIVER_CTAPI_ERROR_BAD_RESPONSE;
  }

  DBG_INFO(lg,
           "Received response:");
  GWEN_Text_LogString((const char*)buffer, lr, lg, GWEN_LoggerLevelInfo);

  if (lr>1) {
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
  }

  *bufferlen=lr;
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  unsigned char apdu[]={0x20, 0x12, 0x01, 0x01, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  DRIVER_CTAPI *dct;
  LCD_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  r=LCD_Slot_GetReader(sl);
  assert(r);
  if (!(LCD_Reader_GetStatus(r) & LCD_READER_STATUS_UP)) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Reader has not been initialized");
    return DRIVER_CTAPI_ERROR_READER_INIT;
  }

  if (!(LCD_Slot_GetStatus(sl) & LCD_SLOT_STATUS_CARD_INSERTED)) {
    DBG_INFO(LCD_Reader_GetLogger(r),
             "No card in slot, will not connect");
    return 0;
  }

  DBG_INFO(LCD_Reader_GetLogger(r),
           "Connecting slot %d", LCD_Slot_GetSlotNum(sl));
  apdu[2]=LCD_Slot_GetSlotNum(sl)+1;

  lr=sizeof(responseBuffer);
  if (LCD_Driver_SendAPDU(d, 1, r, sl, apdu, sizeof(apdu),
                         responseBuffer,&lr)){
    return -1;
  }

  /* copy ATR */
  if (lr>2) {
    GWEN_BUFFER *atr;

    atr=GWEN_Buffer_new(0, lr-2, 0, 1);
    GWEN_Buffer_AppendBytes(atr, (const char*)responseBuffer, lr-2);
    LCD_Slot_SetAtr(sl, atr);
  }
  if (responseBuffer[lr-2]==0x90) {
    if (responseBuffer[lr-1]==1)
      LCD_Slot_AddFlags(sl, LCD_SLOT_FLAGS_PROCESSORCARD);
    else
      LCD_Slot_SubFlags(sl, LCD_SLOT_FLAGS_PROCESSORCARD);
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_INFO(LCD_Reader_GetLogger(r),
             "CTAPI: Soft error SW1=%02x, SW2=%02x",
             (unsigned char)responseBuffer[lr-2],
             (unsigned char)responseBuffer[lr-1]);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  unsigned char apdu[]={0x20, 0x14, 0x01, 0x00, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Disconnecting slot %d", LCD_Slot_GetSlotNum(sl));

  apdu[2]=LCD_Slot_GetSlotNum(sl)+1;
  lr=sizeof(responseBuffer);
  if (LCD_Driver_SendAPDU(d, 1, LCD_Slot_GetReader(sl), sl, apdu, sizeof(apdu),
                         responseBuffer,&lr)){
    return -1;
  }

  if (responseBuffer[lr-2]==0x90) {
    DBG_NOTICE(0, "Card disconnected");
  }
  else {
    DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
               "CTAPI: Soft error SW1=%02x, SW2=%02x",
               (unsigned char)responseBuffer[lr-2],
               (unsigned char)responseBuffer[lr-1]);
  }
  //LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
  LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  GWEN_TYPE_UINT32 currStatus;
  int rv;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
             "Resetting slot");
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



GWEN_TYPE_UINT32 DriverCTAPI_ReaderStatus(LCD_DRIVER *d, LCD_READER *r) {
  unsigned char apdu[]={0x20, 0x13, 0x00, 0x80, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slList;
  unsigned char *p;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  sl=0;
  DBG_DEBUG(LCD_Reader_GetLogger(r),
            "Checking reader status for reader \"%08x\"",
            LCD_Reader_GetReaderId(r));

  lr=sizeof(responseBuffer);
  if (LCD_Driver_SendAPDU(d, 1,
                         r,
                         sl, apdu, sizeof(apdu), responseBuffer,&lr)){
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Error sending APDU");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_AVAILABLE;
  }

  /* check whether a complete tag is returned */
  if (responseBuffer[0]==0x80)
    /* a full TLV is returned, skip T and L */
    p=responseBuffer+2;
  else
    p=responseBuffer;

  slList=LCD_Reader_GetSlots(r);
  sl=LCD_Slot_List_First(slList);
  while(sl) {
    int slotNum;

    slotNum=LCD_Slot_GetSlotNum(sl);
    if (slotNum>=lr-2) {
      DBG_ERROR(LCD_Reader_GetLogger(r),
                "No response for slot %d", slotNum);
    }
    else {
      if ((p[slotNum] & 0x1)) {
        DBG_DEBUG(LCD_Reader_GetLogger(r),
                  "Slot %d has a card inserted", slotNum);
        LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
      }
      else {
        DBG_DEBUG(LCD_Reader_GetLogger(r),
                  "Slot %d has no card inserted", slotNum);
        LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
      }
      if (((p[slotNum] & 0xc)==0x4)) {
        DBG_DEBUG(LCD_Reader_GetLogger(r),
                  "Slot %d is connected", slotNum);
        LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
      }
      else if (((p[slotNum] & 0xc)==0x0) && (p[slotNum] & 0x1)) {
        DBG_DEBUG(LCD_Reader_GetLogger(r),
                  "No connection info, assuming slot %d is connected",
                  slotNum);
        LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
      }
      else {
        DBG_DEBUG(LCD_Reader_GetLogger(r),
                  "Slot %d is not connected", slotNum);
        LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
      }
    }
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ReadReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                            GWEN_BUFFER *buf) {
  unsigned char apdu[]={0x20, 0x13, 0x00, 0x46, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  unsigned char *p;
  int i;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Requesting information about reader \"%08x\"",
             LCD_Reader_GetReaderId(r));

  lr=sizeof(responseBuffer);
  if (LCD_Driver_SendAPDU(d, 1,
                          r, 0, apdu, sizeof(apdu), responseBuffer, &lr)){
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Error sending APDU");
    return DRIVER_CTAPI_ERROR_GENERIC;
  }

  if (lr<17) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Too short response when requesting reader information");
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LCD_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    return DRIVER_CTAPI_ERROR_BAD_RESPONSE;
  }

  /* check whether a complete tag is returned */
  if (responseBuffer[0]!=0x46 ||
      responseBuffer[1]!=lr-4) {
    DBG_WARN(LCD_Reader_GetLogger(r),
             "No/bad tag received when requesting reader information (%02x)",
             responseBuffer[0]);
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LCD_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    p=responseBuffer;
  }
  else
    p=responseBuffer+2;

  /* CTM (CT manufacturer) */
  for (i=0; i<5; i++)
    if (!isspace(p[i]))
      break;
  if (i<5) {
    GWEN_Buffer_AppendString(buf, "Manufacturer=\"");
    for (; i<5; i++) {
      if (p[i]<33)
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
    GWEN_Buffer_AppendString(buf, "\"");
  }
  p+=5;

  /* CTT (CT type) */
  for (i=0; i<5; i++)
    if (!isspace(p[i]))
      break;
  if (i<5) {
    GWEN_Buffer_AppendString(buf, ";Terminal=\"");
    for (; i<5; i++) {
      if (p[i]<33)
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
    GWEN_Buffer_AppendString(buf, "\"");
  }
  p+=5;

  /* CTSV (CT software version) */
  for (i=0; i<5; i++)
    if (!isspace(p[i]))
      break;
  if (i<5) {
    GWEN_Buffer_AppendString(buf, ";Version=\"");
    for (; i<5; i++) {
      if (p[i]<33)
        break;
      GWEN_Buffer_AppendByte(buf, p[i]);
    } /* for */
    GWEN_Buffer_AppendString(buf, "\"");
  }
  p+=5;

  if (lr>18) {
    for (i=0; i<lr-18; i++)
      if (!isspace(p[i]))
        break;
    if (i<18) {
      GWEN_Buffer_AppendString(buf, ";info=\"");
      for (; i<lr-18; i++) {
        if (p[i]==0)
          break;
        GWEN_Buffer_AppendByte(buf, p[i]);
      }
      GWEN_Buffer_AppendString(buf, "\"");
    }
  }

  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ReadReaderUnits(LCD_DRIVER *d, LCD_READER *r,
                                             GWEN_BUFFER *buf) {
  unsigned char apdu[]={0x20, 0x13, 0x00, 0x81, 0x00};
  unsigned char responseBuffer[300];
  int lr;
  unsigned char *p;
  int i;
  DRIVER_CTAPI *dct;
  int len;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Requesting units of reader \"%08x\"",
             LCD_Reader_GetReaderId(r));

  lr=sizeof(responseBuffer);
  if (LCD_Driver_SendAPDU(d, 1,
                         r, 0, apdu, sizeof(apdu), responseBuffer, &lr)){
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Error sending APDU");
    return DRIVER_CTAPI_ERROR_GENERIC;
  }

  if (lr<3) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Too short response when requesting reader unit list");
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LCD_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    return DRIVER_CTAPI_ERROR_BAD_RESPONSE;
  }

  /* check whether a complete tag is returned */
  if (responseBuffer[0]!=0x81) {
    DBG_WARN(LCD_Reader_GetLogger(r),
             "No/bad tag received when requesting reader units (%02x)",
             responseBuffer[0]);
    GWEN_Text_LogString((const char*)responseBuffer, lr,
                        LCD_Reader_GetLogger(r),
                        GWEN_LoggerLevelError);
    p=responseBuffer;
    len=lr-2;
  }
  else {
    p=responseBuffer+2;
    len=responseBuffer[1];
  }

  for (i=0; i<len; i++) {
    char ubuf[32];

    snprintf(ubuf, sizeof(ubuf), "unit%d=\"", i);
    if (GWEN_Buffer_GetUsedBytes(buf))
      GWEN_Buffer_AppendString(buf, ";");
    GWEN_Buffer_AppendString(buf, ubuf);

    if (p[i]==(unsigned char)0x40)
      GWEN_Buffer_AppendString(buf, "KEYBOARD");
    else if (p[i]==(unsigned char)0x50)
      GWEN_Buffer_AppendString(buf, "DISPLAY");
    else if (p[i]==(unsigned char)0x70)
      GWEN_Buffer_AppendString(buf, "FINGERPRINT");
    else if (p[i]<14){
      char numbuf[16];

      snprintf(numbuf, sizeof(numbuf), "%d", p[i]);
      GWEN_Buffer_AppendString(buf, "icc");
      GWEN_Buffer_AppendString(buf, numbuf);
    }
    else {
      char numbuf[16];

      snprintf(numbuf, sizeof(numbuf), "%d", p[i]);
      DBG_WARN(LCD_Reader_GetLogger(r),
               "Got unknown unit %d (%02x)",
               p[i], p[i]);
      GWEN_Buffer_AppendString(buf, numbuf);
    }
    GWEN_Buffer_AppendString(buf, "\"");
  }

  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                        GWEN_BUFFER *buf) {
  GWEN_TYPE_UINT32 res1, res2;

  res1=DriverCTAPI_ReadReaderInfo(d, r, buf);
  res2=DriverCTAPI_ReadReaderUnits(d, r, buf);
  if (res1 && res2)
    return res2;

  return 0;
}




LCD_READER *DriverCTAPI_CreateReader(LCD_DRIVER *d,
                                    GWEN_TYPE_UINT32 readerId,
                                    const char *name,
                                    int port,
                                    unsigned int slots,
                                    GWEN_TYPE_UINT32 flags){
  LCD_READER *r;
  DRIVER_CTAPI *dct;

  DBG_ERROR(0, "Creating reader...");

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  r=ReaderCTAPI_new(readerId, name, port, slots, flags,
                    (dct->nextCtn)++);
  DBG_NOTICE(0, "Created reader with CTN %d",
             ReaderCTAPI_GetCtn(r));
  return r;
}



GWEN_TYPE_UINT32 DriverCTAPI_ConnectReader(LCD_DRIVER *d, LCD_READER *r) {
#if 0
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
#endif
  char rvd;
  DRIVER_CTAPI *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);
  assert(r);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Initializing CTAPI driver with %d, %d (%04x)",
             ReaderCTAPI_GetCtn(r), LCD_Reader_GetPort(r),
             LCD_Reader_GetPort(r));
  rvd=dct->initFn(ReaderCTAPI_GetCtn(r), LCD_Reader_GetPort(r));
  if (rvd!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Could not init reader \"%s\" at port %d : %d",
              LCD_Reader_GetName(r),
              LCD_Reader_GetPort(r),
              rvd);
    LCD_Reader_SubStatus(r, LCD_READER_STATUS_UP);
    return rvd;
  }
  LCD_Reader_AddStatus(r, LCD_READER_STATUS_UP);

#if 0
  slotList=LCD_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LCD_Slot_List_First(slotList);
  while(sl) {
    if (!LCD_Driver_ConnectSlot(d, sl))
      oks++;
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Could not connect any slot");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_CONNECTED;
  }
#endif
  return 0;
}



GWEN_TYPE_UINT32 DriverCTAPI_DisconnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_CTAPI *dct;
  char rvd;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CTAPI, d);
  assert(dct);

  DBG_NOTICE(LCD_Reader_GetLogger(r),
             "Disconnecting reader %s", LCD_Reader_GetName(r));

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
  rvd=dct->closeFn(ReaderCTAPI_GetCtn(r));
  if (rvd!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Could not deinit reader \"%s\"",
              LCD_Reader_GetName(r));
    return rvd;
  }
  else {
    DBG_INFO(LCD_Reader_GetLogger(r),
             "Deinitializing reader %s: done", LCD_Reader_GetName(r));
  }

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Could not disconnect any slot");
    return DRIVER_CTAPI_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}







