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


#include "driverccid_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard2/chipcard2.h>

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>


GWEN_INHERIT(LCD_DRIVER, DRIVER_CCID)



LCD_DRIVER *DriverCCID_new(int argc, char **argv) {
  DRIVER_CCID *dct;
  LCD_DRIVER *d;
  int rv;

  d=LCD_Driver_new();
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_CCID, dct);
  GWEN_INHERIT_SETDATA(LCD_DRIVER, DRIVER_CCID,
                       d, dct,
                       DriverCCID_freeData);

  LCD_Driver_SetSendApduFn(d, DriverCCID_SendAPDU);
  LCD_Driver_SetConnectSlotFn(d, DriverCCID_ConnectSlot);
  LCD_Driver_SetDisconnectSlotFn(d, DriverCCID_DisconnectSlot);
  LCD_Driver_SetConnectReaderFn(d, DriverCCID_ConnectReader);
  LCD_Driver_SetDisconnectReaderFn(d, DriverCCID_DisconnectReader);
  LCD_Driver_SetResetSlotFn(d, DriverCCID_ResetSlot);
  LCD_Driver_SetReaderStatusFn(d, DriverCCID_ReaderStatus);
  LCD_Driver_SetReaderInfoFn(d, DriverCCID_ReaderInfo);
  LCD_Driver_SetGetErrorTextFn(d, DriverCCID_GetErrorText);

  rv=LCD_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LCD_Driver_free(d);
    return 0;
  }

  return d;
}



void DriverCCID_free(DRIVER_CCID *dct) {
  if (dct) {
    GWEN_LibLoader_free(dct->libLoader);
    GWEN_FREE_OBJECT(dct);
  }
}



void DriverCCID_freeData(void *bp, void *p) {
  DRIVER_CCID *dct;

  dct=(DRIVER_CCID*)p;
  DriverCCID_free(dct);
}


int DriverCCID_Start(LCD_DRIVER *d) {
  GWEN_ERRORCODE err;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
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
                             "CCIDHCreateChannel",
                             (void*)&dct->createChannelFn);
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
                             "CCIDHCloseChannel",
                             (void*)&dct->closeChannelFn);
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
                             "CCIDHPowerICC",
                             (void*)&dct->powerIccFn);
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
                             "CCIDHTransmitToICC",
                             (void*)&dct->transmitFn);
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
                             "CCIDHControl",
                             (void*)&dct->controlFn);
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
                             "CCIDHICCPresence",
                             (void*)&dct->presenceFn);
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
                             "CCIDHGetCapabilities",
                             (void*)&dct->getCapsFn);
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



const char *DriverCCID_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err) {
  const char *s;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  switch (err) {
  case CCID_NOT_SUPPORTED:
    s="Function not supported";
    break;
  case CCID_ICC_PRESENT:
    s="Card present";
    break;
  case CCID_ICC_NOT_PRESENT:
    s="Card not present";
    break;
  case DRIVER_CCID_ERROR_BAD_RESPONSE:
    s="Bad response from CCID driver";
    break;
  case DRIVER_CCID_ERROR_NO_SLOTS_CONNECTED:
    s="Could not connect any slot";
    break;
  case DRIVER_CCID_ERROR_NO_SLOTS_DISCONNECTED:
    s="Could not disconnect any slot";
    break;
  case DRIVER_CCID_ERROR_NO_SLOTS_AVAILABLE:
    s="No slot available";
    break;
  case DRIVER_CCID_ERROR_NOT_SUPPORTED:
    s="Function not supported";
    break;
  default:
    s="Unknow error code";
  };
  return s;
}



int DriverCCID_ExtractProtocolInfo(unsigned char *atr,
                                  unsigned int atrlen) {
  unsigned char *origAtr;
  int c;

  if (atrlen<3) {
    DBG_NOTICE(0, "ATR too short, assuming protocol 0");
    return 0;
  }

  origAtr=atr;
  atr++;          /* TS */
  c=*atr;         /* T0 */
  if (c & 0x10)   /* TA1 follows */
    atr++;
  if (c & 0x20)   /* TB1 follows */
    atr++;
  if (c & 0x40)   /* TC1 follows */
    atr++;

  if (c & 0x80) { /* found TD1 */
    atr++;
    if ((atr-origAtr)>=atrlen) {
      DBG_ERROR(0, "ATR too short, assuming protocol 0");
      return 0;
    }
    return (int)(*atr & 0xf);
  }
  DBG_NOTICE(0, "No TD1, assuming protocol 0");
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen){
  long retval;
  GWEN_TYPE_UINT32 tmplen;
  DRIVER_CCID *dct;
  const char *lg;
  GWEN_TYPE_UINT32 controlCode;

  lg=LCD_Reader_GetLogger(r);

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  assert(apdu);
  assert(apdulen>3);
  assert(buffer);

  tmplen=*bufferlen;

  controlCode=
      (apdu[0]<<24)+
      (apdu[1]<<16)+
      (apdu[2]<<8)+
      apdu[3];

  if (toReader) {
    assert(apdulen>7);
    DBG_INFO(lg,
             "Sending command to reader (ControlCode=%08x):", controlCode);
    GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);
    retval=dct->controlFn(LCD_Slot_GetSlotNum(slot),
                          controlCode,
                          apdu+4,
                          apdulen-4,
			  buffer,
			  *bufferlen,
			  &tmplen);
    *bufferlen=tmplen;
    if (retval==0 && *bufferlen>3) {
      /* special treatment */
      int rlen;

      DBG_INFO(lg, "Found response within response, extracting");
      GWEN_Text_LogString((const char*)buffer, *bufferlen, lg, GWEN_LoggerLevelInfo);
      rlen=buffer[2];
      if ((rlen+4)==*bufferlen) {
	unsigned char *pSrc;
	unsigned char *pDst;

	/* real response is burried within the complete response */
	pSrc=buffer+3;
	pDst=buffer;
	memmove(pDst, pSrc, rlen);
        *bufferlen=rlen;
        tmplen=rlen;
      }
    }
  }
  else {
    SCARD_IO_HEADER txHeader;
    SCARD_IO_HEADER rxHeader;

    DBG_INFO(lg,
             "Sending command to card (bufferlen=%d):", tmplen);
    GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);
    txHeader.protocol=LCD_Slot_GetProtocolInfo(slot);
    //txHeader.protocol=1;
    txHeader.length=sizeof(SCARD_IO_HEADER);
    rxHeader.length=sizeof(SCARD_IO_HEADER);
    retval=dct->transmitFn(LCD_Slot_GetSlotNum(slot),
                           txHeader,
                           apdu,
                           apdulen,
                           buffer,
                           &tmplen,
                           &rxHeader);
    *bufferlen=tmplen;
  }

  if (retval==0) {
    DBG_INFO(lg, "Response:");
    GWEN_Text_LogString((const char*)buffer, *bufferlen, lg, GWEN_LoggerLevelInfo);
  }

  if (retval!=0) {
    DBG_ERROR(lg,
              "CCID error on \"CCIDTransmit/Control\": %ld",
              retval);
    return retval;
  }
  if (tmplen<2 || tmplen>258) {
    DBG_ERROR(lg,
              "Bad response size (%d)", tmplen);
    return DRIVER_CCID_ERROR_BAD_RESPONSE;
  }

  if ((unsigned char)buffer[tmplen-2]!=0x90) {
    DBG_NOTICE(lg,
               "CCID: Error: SW1=%02x, SW2=%02x "
               "(CLA=%02x, INS=%02x, P1=%02x, P2=%02x)",
               (unsigned char)buffer[tmplen-2],
               (unsigned char)buffer[tmplen-1],
               apdu[0], apdu[1], apdu[2], apdu[3]);
  }

  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_CCID *dct;
  LCD_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  r=LCD_Slot_GetReader(sl);
  assert(r);

  DBG_INFO(LCD_Reader_GetLogger(r),
	   "Connecting slot %d", LCD_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  LCD_Slot_SetAtr(sl, 0);
  retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                         CCID_POWER_UP,
                         atrBuffer,
			 &atrLen);
  if (retval==CCID_ICC_PRESENT || retval==0) {
    int proto;

    DBG_INFO(LCD_Reader_GetLogger(r), "Card present.");
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    if (retval==0) {
      LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
    }

    if (atrLen) {
      GWEN_BUFFER *abuf;

      abuf=GWEN_Buffer_new(0, atrLen, 0, 1);
      GWEN_Buffer_AppendBytes(abuf, (const char*)atrBuffer, atrLen);
      LCD_Slot_SetAtr(sl, abuf);
    }

    proto=DriverCCID_ExtractProtocolInfo(atrBuffer, atrLen);
    if (proto==0) {
      DBG_NOTICE(LCD_Reader_GetLogger(r),
                 "Protocol is 0, assuming memorycard");
      LCD_Slot_SubFlags(sl, LCD_SLOT_FLAGS_PROCESSORCARD);
    }
    else {
      LCD_Slot_AddFlags(sl, LCD_SLOT_FLAGS_PROCESSORCARD);
    }
    LCD_Slot_SetProtocolInfo(sl, proto);
  }
  else if (retval==CCID_ICC_NOT_PRESENT) {
    DBG_NOTICE(LCD_Reader_GetLogger(r), "No card inserted");
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
	       "CCID: Soft error %d", (int)retval);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
    if (retval==CCID_ERROR_POWER_ACTION) {
      DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
		 "CCID: Ignoring error, assuming missing card");
    }
    else
      return retval;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);


  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Disconnecting slot %d", LCD_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                         CCID_POWER_DOWN,
                         atrBuffer,
                         &atrLen);
  if (retval==CCID_NOT_SUPPORTED || retval==CCID_NOT_SUPPORTED) {
    atrLen=sizeof(atrBuffer);
    retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                           CCID_RESET,
                           atrBuffer,
                           &atrLen);
  }
  if (retval!=0) {
    DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
               "CCID: Soft error %d", (int)retval);
  }

  LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);


  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Resetting slot %d", LCD_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                         CCID_RESET,
                         atrBuffer,
                         &atrLen);

  if (retval==CCID_ICC_PRESENT) {
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  }
  else if (retval==0) {
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
               "CCID: Soft error %d", (int)retval);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
    LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_ReaderStatus(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slList;
  long retval;
  DRIVER_CCID *dct;
  int oks;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  DBG_DEBUG(LCD_Reader_GetLogger(r),
            "Checking reader status for reader \"%08x\"",
            LCD_Reader_GetReaderId(r));

  slList=LCD_Reader_GetSlots(r);
  oks=0;
  sl=LCD_Slot_List_First(slList);
  while(sl) {
    int slotNum;

    slotNum=LCD_Slot_GetSlotNum(sl);
    retval=dct->presenceFn(LCD_Slot_GetSlotNum(sl));
    if (retval==CCID_ICC_PRESENT) {
      oks++;
      if (!(LCD_Slot_GetLastStatus(sl) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
        /* card is not connected, try to do so */
        if (LCD_Driver_ConnectSlot(d, sl)) {
          LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
          LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
        }
      }
      else {
        LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
      }
    }
    else if (retval==CCID_ICC_NOT_PRESENT){
      /* no card present, so there can be no card connected */
      oks++;
      LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
      LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    }
    else {
      DBG_NOTICE(LCD_Reader_GetLogger(r),
                 "CCID: Soft error %d", (int)retval);
      LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
      LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
      LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_DISABLED);
    }
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "All slots disabled, returning error");
    return DRIVER_CCID_ERROR_NO_SLOTS_AVAILABLE;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                      GWEN_BUFFER *buf) {
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  DBG_DEBUG(LCD_Reader_GetLogger(r),
            "Requesting information about reader \"%08x\"",
            LCD_Reader_GetReaderId(r));

  DBG_WARN(LCD_Reader_GetLogger(r),
           "ReaderInfo() not yet supported for CCID drivers");

  return DRIVER_CCID_ERROR_NOT_SUPPORTED;
}



GWEN_TYPE_UINT32 DriverCCID_ConnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  assert(r);
  slotList=LCD_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LCD_Slot_List_First(slotList);
  while(sl) {
    long retval;

    retval=dct->createChannelFn(LCD_Slot_GetSlotNum(sl),
                                LCD_Reader_GetPort(r));
    if (retval==0) {
      if (!LCD_Driver_ConnectSlot(d, sl)) {
        oks++;
      }
    }
    else {
      DBG_ERROR(LCD_Reader_GetLogger(r),
		"CCID error on createChannel %d/%d: %ld",
		LCD_Slot_GetSlotNum(sl),
		LCD_Reader_GetPort(r),
		retval);
    }
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Could not connect any slot");
    return DRIVER_CCID_ERROR_NO_SLOTS_CONNECTED;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverCCID_DisconnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_CCID *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_CCID, d);
  assert(dct);

  assert(r);
  slotList=LCD_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LCD_Slot_List_First(slotList);
  while(sl) {
    long retval;

    if (!LCD_Driver_DisconnectSlot(d, sl))
      oks++;
    retval=dct->closeChannelFn(LCD_Slot_GetSlotNum(sl));
    if (retval!=0) {
      DBG_ERROR(LCD_Reader_GetLogger(r),
                "Error closing channel %d (%d)",
                LCD_Slot_GetSlotNum(sl), (int)retval);
    }
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "Could not connect any slot");
    return DRIVER_CCID_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}



void log_msg(const int priority, const char *fmt, ...) {
  char msgBuf[512];
  va_list argptr;

  va_start(argptr, fmt);
  vsnprintf(msgBuf, sizeof(msgBuf), fmt, argptr);
  va_end(argptr);

  switch(priority) {
  case 1: /* PCSC_LOG_INFO */
    DBG_INFO(0, "%s", msgBuf);
    break;
  case 2: /* PCSC_LOG_ERROR */
  case 3: /* PCSC_LOG_CRITICAL */
    DBG_ERROR(0, "%s", msgBuf);
    break;
  case 0: /* PCSC_LOG_DEBUG */
  default:
    DBG_DEBUG(0, "%s", msgBuf);
    break;
  } /* switch */
}



char *pcsc_stringify_error(long x) {
  static char errbuf[256];

  snprintf(errbuf, sizeof(errbuf),
           "PC/SC-Error: %08lx",
           (unsigned long) x);
  return errbuf;
}


