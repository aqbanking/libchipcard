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

#include "driverifdold_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard2/chipcard2.h>

#include <unistd.h>

GWEN_INHERIT(LC_DRIVER, DRIVER_IFDOLD)



LC_DRIVER *DriverIFDOld_new(int argc, char **argv) {
  DRIVER_IFDOLD *dct;
  LC_DRIVER *d;
  int rv;

  d=LC_Driver_new();
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_IFDOLD, dct);
  GWEN_INHERIT_SETDATA(LC_DRIVER, DRIVER_IFDOLD,
                       d, dct,
                       DriverIFDOld_freeData);

  LC_Driver_SetSendApduFn(d, DriverIFDOld_SendAPDU);
  LC_Driver_SetConnectSlotFn(d, DriverIFDOld_ConnectSlot);
  LC_Driver_SetDisconnectSlotFn(d, DriverIFDOld_DisconnectSlot);
  LC_Driver_SetConnectReaderFn(d, DriverIFDOld_ConnectReader);
  LC_Driver_SetDisconnectReaderFn(d, DriverIFDOld_DisconnectReader);
  LC_Driver_SetResetSlotFn(d, DriverIFDOld_ResetSlot);
  LC_Driver_SetReaderStatusFn(d, DriverIFDOld_ReaderStatus);
  LC_Driver_SetReaderInfoFn(d, DriverIFDOld_ReaderInfo);
  LC_Driver_SetGetErrorTextFn(d, DriverIFDOld_GetErrorText);

  rv=LC_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LC_Driver_free(d);
    return 0;
  }

  return d;
}



void DriverIFDOld_free(DRIVER_IFDOLD *dct) {
  if (dct) {
    GWEN_LibLoader_free(dct->libLoader);
    GWEN_FREE_OBJECT(dct);
  }
}



void DriverIFDOld_freeData(void *bp, void *p) {
  DRIVER_IFDOLD *dct;

  dct=(DRIVER_IFDOLD*)p;
  DriverIFDOld_free(dct);
}


int DriverIFDOld_Start(LC_DRIVER *d) {
  GWEN_ERRORCODE err;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
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
                             "IFDHCreateChannel",
                             (void*)&dct->createChannelFn);
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
                             "IFDHCloseChannel",
                             (void*)&dct->closeChannelFn);
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
                             "IFDHPowerICC",
                             (void*)&dct->powerIccFn);
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
                             "IFDHTransmitToICC",
                             (void*)&dct->transmitFn);
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
                             "IFDHControl",
                             (void*)&dct->controlFn);
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
                             "IFDHICCPresence",
                             (void*)&dct->presenceFn);
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
                             "IFDHGetCapabilities",
                             (void*)&dct->getCapsFn);
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



const char *DriverIFDOld_GetErrorText(LC_DRIVER *d, GWEN_TYPE_UINT32 err) {
  const char *s;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  switch (err) {
  case IFD_NOT_SUPPORTED:
    s="Function not supported";
    break;
  case IFD_ICC_PRESENT:
    s="Card present";
    break;
  case IFD_ICC_NOT_PRESENT:
    s="Card not present";
    break;
  case DRIVER_IFDOLD_ERROR_BAD_RESPONSE:
    s="Bad response from IFD driver";
    break;
  case DRIVER_IFDOLD_ERROR_NO_SLOTS_CONNECTED:
    s="Could not connect any slot";
    break;
  case DRIVER_IFDOLD_ERROR_NO_SLOTS_DISCONNECTED:
    s="Could not disconnect any slot";
    break;
  case DRIVER_IFDOLD_ERROR_NO_SLOTS_AVAILABLE:
    s="No slot available";
    break;
  case DRIVER_IFDOLD_ERROR_NOT_SUPPORTED:
    s="Function not supported";
    break;
  default:
    s="Unknow error code";
  };
  return s;
}



int DriverIFDOld_ExtractProtocolInfo(unsigned char *atr,
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



GWEN_TYPE_UINT32 DriverIFDOld_SendAPDU(LC_DRIVER *d,
                                    int toReader,
                                    LC_READER *r,
                                    LC_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen){
  long retval;
  GWEN_TYPE_UINT32 tmplen;
  DRIVER_IFDOLD *dct;
  const char *lg;

  lg=LC_Reader_GetLogger(r);

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  assert(apdu);
  assert(apdulen>3);
  assert(buffer);

  tmplen=*bufferlen;

  if (toReader) {
    DBG_INFO(lg, "Sending command to reader:");
    GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevelInfo);
    retval=dct->controlFn(LC_Slot_GetSlotNum(slot),
			  apdu,
                          apdulen,
			  buffer,
                          &tmplen);
    *bufferlen=tmplen;
    if (retval==0 && *bufferlen>3) {
      /* special treatment */
      int rlen;

      GWEN_Text_LogString((const char*)buffer, *bufferlen, lg, GWEN_LoggerLevelInfo);
      rlen=buffer[2];
      if ((rlen+4)==*bufferlen) {
	unsigned char *pSrc;
	unsigned char *pDst;

	/* real response is burried within the complete response */
	DBG_INFO(lg, "Found response within response, extracting");
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
    txHeader.protocol=LC_Slot_GetProtocolInfo(slot);
    //txHeader.protocol=1;
    txHeader.length=sizeof(SCARD_IO_HEADER);
    rxHeader.length=sizeof(SCARD_IO_HEADER);
    retval=dct->transmitFn(LC_Slot_GetSlotNum(slot),
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
              "IFD error on \"IFDTransmit/Control\": %ld",
              retval);
    return retval;
  }
  if (tmplen<2 || tmplen>258) {
    DBG_ERROR(lg,
              "Bad response size (%d)", tmplen);
    return DRIVER_IFDOLD_ERROR_BAD_RESPONSE;
  }

  if ((unsigned char)buffer[tmplen-2]!=0x90) {
    DBG_NOTICE(lg,
               "IFD: Error: SW1=%02x, SW2=%02x "
               "(CLA=%02x, INS=%02x, P1=%02x, P2=%02x)",
               (unsigned char)buffer[tmplen-2],
               (unsigned char)buffer[tmplen-1],
               apdu[0], apdu[1], apdu[2], apdu[3]);
  }

  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_ConnectSlot(LC_DRIVER *d, LC_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_IFDOLD *dct;
  LC_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  r=LC_Slot_GetReader(sl);
  assert(r);

  DBG_INFO(LC_Reader_GetLogger(r),
	   "Connecting slot %d", LC_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  LC_Slot_SetAtr(sl, 0);
  retval=dct->powerIccFn(LC_Slot_GetSlotNum(sl),
                         IFD_POWER_UP,
                         atrBuffer,
			 &atrLen);
  if (retval==IFD_ICC_PRESENT || retval==0) {
    int proto;

    DBG_INFO(LC_Reader_GetLogger(r), "Card present.");
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    if (retval==0) {
      LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
    }

    if (atrLen) {
      GWEN_BUFFER *abuf;

      abuf=GWEN_Buffer_new(0, atrLen, 0, 1);
      GWEN_Buffer_AppendBytes(abuf, atrBuffer, atrLen);
      LC_Slot_SetAtr(sl, abuf);
    }

    proto=DriverIFDOld_ExtractProtocolInfo(atrBuffer, atrLen);
    if (proto==0) {
      DBG_NOTICE(LC_Reader_GetLogger(r),
                 "Protocol is 0, assuming memorycard");
      LC_Slot_SubFlags(sl, LC_SLOT_FLAGS_PROCESSORCARD);
    }
    else {
      LC_Slot_AddFlags(sl, LC_SLOT_FLAGS_PROCESSORCARD);
    }
    LC_Slot_SetProtocolInfo(sl, proto);
  }
  else if (retval==IFD_ICC_NOT_PRESENT) {
    DBG_NOTICE(LC_Reader_GetLogger(r), "No card inserted");
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
	       "IFD: Soft error %d", (int)retval);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
    if (retval==IFD_ERROR_POWER_ACTION) {
      DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
		 "IFD: Ignoring error, assuming missing card");
    }
    else
      return retval;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_DisconnectSlot(LC_DRIVER *d, LC_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);


  DBG_INFO(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
           "Disconnecting slot %d", LC_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LC_Slot_GetSlotNum(sl),
                         IFD_POWER_DOWN,
                         atrBuffer,
                         &atrLen);
  if (retval==IFD_NOT_SUPPORTED || retval==IFD_NOT_SUPPORTED) {
    atrLen=sizeof(atrBuffer);
    retval=dct->powerIccFn(LC_Slot_GetSlotNum(sl),
                           IFD_RESET,
                           atrBuffer,
                           &atrLen);
  }
  if (retval!=0) {
    DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
               "IFD: Soft error %d", (int)retval);
  }

  LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_ResetSlot(LC_DRIVER *d, LC_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  GWEN_TYPE_UINT32 atrLen;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);


  DBG_INFO(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
           "Resetting slot %d", LC_Slot_GetSlotNum(sl));

  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LC_Slot_GetSlotNum(sl),
                         IFD_RESET,
                         atrBuffer,
                         &atrLen);

  if (retval==IFD_ICC_PRESENT) {
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  }
  else if (retval==0) {
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
  }
  else {
    DBG_NOTICE(LC_Reader_GetLogger(LC_Slot_GetReader(sl)),
               "IFD: Soft error %d", (int)retval);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
    LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_ReaderStatus(LC_DRIVER *d, LC_READER *r) {
  LC_SLOT *sl;
  LC_SLOT_LIST *slList;
  long retval;
  DRIVER_IFDOLD *dct;
  int oks;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  DBG_DEBUG(LC_Reader_GetLogger(r),
            "Checking reader status for reader \"%08x\"",
            LC_Reader_GetReaderId(r));

  slList=LC_Reader_GetSlots(r);
  oks=0;
  sl=LC_Slot_List_First(slList);
  while(sl) {
    int slotNum;

    slotNum=LC_Slot_GetSlotNum(sl);
    retval=dct->presenceFn(LC_Slot_GetSlotNum(sl));
    if (retval==IFD_ICC_PRESENT) {
      oks++;
      if (!(LC_Slot_GetLastStatus(sl) & LC_SLOT_STATUS_CARD_CONNECTED)) {
        /* card is not connected, try to do so */
        if (LC_Driver_ConnectSlot(d, sl)) {
          LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
          LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
        }
      }
      else {
        LC_Slot_AddStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
      }
    }
    else if (retval==IFD_ICC_NOT_PRESENT){
      /* no card present, so there can be no card connected */
      oks++;
      LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
      LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
    }
    else {
      DBG_NOTICE(LC_Reader_GetLogger(r),
                 "IFD: Soft error %d", (int)retval);
      LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_CONNECTED);
      LC_Slot_SubStatus(sl, LC_SLOT_STATUS_CARD_INSERTED);
      LC_Slot_AddStatus(sl, LC_SLOT_STATUS_DISABLED);
    }
    sl=LC_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LC_Reader_GetLogger(r),
              "All slots disabled, returning error");
    return DRIVER_IFDOLD_ERROR_NO_SLOTS_AVAILABLE;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_ReaderInfo(LC_DRIVER *d, LC_READER *r,
                                      GWEN_BUFFER *buf) {
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  DBG_DEBUG(LC_Reader_GetLogger(r),
            "Requesting information about reader \"%08x\"",
            LC_Reader_GetReaderId(r));

  DBG_WARN(LC_Reader_GetLogger(r),
           "ReaderInfo() not yet supported for IFD drivers");

  return DRIVER_IFDOLD_ERROR_NOT_SUPPORTED;
}



GWEN_TYPE_UINT32 DriverIFDOld_ConnectReader(LC_DRIVER *d, LC_READER *r) {
  LC_SLOT *sl;
  LC_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  assert(r);
  slotList=LC_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LC_Slot_List_First(slotList);
  while(sl) {
    long retval;

    retval=dct->createChannelFn(LC_Slot_GetSlotNum(sl),
                                LC_Reader_GetPort(r));
    if (retval==0) {
      if (!LC_Driver_ConnectSlot(d, sl)) {
        oks++;
      }
    }
    else {
      DBG_ERROR(LC_Reader_GetLogger(r),
		"IFD error on createChannel %d/%d: %ld",
		LC_Slot_GetSlotNum(sl),
		LC_Reader_GetPort(r),
		retval);
    }
    sl=LC_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Could not connect any slot");
    return DRIVER_IFDOLD_ERROR_NO_SLOTS_CONNECTED;
  }
  return 0;
}



GWEN_TYPE_UINT32 DriverIFDOld_DisconnectReader(LC_DRIVER *d, LC_READER *r) {
  LC_SLOT *sl;
  LC_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_IFDOLD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LC_DRIVER, DRIVER_IFDOLD, d);
  assert(dct);

  assert(r);
  slotList=LC_Reader_GetSlots(r);
  assert(slotList);
  oks=0;
  sl=LC_Slot_List_First(slotList);
  while(sl) {
    long retval;

    if (!LC_Driver_DisconnectSlot(d, sl))
      oks++;
    retval=dct->closeChannelFn(LC_Slot_GetSlotNum(sl));
    if (retval!=0) {
      DBG_ERROR(LC_Reader_GetLogger(r),
                "Error closing channel %d (%d)",
                LC_Slot_GetSlotNum(sl), (int)retval);
    }
    sl=LC_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LC_Reader_GetLogger(r),
              "Could not connect any slot");
    return DRIVER_IFDOLD_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}




