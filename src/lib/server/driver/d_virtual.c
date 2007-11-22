/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driver.c 284 2006-09-22 00:53:00Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/* included by driver.c */




uint32_t LCD_Driver_SendAPDU(LCD_DRIVER *d,
                                     int toReader,
                                     LCD_READER *r,
                                     LCD_SLOT *slot,
                                     const unsigned char *apdu,
                                     unsigned int apdulen,
                                     unsigned char *buffer,
                                     int *bufferlen){
  assert(d);
  assert(d->sendApduFn);
  return d->sendApduFn(d, toReader, r, slot, apdu, apdulen,
                       buffer, bufferlen);
}



uint32_t LCD_Driver_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->connectSlotFn);
  return d->connectSlotFn(d, sl);
}



uint32_t LCD_Driver_ConnectReader(LCD_DRIVER *d, LCD_READER *r){
  uint32_t rv;

  assert(d);
  assert(d->connectReaderFn);
  rv=d->connectReaderFn(d, r);
  if (rv==0)
    LCD_Reader_AddStatus(r, LCD_READER_STATUS_UP);
  return rv;
}



uint32_t LCD_Driver_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->disconnectSlotFn);
  return d->disconnectSlotFn(d, sl);
}



uint32_t LCD_Driver_DisconnectReader(LCD_DRIVER *d, LCD_READER *r){
  uint32_t rv;

  assert(d);
  assert(d->disconnectReaderFn);
  rv=d->disconnectReaderFn(d, r);
  LCD_Reader_SubStatus(r, LCD_READER_STATUS_UP);
  return rv;
}



uint32_t LCD_Driver_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->resetSlotFn);
  return d->resetSlotFn(d, sl);
}



uint32_t LCD_Driver_ReaderStatus(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(d->readerStatusFn);
  return d->readerStatusFn(d, r);
}



uint32_t LCD_Driver_ReaderInfo(LCD_DRIVER *d,
                                      LCD_READER *r,
                                      GWEN_BUFFER *buf){
  assert(d);
  assert(d->readerInfoFn);
  return d->readerInfoFn(d, r, buf);
}



LCD_READER *LCD_Driver_CreateReader(LCD_DRIVER *d,
                                    uint32_t readerId,
                                    const char *name,
                                    int port,
                                    const char *devicePath,
                                    unsigned int slots,
                                    uint32_t flags){
  LCD_READER *r;

  assert(d);

  r=LCD_Reader_new(readerId, name, port, slots, flags);
  if (devicePath)
    LCD_Reader_SetDevicePath(r, devicePath);

  if (d->extendReaderFn!=0) {
    int rv;

    rv=d->extendReaderFn(d, r);
    if (rv) {
      DBG_ERROR(0, "Could not extend new reader (%d)", rv);
      LCD_Reader_free(r);
      return 0;
    }
  }

  LCD_Reader_SetDriversReaderId(r, ++(d->lastReaderId));

  return r;
}



const char *LCD_Driver_GetErrorText(LCD_DRIVER *d, uint32_t err){
  assert(d);
  if (err>=0x80000000)
    return LC_Error_toString(err & 0x7fffffff);
  assert(d->getErrorTextFn);
  return d->getErrorTextFn(d, err);
}








void LCD_Driver_SetSendApduFn(LCD_DRIVER *d, LCD_DRIVER_SENDAPDU_FN fn){
  assert(d);
  d->sendApduFn=fn;
}



void LCD_Driver_SetConnectSlotFn(LCD_DRIVER *d, LCD_DRIVER_CONNECTSLOT_FN fn){
  assert(d);
  d->connectSlotFn=fn;
}



void LCD_Driver_SetDisconnectSlotFn(LCD_DRIVER *d,
                                   LCD_DRIVER_DISCONNECTSLOT_FN fn){
  assert(d);
  d->disconnectSlotFn=fn;
}



void LCD_Driver_SetConnectReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_CONNECTREADER_FN fn){
  assert(d);
  d->connectReaderFn=fn;
}



void LCD_Driver_SetDisconnectReaderFn(LCD_DRIVER *d,
                                     LCD_DRIVER_DISCONNECTREADER_FN fn){
  assert(d);
  d->disconnectReaderFn=fn;
}



void LCD_Driver_SetResetSlotFn(LCD_DRIVER *d, LCD_DRIVER_RESETSLOT_FN fn){
  assert(d);
  d->resetSlotFn=fn;
}



void LCD_Driver_SetReaderStatusFn(LCD_DRIVER *d,
                                 LCD_DRIVER_READERSTATUS_FN fn){
  assert(d);
  d->readerStatusFn=fn;
}



void LCD_Driver_SetReaderInfoFn(LCD_DRIVER *d,
                               LCD_DRIVER_READERINFO_FN fn){
  assert(d);
  d->readerInfoFn=fn;
}



void LCD_Driver_SetExtendReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_EXTENDREADER_FN fn){
  assert(d);
  d->extendReaderFn=fn;
}



void LCD_Driver_SetGetErrorTextFn(LCD_DRIVER *d,
                                 LCD_DRIVER_GETERRORTEXT_FN fn){
  assert(d);
  d->getErrorTextFn=fn;
}



void
LCD_Driver_SetPerformVerificationFn(LCD_DRIVER *d,
                                    LCD_DRIVER_PERFORMVERIFICATION_FN fn){
  assert(d);
  d->performVerificationFn=fn;
}



void
LCD_Driver_SetPerformModificationFn(LCD_DRIVER *d,
                                    LCD_DRIVER_PERFORMMODIFICATION_FN fn){
  assert(d);
  d->performModificationFn=fn;
}



void LCD_Driver_SetHandleRequestFn(LCD_DRIVER *d,
                                   LCD_DRIVER_HANDLEREQUEST_FN fn) {
  assert(d);
  d->handleRequestFn=fn;
}




