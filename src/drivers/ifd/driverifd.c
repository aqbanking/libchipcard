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


#include "driverifd_p.h"
#include "readerifd_l.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard/chipcard.h>

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>



GWEN_INHERIT(LCD_DRIVER, DRIVER_IFD)



/* possible values:
 *   0: undetermined
 *   1: generic ccid driver pre 1.1.0
 *  -1: generic ccid driver 1.1.0 or higher
 */
static int lc_driver__ccid_pre_1_1_0=0;


LCD_DRIVER *DriverIFD_new(int argc, char **argv) {
  DRIVER_IFD *dct;
  LCD_DRIVER *d;
  int rv;

  d=LCD_Driver_new();
  if (!d) {
    DBG_ERROR(0, "Could not create driver, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(DRIVER_IFD, dct);
  GWEN_INHERIT_SETDATA(LCD_DRIVER, DRIVER_IFD,
                       d, dct,
                       DriverIFD_freeData);

  LCD_Driver_SetSendApduFn(d, DriverIFD_SendAPDU);
  LCD_Driver_SetConnectSlotFn(d, DriverIFD_ConnectSlot);
  LCD_Driver_SetDisconnectSlotFn(d, DriverIFD_DisconnectSlot);
  LCD_Driver_SetConnectReaderFn(d, DriverIFD_ConnectReader);
  LCD_Driver_SetDisconnectReaderFn(d, DriverIFD_DisconnectReader);
  LCD_Driver_SetResetSlotFn(d, DriverIFD_ResetSlot);
  LCD_Driver_SetReaderStatusFn(d, DriverIFD_ReaderStatus);
  LCD_Driver_SetReaderInfoFn(d, DriverIFD_ReaderInfo);
  LCD_Driver_SetExtendReaderFn(d, DriverIFD_ExtendReader);
  LCD_Driver_SetGetErrorTextFn(d, DriverIFD_GetErrorText);

  rv=LCD_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LCD_Driver_free(d);
    return 0;
  }

  dct->ifdVersion=0;

  return d;
}



void DriverIFD_free(DRIVER_IFD *dct) {
  if (dct) {
    GWEN_LibLoader_free(dct->libLoader);
    GWEN_FREE_OBJECT(dct);
  }
}



void GWENHYWFAR_CB DriverIFD_freeData(void *bp, void *p) {
  DRIVER_IFD *dct;

  dct=(DRIVER_IFD*)p;
  DriverIFD_free(dct);
}


int DriverIFD_Start(LCD_DRIVER *d) {
  int err;
  DRIVER_IFD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  GWEN_LibLoader_free(dct->libLoader);
  dct->libLoader=GWEN_LibLoader_new();
  err=GWEN_LibLoader_OpenLibrary(dct->libLoader,
                                 LCD_Driver_GetLibraryFile(d));
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Loading library", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHCreateChannel",
                             (void*)&dct->createChannelFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHCloseChannel",
                             (void*)&dct->closeChannelFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHPowerICC",
                             (void*)&dct->powerIccFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHTransmitToICC",
                             (void*)&dct->transmitFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHCreateChannelByName",
                             (void*)&dct->createChannelByNameFn);
  if (!err) {
    DBG_NOTICE(0, "Detected IFD version 3");
    err=GWEN_LibLoader_Resolve(dct->libLoader,
                               "IFDHControl",
                               (void*)&dct->control3Fn);
    if (err) {
      DBG_ERROR_ERR(0, err);
      GWEN_LibLoader_CloseLibrary(dct->libLoader);
      if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
        DBG_ERROR(0, "Error communicating with the server");
        return -1;
      }
      LCD_Driver_Disconnect(d);
      return -1;
    }
    dct->ifdVersion=3;
  }
  else {
    err=GWEN_LibLoader_Resolve(dct->libLoader,
                               "IFDHControl",
                               (void*)&dct->control2Fn);
    if (err) {
      DBG_ERROR_ERR(0, err);
      GWEN_LibLoader_CloseLibrary(dct->libLoader);
      if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
        DBG_ERROR(0, "Error communicating with the server");
        return -1;
      }
      LCD_Driver_Disconnect(d);
      return -1;
    }
    DBG_NOTICE(0, "Detected IFD version 2");
    dct->ifdVersion=2;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHICCPresence",
                             (void*)&dct->presenceFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
                             "IFDHGetCapabilities",
                             (void*)&dct->getCapsFn);
  if (err) {
    DBG_ERROR_ERR(0, err);
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    if (LCD_Driver_Connect(d, LC_ERROR_GENERIC, "Resolving symbols", 0, 0)) {
      DBG_ERROR(0, "Error communicating with the server");
      return -1;
    }
    LCD_Driver_Disconnect(d);
    return -1;
  }

  err=GWEN_LibLoader_Resolve(dct->libLoader,
			     "IFDHSetProtocolParameters",
			     (void*)&dct->setProtoFn);
  if (err) {
    DBG_WARN(0, "IFDHSetProtocolParameters() not found (%d)", err);
    /* ignore error */
  }

  /* send status report to server */
  if (LCD_Driver_Connect(d, 0, "Library loaded", 0, 0)) {
    DBG_ERROR(0, "Error communicating with the server");
    GWEN_LibLoader_CloseLibrary(dct->libLoader);
    return -1;
  }
  DBG_INFO(0, "Connected.");
  return 0;
}



const char *DriverIFD_GetErrorText(LCD_DRIVER *d, uint32_t err) {
  const char *s;
  DRIVER_IFD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
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
  default:
    s="Unknow error code";
  };
  return s;
}



int DriverIFD_ExtractProtocolInfo(unsigned char *atr,
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



uint32_t DriverIFD_SendAPDU(LCD_DRIVER *d,
			    int toReader,
			    LCD_READER *r,
			    LCD_SLOT *slot,
			    const unsigned char *apdu,
			    unsigned int apdulen,
			    unsigned char *buffer,
			    int *bufferlen){
  long retval;
  DWORD tmplen;
  DRIVER_IFD *dct;
  const char *lg;
  int bufferLenAtStart;

  lg=LCD_Reader_GetLogger(r);

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  assert(apdu);
  assert(apdulen>3);
  assert(buffer);

  tmplen=*bufferlen;
  bufferLenAtStart=*bufferlen;

  if (toReader) {
    if (dct->ifdVersion>=3) {
      int feature;
      uint32_t controlCode;
  
      feature=apdu[0];
      controlCode=
          (apdu[1]<<24)+
          (apdu[2]<<16)+
          (apdu[3]<<8)+
        apdu[4];
      if (feature && controlCode==0)
        controlCode=ReaderIFD_GetFeatureCode(r, feature);
    
      if (controlCode==0) {
        DBG_ERROR(lg, "Bad control code");
        return LC_ERROR_INVALID;
      }
      assert(apdulen>8);
      DBG_INFO(lg,
               "Sending command to reader (ControlCode=%08x):", controlCode);
      GWEN_Text_LogString((const char*)apdu+5,
			  apdulen-5, lg, GWEN_LoggerLevel_Info);
      retval=dct->control3Fn(LCD_Slot_GetSlotNum(slot),
                             controlCode,
                             apdu+5,
                             apdulen-5,
                             buffer,
                             *bufferlen,
                             &tmplen);
    }
    else {
      assert(apdulen>3);
      DBG_INFO(lg, "Sending command to reader:");
      GWEN_Text_LogString((const char*)apdu, apdulen, lg,
                          GWEN_LoggerLevel_Info);
      retval=dct->control2Fn(LCD_Slot_GetSlotNum(slot),
                             apdu,
                             apdulen,
                             buffer,
                             &tmplen);
    }

    *bufferlen=tmplen;
    if (retval==0 && *bufferlen>3) {
      /* special treatment */
      int rlen;

      rlen=buffer[2];
      if ((rlen+4)==*bufferlen) {
        unsigned char *pSrc;
        unsigned char *pDst;

        /* real response is burried within the complete response */
        DBG_INFO(lg, "Found response within response, extracting");
        GWEN_Text_LogString((const char*)buffer, *bufferlen, lg,
                            GWEN_LoggerLevel_Info);
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
             "Sending command to card (bufferlen=%d):", (unsigned int)tmplen);
    GWEN_Text_LogString((const char*)apdu, apdulen, lg, GWEN_LoggerLevel_Info);
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
    GWEN_Text_LogString((const char*)buffer, *bufferlen, lg, GWEN_LoggerLevel_Info);
  }

  if (retval!=0) {
    DBG_ERROR(lg,
              "CCID error on \"CCIDTransmit/Control\": %ld",
              retval);
    return retval;
  }

  if (LCD_Slot_GetProtocolInfo(slot)!=0 ||
      !(LCD_Reader_GetReaderFlags(r) & LC_READER_FLAGS_NO_MEMORY_SW)) {
    if (tmplen<2 || tmplen>258) {
      DBG_ERROR(lg,
		"Bad response size (%d)", (unsigned int)tmplen);
      return LC_ERROR_BAD_RESPONSE;
    }
  }
  else {
    uint8_t sw1, sw2;

    /* preset: no precise diagnostics plus special sw2*/
    sw1=0x64;
    sw2=0xff;

    DBG_INFO(lg, "Manufacturing SW1/SW2 for memory cards");
    if (apdu[0]==0x00 && apdu[1]==0xb0) {
      /* read binary */
      if ((apdu[4]==0x00 && tmplen) ||
	  (apdu[4]>=tmplen)) {
	if (tmplen) {
	  sw1=0x90;
	  sw2=0x90;
	}
      }
    }
    else if (apdu[0]==0x00 && apdu[1]==0xd0) {
      /* write binary */
      sw1=0x90;
      sw2=0x00;
    }

    if (tmplen+2<bufferLenAtStart) {
      buffer[tmplen]=0x90;
      buffer[tmplen+1]=0x00;
      tmplen+=2;
      *bufferlen=tmplen;
    }
    else {
      DBG_ERROR(lg, "Buffer too small (%d<%d)",
		bufferLenAtStart, (int)(tmplen+2));
      return LC_ERROR_BUFFER_OVERFLOW;
    }
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



long DriverIFD__ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  DWORD atrLen;
  DRIVER_IFD *dct;
  LCD_READER *r;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
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
  if (atrLen>2) {
    /* TODO: more elaborate checks of the ATR to catch other cards which
     * report different ATRs at every other PowerUp */
    if (atrBuffer[0]==0x3b &&
        atrBuffer[1]==0xef) {
      unsigned char atrBuffer2[300];
      DWORD atrLen2;
      DBG_NOTICE(0, "Suspicious ATR, rereading it");
      atrLen2=sizeof(atrBuffer2);
      retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                             CCID_POWER_UP,
                             atrBuffer2,
			     &atrLen2);
      if (atrLen2>2) {
        if (atrBuffer2[1]!=0xef) {
          DBG_NOTICE(0, "Got another ATR in second attempt, using that");
          atrLen=atrLen2;
          memmove(atrBuffer, atrBuffer2, atrLen2);
        }
      }
    }

    if (atrLen) {
      GWEN_BUFFER *abuf;

      abuf=GWEN_Buffer_new(0, atrLen, 0, 1);
      GWEN_Buffer_AppendBytes(abuf, (const char*)atrBuffer, atrLen);
      LCD_Slot_SetAtr(sl, abuf);
    }
  }

  if (retval==CCID_ICC_PRESENT || retval==0) {
    int proto;

    DBG_INFO(LCD_Reader_GetLogger(r), "Card present.");
    LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_INSERTED);
    if (retval==0) {
      LCD_Slot_AddStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
    }

    proto=DriverIFD_ExtractProtocolInfo(atrBuffer, atrLen);
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
    return retval;
  }
  return 0;
}



uint32_t DriverIFD_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  int i;
  DRIVER_IFD *dct;
  long retval;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  retval=DriverIFD__ConnectSlot(d, sl);
  if (retval!=CCID_ERROR_POWER_ACTION) {
    DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
	     "CCID: Result is not POWER_ACTION (%d)",
	     (int)retval);
    return retval;
  }

  if (dct->setProtoFn==NULL)
    return retval;

  /* try protocols 0 through 15 */
  for (i=0; i<16; i++) {
    /* set protocol bt don't change PTS */
    retval=dct->setProtoFn(LCD_Slot_GetSlotNum(sl),
			   i, 0, 0, 0, 0);
    if (retval) {
      DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
	       "CCID: Error setting protocol %d (%d)",
	       i, (int)retval);
    }
    else {
      retval=DriverIFD__ConnectSlot(d, sl);
      if (retval!=CCID_ERROR_POWER_ACTION) {
	DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
		 "CCID: Result is not POWER_ACTION (%d, proto=%d)",
		 (int)retval, i);
	return retval;
      }
    }
  }

  /* assuming no card inserted */
  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
	   "Assuming no card is inserted");
  return 0;
}



uint32_t DriverIFD_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  DWORD atrLen;
  DRIVER_IFD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Disconnecting slot %d", LCD_Slot_GetSlotNum(sl));

  LCD_Slot_SetAtr(sl, 0);
  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                         CCID_POWER_DOWN,
                         atrBuffer,
			 &atrLen);
  if (atrLen) {
    DBG_DEBUG(0, "Received ATR:");
    GWEN_Text_LogString((const char*)atrBuffer,
                        atrLen, 0, GWEN_LoggerLevel_Debug);
  }
  if (retval!=0) {
    DBG_NOTICE(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
               "CCID: Soft error %d", (int)retval);
  }

  LCD_Slot_SubStatus(sl, LCD_SLOT_STATUS_CARD_CONNECTED);
  return 0;
}



uint32_t DriverIFD_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl) {
  long retval;
  unsigned char atrBuffer[300];
  DWORD atrLen;
  DRIVER_IFD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);


  DBG_INFO(LCD_Reader_GetLogger(LCD_Slot_GetReader(sl)),
           "Resetting slot %d", LCD_Slot_GetSlotNum(sl));
  atrLen=sizeof(atrBuffer);
  retval=dct->powerIccFn(LCD_Slot_GetSlotNum(sl),
                         CCID_RESET,
                         atrBuffer,
			 &atrLen);
  if (atrLen) {
    DBG_DEBUG(0, "Received ATR:");
    GWEN_Text_LogString((const char*)atrBuffer,
                        atrLen, 0, GWEN_LoggerLevel_Debug);
  }
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



uint32_t DriverIFD_ReaderStatus(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slList;
  long retval;
  DRIVER_IFD *dct;
  int oks;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
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
    if (retval==CCID_COMMUNICATION_ERROR) {
      int i;

      /* handle special case where reader reports BUSY error
       * for a warmup period, just try it three times */
      for (i=0; i<3; i++) {
        retval=dct->presenceFn(LCD_Slot_GetSlotNum(sl));
        if (retval!=CCID_COMMUNICATION_ERROR)
          break;
        GWEN_Socket_Select(0, 0, 0, 500);
      }
    }

    if (retval==CCID_ICC_PRESENT) {
      oks++;
      if (!(LCD_Slot_GetLastStatus(sl) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
        /* card is not connected but was not inserted before, try to
         * connect now */
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
      if (LCD_Slot_GetLastStatus(sl) & LCD_SLOT_STATUS_CARD_CONNECTED)
        /* was connected last time we checked, so reset the ATR */
        LCD_Slot_SetAtr(sl, 0);
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
      LCD_Slot_SetAtr(sl, 0);
    }
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  if (!oks) {
    DBG_ERROR(LCD_Reader_GetLogger(r),
              "All slots disabled, returning error");
    return LC_ERROR_NO_SLOTS_AVAILABLE;
  }
  return 0;
}



uint32_t DriverIFD_ReaderInfo(LCD_DRIVER *d, LCD_READER *r, GWEN_BUFFER *buf){
  DRIVER_IFD *dct;
  const char *lg;
  long retval;
  unsigned char buffer[256];
  DWORD bufferlen;
  int cnt;
  int i;
  PCSC_TLV_STRUCTURE *tlv;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  lg=LCD_Reader_GetLogger(r);

  if (dct->ifdVersion>=3) {
    DBG_DEBUG(LCD_Reader_GetLogger(r),
              "Requesting feature list of reader \"%08x\"",
              LCD_Reader_GetReaderId(r));
  
    bufferlen=sizeof(buffer);
    retval=dct->control3Fn(0, /* lun */
                           CM_IOCTL_GET_FEATURE_REQUEST,
                           NULL, 0,
                           buffer,
                           bufferlen,
			   &bufferlen);
    if (retval==0) {
      DBG_INFO(lg, "Response:");
      GWEN_Text_LogString((const char*)buffer, bufferlen, lg,
                          GWEN_LoggerLevel_Info);
    }
    else {
      DBG_ERROR(lg,
                "CCID error on \"CCIDTransmit/Control\": %ld", retval);
      return retval;
    }
  
    if (bufferlen % sizeof(PCSC_TLV_STRUCTURE)) {
      DBG_ERROR(lg,
		"Inconsistent size of response (%d)",
		(unsigned int)bufferlen);
      return LC_ERROR_BAD_RESPONSE;
    }
  
    cnt=bufferlen/sizeof(PCSC_TLV_STRUCTURE);
    tlv=(PCSC_TLV_STRUCTURE*)buffer;
    for (i=0; i<cnt; i++) {
      uint32_t v;
  
      v=tlv[i].value;
#ifdef LC_ENDIAN_LITTLE
      /* only translate control codes for generic ccid driver >=1.1.0 */
      if (lc_driver__ccid_pre_1_1_0==-1)
	v=((v & 0xff000000)>>24) |
	  ((v & 0x00ff0000)>>8) |
	  ((v & 0x0000ff00)<<8) |
	  ((v & 0x000000ff)<<24);
#endif
      DBG_NOTICE(lg, "TLV: %d (%08x)", tlv[i].tag, v);
      ReaderIFD_SetFeatureCode(r, tlv[i].tag, v);
    }

    if (GWEN_Buffer_GetUsedBytes(buf))
      GWEN_Buffer_AppendByte(buf, ';');
    GWEN_Buffer_AppendString(buf, "unit0=\"icc1\"");
    if (ReaderIFD_GetFeatureCode(r, FEATURE_VERIFY_PIN_DIRECT)) {
      GWEN_Buffer_AppendString(buf, ";unit1=\"KEYBOARD\"");
    }
  }

  return 0;
}



uint32_t DriverIFD_ConnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_IFD *dct;
  const char *devPath;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
  assert(dct);

  assert(r);
  slotList=LCD_Reader_GetSlots(r);
  assert(slotList);
  oks=0;

  devPath=LCD_Reader_GetDevicePath(r);
  if (devPath && dct->createChannelByNameFn==0) {
    DBG_ERROR(0, "Device path given but function IFDHCreateChannelByName "
              "is not available, can't start reader");
    return LC_ERROR_NO_SLOTS_CONNECTED;
  }
  sl=LCD_Slot_List_First(slotList);
  while(sl) {
    long retval;

    if (devPath) {
      DBG_NOTICE(0, "Creating channel %d/%s",
                 LCD_Slot_GetSlotNum(sl), devPath);
      retval=dct->createChannelByNameFn(LCD_Slot_GetSlotNum(sl),
                                        devPath);
    }
    else {
      DBG_NOTICE(0, "Creating channel %d/%d",
                 LCD_Slot_GetSlotNum(sl), LCD_Reader_GetPort(r));
      retval=dct->createChannelFn(LCD_Slot_GetSlotNum(sl),
                                  LCD_Reader_GetPort(r));
    }

    if (retval==CCID_COMMUNICATION_ERROR) {
      int i;

      /* retry for 3 times */
      for (i=0; i<3; i++) {
        retval=dct->createChannelFn(LCD_Slot_GetSlotNum(sl),
                                    LCD_Reader_GetPort(r));
        if (retval!=CCID_COMMUNICATION_ERROR)
          break;
        GWEN_Socket_Select(0, 0, 0, 500);
      }
    }
    if (retval==0) {
      oks++;
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
    return LC_ERROR_NO_SLOTS_CONNECTED;
  }

  return 0;
}



uint32_t DriverIFD_DisconnectReader(LCD_DRIVER *d, LCD_READER *r) {
  LCD_SLOT *sl;
  LCD_SLOT_LIST *slotList;
  unsigned int oks;
  DRIVER_IFD *dct;

  assert(d);
  dct=GWEN_INHERIT_GETDATA(LCD_DRIVER, DRIVER_IFD, d);
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
    DBG_NOTICE(0, "Closing channel %d", LCD_Slot_GetSlotNum(sl));
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
    return LC_ERROR_NO_SLOTS_DISCONNECTED;
  }
  return 0;
}



int DriverIFD_ExtendReader(LCD_DRIVER *d, LCD_READER *r) {
  ReaderIFD_Extend(r);
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
    DBG_INFO(0, "PCSC(%d): %s", priority, msgBuf);
    break;
  case 2: /* PCSC_LOG_ERROR */
  case 3: /* PCSC_LOG_CRITICAL */
    DBG_ERROR(0, "PCSC(%d): %s", priority, msgBuf);
    break;
  case 0: /* PCSC_LOG_DEBUG */
  default:
    DBG_DEBUG(0, "PCSC(%d): %s", priority, msgBuf);
    break;
  } /* switch */

  DriverIFD__checkMsg(priority, msgBuf);
}



void debug_msg(const int priority, const char *fmt, ...) {
  char msgBuf[1024];
  va_list argptr;

  va_start(argptr, fmt);
  vsnprintf(msgBuf, sizeof(msgBuf), fmt, argptr);
  va_end(argptr);

  switch(priority) {
  case 1: /* PCSC_LOG_INFO */
    DBG_INFO(0, "PCSC: %s", msgBuf);
    break;
  case 2: /* PCSC_LOG_ERROR */
  case 3: /* PCSC_LOG_CRITICAL */
    DBG_ERROR(0, "PCSC: %s", msgBuf);
    break;
  case 0: /* PCSC_LOG_DEBUG */
  default:
    DBG_DEBUG(0, "PCSC: %s", msgBuf);
    break;
  } /* switch */

  DriverIFD__checkMsg(priority, msgBuf);
}



void log_xxd(const int priority, const char *msg,
             const unsigned char *buffer,
	     const int size) {
  GWEN_LOGGER_LEVEL lv;

  switch(priority) {
  case 1: /* PCSC_LOG_INFO */
    lv=GWEN_LoggerLevel_Info;
    DBG_INFO(0, "PCSC %d: %s", priority, msg);
    break;
  case 2: /* PCSC_LOG_ERROR */
  case 3: /* PCSC_LOG_CRITICAL */
    lv=GWEN_LoggerLevel_Error;
    DBG_ERROR(0, "PCSC %d: %s", priority, msg);
    break;
  case 0: /* PCSC_LOG_DEBUG */
  default:
    lv=GWEN_LoggerLevel_Debug;
    DBG_DEBUG(0, "PCSC %d: %s", priority, msg);
    break;
  } /* switch */

  GWEN_Text_LogString((const char*) buffer, size, 0, lv);

}



char *pcsc_stringify_error(long x) {
  static char errbuf[256];

  snprintf(errbuf, sizeof(errbuf),
           "PC/SC-Error: %08lx",
           (unsigned long) x);
  return errbuf;
}



void DriverIFD__checkMsg(int priority, const char *msg) {
  if (lc_driver__ccid_pre_1_1_0==0 && priority>0) {
    if (GWEN_Text_ComparePattern((const char*)msg,
                                 "*ProductString: Generic CCID driver*",
                                 0)!=-1) {
      const char *p;

      p=msg;
      p=strrchr(p, 'v');
      if (p && isdigit(p[1])) {
	int vmajor=0;
        int vminor=0;
        int vpatchlevel=0;

        p++;
        /* read vmajor */
        while (*p && isdigit(*p)) {
          vmajor=(vmajor*10)+(*p-'0');
          p++;
        }
        if (*p=='.') {
          p++;
          /* read vminor */
          while (*p && isdigit(*p)) {
            vminor=(vminor*10)+(*p-'0');
            p++;
          }
          if (*p=='.') {
            /* read vpatchlevel */
            p++;
            while (*p && isdigit(*p)) {
              vpatchlevel=(vpatchlevel*10)+(*p-'0');
              p++;
            }
            DBG_NOTICE(0, "Detected Generic CCID driver (%d.%d.%d)",
                       vmajor, vminor, vpatchlevel);

            /* check whether we have to reverse the translation of the
             * control codes
             */
            if (!(vmajor>1 || (vmajor==1 && vminor>0))) {
              DBG_WARN(0,
                       "Old Generic CCID driver, "
                       "will not translate control codes");
              lc_driver__ccid_pre_1_1_0=1;
            }
            else
              lc_driver__ccid_pre_1_1_0=-1;
          }
        }
      }
      else {
	/* grr... newer drivers don't show the version string in this message,
	 * so we assume this *is* a newer driver
	 */
	DBG_NOTICE(0, "Message doesn't show the version, assuming newer driver version (post 1.1.0)");
	lc_driver__ccid_pre_1_1_0=-1;
      }
    } /* if generic ccid driver */
  } /* if driver type still undetermined */
}



