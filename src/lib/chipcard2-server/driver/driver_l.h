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


#ifndef CHIPCARD_DRIVER_DRIVER_L_H
#define CHIPCARD_DRIVER_DRIVER_L_H


typedef struct LCD_DRIVER LCD_DRIVER;

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>

#include "reader_l.h"
#include <chipcard2/sharedstuff/pininfo.h>


#define LCD_DRIVER_IPC_MAXWORK 256


typedef enum {
  LCD_DriverCheckArgsResultOk=0,
  LCD_DriverCheckArgsResultError,
  LCD_DriverCheckArgsResultVersion,
  LCD_DriverCheckArgsResultHelp
} LCD_DRIVER_CHECKARGS_RESULT;


GWEN_INHERIT_FUNCTION_DEFS(LCD_DRIVER);


typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_SENDAPDU_FN)(LCD_DRIVER *d,
                                                  int toReader,
                                                  LCD_READER *r,
                                                  LCD_SLOT *slot,
                                                  const unsigned char *apdu,
                                                  unsigned int apdulen,
                                                  unsigned char *buffer,
                                                  int *bufferlen);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_CONNECTSLOT_FN)(LCD_DRIVER *d,
                                                     LCD_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_CONNECTREADER_FN)(LCD_DRIVER *d,
                                                       LCD_READER *r);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_DISCONNECTSLOT_FN)(LCD_DRIVER *d,
                                                        LCD_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_DISCONNECTREADER_FN)(LCD_DRIVER *d,
                                                          LCD_READER *r);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_RESETSLOT_FN)(LCD_DRIVER *d,
                                                   LCD_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_READERSTATUS_FN)(LCD_DRIVER *d,
                                                      LCD_READER *r);

typedef GWEN_TYPE_UINT32 (*LCD_DRIVER_READERINFO_FN)(LCD_DRIVER *d,
                                                    LCD_READER *r,
                                                    GWEN_BUFFER *buf);

typedef LCD_READER* (*LCD_DRIVER_CREATEREADER_FN)(LCD_DRIVER *d,
                                                  GWEN_TYPE_UINT32 readerId,
                                                  const char *name,
                                                  int port,
                                                  unsigned int slots,
                                                  GWEN_TYPE_UINT32 flags);

typedef const char* (*LCD_DRIVER_GETERRORTEXT_FN)(LCD_DRIVER *d,
                                                  GWEN_TYPE_UINT32 err);


typedef int (*LCD_DRIVER_HANDLEREQUEST_FN)(LCD_DRIVER *d,
                                           GWEN_TYPE_UINT32 rid,
                                           const char *name,
                                           GWEN_DB_NODE *dbReq);

typedef GWEN_TYPE_UINT32
  (*LCD_DRIVER_PERFORMVERIFICATION_FN)(LCD_DRIVER *d,
                                       LCD_READER *r,
                                       LCD_SLOT *slot,
                                       const LC_PININFO *pi,
                                       int *triesLeft);

typedef GWEN_TYPE_UINT32
  (*LCD_DRIVER_PERFORMMODIFICATION_FN)(LCD_DRIVER *d,
                                       LCD_READER *r,
                                       LCD_SLOT *slot,
                                       const LC_PININFO *pi,
                                       int *triesLeft);


void LCD_Driver_Usage(const char *prgName);

LCD_DRIVER *LCD_Driver_new();
void LCD_Driver_free(LCD_DRIVER *d);

int LCD_Driver_Init(LCD_DRIVER *d, int argc, char **argv);

const char *LCD_Driver_GetDriverDataDir(const LCD_DRIVER *d);
const char *LCD_Driver_GetLibraryFile(const LCD_DRIVER *d);
const char *LCD_Driver_GetDriverId(const LCD_DRIVER *d);

int LCD_Driver_Test(LCD_DRIVER *d);
int LCD_Driver_IsTestMode(const LCD_DRIVER *d);

GWEN_TYPE_UINT32 LCD_Driver_SendCommand(LCD_DRIVER *d,
                                       GWEN_DB_NODE *dbCommand);
int LCD_Driver_SendResponse(LCD_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand);

int LCD_Driver_SendResult(LCD_DRIVER *d,
                         GWEN_TYPE_UINT32 rid,
                         const char *name,
                         const char *code,
                         const char *text);

int LCD_Driver_RemoveCommand(LCD_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound);

GWEN_TYPE_UINT32 LCD_Driver_GetNextInRequest(LCD_DRIVER *d);
GWEN_DB_NODE *LCD_Driver_GetInRequestData(LCD_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid);

int LCD_Driver_Work(LCD_DRIVER *d);


LCD_READER_LIST *LCD_Driver_GetReaders(const LCD_DRIVER *d);
LCD_READER *LCD_Driver_FindReader(const LCD_DRIVER *d);
LCD_READER *LCD_Driver_FindReaderByName(const LCD_DRIVER *d, const char *name);
LCD_READER *LCD_Driver_FindReaderByPort(const LCD_DRIVER *d, int port);
LCD_READER *LCD_Driver_FindReaderById(const LCD_DRIVER *d, GWEN_TYPE_UINT32 id);
LCD_READER *LCD_Driver_FindReaderByDriversId(const LCD_DRIVER *d,
                                           GWEN_TYPE_UINT32 id);

void LCD_Driver_AddReader(LCD_DRIVER *d, LCD_READER *r);
void LCD_Driver_DelReader(LCD_DRIVER *d, LCD_READER *r);

int LCD_Driver_Connect(LCD_DRIVER *d,
                       const char *code, const char *text,
                       GWEN_TYPE_UINT32 dflagsValue,
                       GWEN_TYPE_UINT32 dflagsMask);
void LCD_Driver_Disconnect(LCD_DRIVER *d);
int LCD_Driver_SendStatusChangeNotification(LCD_DRIVER *d,
                                           LCD_SLOT *sl);

int LCD_Driver_SendReaderErrorNotification(LCD_DRIVER *d,
                                          LCD_READER *r,
                                          const char *text);



GWEN_TYPE_UINT32 LCD_Driver_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen);
GWEN_TYPE_UINT32 LCD_Driver_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 LCD_Driver_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 LCD_Driver_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 LCD_Driver_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 LCD_Driver_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);


GWEN_TYPE_UINT32 LCD_Driver_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);

const char *LCD_Driver_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);

GWEN_TYPE_UINT32 LCD_Driver_ReaderInfo(LCD_DRIVER *d,
                                      LCD_READER *r,
                                      GWEN_BUFFER *buf);

LCD_READER *LCD_Driver_CreateReader(LCD_DRIVER *d,
                                    GWEN_TYPE_UINT32 readerId,
                                    const char *name,
                                    int port,
                                    unsigned int slots,
                                    GWEN_TYPE_UINT32 flags);

GWEN_TYPE_UINT32 LCD_Driver_PerformVerification(LCD_DRIVER *d,
                                                LCD_READER *r,
                                                LCD_SLOT *slot,
                                                const LC_PININFO *pi,
                                                int *triesLeft);

GWEN_TYPE_UINT32 LCD_Driver_PerformModification(LCD_DRIVER *d,
                                                LCD_READER *r,
                                                LCD_SLOT *slot,
                                                const LC_PININFO *pi,
                                                int *triesLeft);


void LCD_Driver_SetSendApduFn(LCD_DRIVER *d, LCD_DRIVER_SENDAPDU_FN fn);
void LCD_Driver_SetConnectSlotFn(LCD_DRIVER *d, LCD_DRIVER_CONNECTSLOT_FN fn);
void LCD_Driver_SetDisconnectSlotFn(LCD_DRIVER *d,
                                   LCD_DRIVER_DISCONNECTSLOT_FN fn);
void LCD_Driver_SetConnectReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_CONNECTREADER_FN fn);
void LCD_Driver_SetDisconnectReaderFn(LCD_DRIVER *d,
                                     LCD_DRIVER_DISCONNECTREADER_FN fn);
void LCD_Driver_SetResetSlotFn(LCD_DRIVER *d, LCD_DRIVER_RESETSLOT_FN fn);
void LCD_Driver_SetReaderStatusFn(LCD_DRIVER *d,
                                 LCD_DRIVER_READERSTATUS_FN fn);
void LCD_Driver_SetReaderInfoFn(LCD_DRIVER *d,
                               LCD_DRIVER_READERINFO_FN fn);
void LCD_Driver_SetCreateReaderFn(LCD_DRIVER *d,
                                 LCD_DRIVER_CREATEREADER_FN fn);
void LCD_Driver_SetGetErrorTextFn(LCD_DRIVER *d,
                                  LCD_DRIVER_GETERRORTEXT_FN fn);
void LCD_Driver_SetPerformVerificationFn(LCD_DRIVER *d,
                                         LCD_DRIVER_PERFORMVERIFICATION_FN f);
void LCD_Driver_SetPerformModificationFn(LCD_DRIVER *d,
                                         LCD_DRIVER_PERFORMMODIFICATION_FN f);
void LCD_Driver_SetHandleRequestFn(LCD_DRIVER *d,
                                   LCD_DRIVER_HANDLEREQUEST_FN fn);






#endif /* CHIPCARD_DRIVER_DRIVER_L_H */




