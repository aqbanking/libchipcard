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


#ifndef CHIPCARD_DRIVER_DRIVER_H
#define CHIPCARD_DRIVER_DRIVER_H


typedef struct LC_DRIVER LC_DRIVER;

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>

#include <chipcard2-server/driver/reader.h>

#define LC_DRIVER_IPC_MAXWORK 256


typedef enum {
  LC_DriverCheckArgsResultOk=0,
  LC_DriverCheckArgsResultError,
  LC_DriverCheckArgsResultVersion,
  LC_DriverCheckArgsResultHelp
} LC_DRIVER_CHECKARGS_RESULT;


GWEN_INHERIT_FUNCTION_DEFS(LC_DRIVER);


typedef GWEN_TYPE_UINT32 (*LC_DRIVER_SENDAPDU_FN)(LC_DRIVER *d,
                                                  int toReader,
                                                  LC_READER *r,
                                                  LC_SLOT *slot,
                                                  const unsigned char *apdu,
                                                  unsigned int apdulen,
                                                  unsigned char *buffer,
                                                  int *bufferlen);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_CONNECTSLOT_FN)(LC_DRIVER *d,
                                                     LC_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_CONNECTREADER_FN)(LC_DRIVER *d,
                                                       LC_READER *r);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_DISCONNECTSLOT_FN)(LC_DRIVER *d,
                                                        LC_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_DISCONNECTREADER_FN)(LC_DRIVER *d,
                                                          LC_READER *r);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_RESETSLOT_FN)(LC_DRIVER *d,
                                                   LC_SLOT *sl);
typedef GWEN_TYPE_UINT32 (*LC_DRIVER_READERSTATUS_FN)(LC_DRIVER *d,
                                                      LC_READER *r);

typedef GWEN_TYPE_UINT32 (*LC_DRIVER_READERINFO_FN)(LC_DRIVER *d,
                                                    LC_READER *r,
                                                    GWEN_BUFFER *buf);
typedef LC_READER* (*LC_DRIVER_CREATEREADER_FN)(LC_DRIVER *d,
                                                GWEN_TYPE_UINT32 readerId,
                                                const char *name,
                                                int port,
                                                unsigned int slots,
                                                GWEN_TYPE_UINT32 flags);


typedef const char* (*LC_DRIVER_GETERRORTEXT_FN)(LC_DRIVER *d,
                                                 GWEN_TYPE_UINT32 err);



void LC_Driver_Usage(const char *prgName);

LC_DRIVER *LC_Driver_new();
void LC_Driver_free(LC_DRIVER *d);

int LC_Driver_Init(LC_DRIVER *d, int argc, char **argv);

const char *LC_Driver_GetDriverDataDir(const LC_DRIVER *d);
const char *LC_Driver_GetLibraryFile(const LC_DRIVER *d);
const char *LC_Driver_GetDriverId(const LC_DRIVER *d);

int LC_Driver_Test(LC_DRIVER *d);
int LC_Driver_IsTestMode(const LC_DRIVER *d);

GWEN_TYPE_UINT32 LC_Driver_SendCommand(LC_DRIVER *d,
                                       GWEN_DB_NODE *dbCommand);
int LC_Driver_SendResponse(LC_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand);

int LC_Driver_SendResult(LC_DRIVER *d,
                         GWEN_TYPE_UINT32 rid,
                         const char *name,
                         const char *code,
                         const char *text);

int LC_Driver_RemoveCommand(LC_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound);

GWEN_TYPE_UINT32 LC_Driver_GetNextInRequest(LC_DRIVER *d);
GWEN_DB_NODE *LC_Driver_GetInRequestData(LC_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid);

int LC_Driver_Work(LC_DRIVER *d);


LC_READER_LIST *LC_Driver_GetReaders(const LC_DRIVER *d);
LC_READER *LC_Driver_FindReader(const LC_DRIVER *d);
LC_READER *LC_Driver_FindReaderByName(const LC_DRIVER *d, const char *name);
LC_READER *LC_Driver_FindReaderByPort(const LC_DRIVER *d, int port);
LC_READER *LC_Driver_FindReaderById(const LC_DRIVER *d, GWEN_TYPE_UINT32 id);
LC_READER *LC_Driver_FindReaderByDriversId(const LC_DRIVER *d,
                                           GWEN_TYPE_UINT32 id);

void LC_Driver_AddReader(LC_DRIVER *d, LC_READER *r);
void LC_Driver_DelReader(LC_DRIVER *d, LC_READER *r);

int LC_Driver_Connect(LC_DRIVER *d,
                      const char *code, const char *text);
void LC_Driver_Disconnect(LC_DRIVER *d);
int LC_Driver_SendStatusChangeNotification(LC_DRIVER *d,
                                           LC_SLOT *sl);

int LC_Driver_SendReaderErrorNotification(LC_DRIVER *d,
                                          LC_READER *r,
                                          const char *text);



GWEN_TYPE_UINT32 LC_Driver_SendAPDU(LC_DRIVER *d,
                                    int toReader,
                                    LC_READER *r,
                                    LC_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen);
GWEN_TYPE_UINT32 LC_Driver_ConnectSlot(LC_DRIVER *d, LC_SLOT *sl);
GWEN_TYPE_UINT32 LC_Driver_ConnectReader(LC_DRIVER *d, LC_READER *r);

GWEN_TYPE_UINT32 LC_Driver_DisconnectSlot(LC_DRIVER *d, LC_SLOT *sl);
GWEN_TYPE_UINT32 LC_Driver_DisconnectReader(LC_DRIVER *d, LC_READER *r);

GWEN_TYPE_UINT32 LC_Driver_ResetSlot(LC_DRIVER *d, LC_SLOT *sl);


GWEN_TYPE_UINT32 LC_Driver_ReaderStatus(LC_DRIVER *d, LC_READER *r);

const char *LC_Driver_GetErrorText(LC_DRIVER *d, GWEN_TYPE_UINT32 err);

GWEN_TYPE_UINT32 LC_Driver_ReaderInfo(LC_DRIVER *d,
                                      LC_READER *r,
                                      GWEN_BUFFER *buf);

LC_READER *LC_Driver_CreateReader(LC_DRIVER *d,
                                  GWEN_TYPE_UINT32 readerId,
                                  const char *name,
                                  int port,
                                  unsigned int slots,
                                  GWEN_TYPE_UINT32 flags);


void LC_Driver_SetSendApduFn(LC_DRIVER *d, LC_DRIVER_SENDAPDU_FN fn);
void LC_Driver_SetConnectSlotFn(LC_DRIVER *d, LC_DRIVER_CONNECTSLOT_FN fn);
void LC_Driver_SetDisconnectSlotFn(LC_DRIVER *d,
                                   LC_DRIVER_DISCONNECTSLOT_FN fn);
void LC_Driver_SetConnectReaderFn(LC_DRIVER *d,
                                  LC_DRIVER_CONNECTREADER_FN fn);
void LC_Driver_SetDisconnectReaderFn(LC_DRIVER *d,
                                     LC_DRIVER_DISCONNECTREADER_FN fn);
void LC_Driver_SetResetSlotFn(LC_DRIVER *d, LC_DRIVER_RESETSLOT_FN fn);
void LC_Driver_SetReaderStatusFn(LC_DRIVER *d,
                                 LC_DRIVER_READERSTATUS_FN fn);
void LC_Driver_SetReaderInfoFn(LC_DRIVER *d,
                               LC_DRIVER_READERINFO_FN fn);
void LC_Driver_SetCreateReaderFn(LC_DRIVER *d,
                                 LC_DRIVER_CREATEREADER_FN fn);
void LC_Driver_SetGetErrorTextFn(LC_DRIVER *d,
                                 LC_DRIVER_GETERRORTEXT_FN fn);






#endif /* CHIPCARD_DRIVER_DRIVER_H */




