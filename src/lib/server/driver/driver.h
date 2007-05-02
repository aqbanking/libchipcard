/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driver_l.h 321 2006-09-30 23:40:55Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_DRIVER_H
#define CHIPCARD_DRIVER_DRIVER_H

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>

#include <chipcard/chipcard.h>

typedef struct LCD_DRIVER LCD_DRIVER;
GWEN_INHERIT_FUNCTION_LIB_DEFS(LCD_DRIVER, CHIPCARD_API);

#include <chipcard/server/driver/reader.h>
#include <chipcard/sharedstuff/pininfo.h>


#define LCD_DRIVER_IPC_MAXWORK 256

#define LCD_DRIVER_ERROR_OFFSET 0x80000000


/** @name Prototypes For Virtual Functions
 *
 */
/*@{*/
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

typedef int (*LCD_DRIVER_EXTENDREADER_FN)(LCD_DRIVER *d, LCD_READER *r);


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
/*@}*/



/** @name Setters for Virtual Functions
 *
 */
/*@{*/
CHIPCARD_API
void LCD_Driver_SetSendApduFn(LCD_DRIVER *d, LCD_DRIVER_SENDAPDU_FN fn);

CHIPCARD_API
void LCD_Driver_SetConnectSlotFn(LCD_DRIVER *d, LCD_DRIVER_CONNECTSLOT_FN fn);

CHIPCARD_API
void LCD_Driver_SetDisconnectSlotFn(LCD_DRIVER *d,
                                   LCD_DRIVER_DISCONNECTSLOT_FN fn);

CHIPCARD_API
void LCD_Driver_SetConnectReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_CONNECTREADER_FN fn);

CHIPCARD_API
void LCD_Driver_SetDisconnectReaderFn(LCD_DRIVER *d,
                                     LCD_DRIVER_DISCONNECTREADER_FN fn);

CHIPCARD_API
void LCD_Driver_SetResetSlotFn(LCD_DRIVER *d, LCD_DRIVER_RESETSLOT_FN fn);

CHIPCARD_API
void LCD_Driver_SetReaderStatusFn(LCD_DRIVER *d,
                                 LCD_DRIVER_READERSTATUS_FN fn);

CHIPCARD_API
void LCD_Driver_SetReaderInfoFn(LCD_DRIVER *d,
                                LCD_DRIVER_READERINFO_FN fn);

CHIPCARD_API
void LCD_Driver_SetExtendReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_EXTENDREADER_FN fn);

CHIPCARD_API
void LCD_Driver_SetGetErrorTextFn(LCD_DRIVER *d,
                                  LCD_DRIVER_GETERRORTEXT_FN fn);

CHIPCARD_API
void LCD_Driver_SetPerformVerificationFn(LCD_DRIVER *d,
                                         LCD_DRIVER_PERFORMVERIFICATION_FN f);

CHIPCARD_API
void LCD_Driver_SetPerformModificationFn(LCD_DRIVER *d,
                                         LCD_DRIVER_PERFORMMODIFICATION_FN f);

CHIPCARD_API
void LCD_Driver_SetHandleRequestFn(LCD_DRIVER *d,
                                   LCD_DRIVER_HANDLEREQUEST_FN fn);
/*@}*/



/** @name Constructor, Destructor, Init
 *
 */
/*@{*/

CHIPCARD_API
LCD_DRIVER *LCD_Driver_new();

CHIPCARD_API
void LCD_Driver_free(LCD_DRIVER *d);

CHIPCARD_API
int LCD_Driver_Init(LCD_DRIVER *d, int argc, char **argv);

CHIPCARD_API
int LCD_Driver_Connect(LCD_DRIVER *d,
                       int code, const char *text,
                       GWEN_TYPE_UINT32 dflagsValue,
                       GWEN_TYPE_UINT32 dflagsMask);

CHIPCARD_API
void LCD_Driver_Disconnect(LCD_DRIVER *d);

/*@}*/




/** @name Functions for Testmode
 *
 */
/*@{*/
CHIPCARD_API
int LCD_Driver_Test(LCD_DRIVER *d);

CHIPCARD_API
int LCD_Driver_IsTestMode(const LCD_DRIVER *d);
/*@}*/



/** @name Getting Important Setup Information
 *
 */
/*@{*/

CHIPCARD_API
const char *LCD_Driver_GetDriverDataDir(const LCD_DRIVER *d);

CHIPCARD_API
const char *LCD_Driver_GetLibraryFile(const LCD_DRIVER *d);

CHIPCARD_API
const char *LCD_Driver_GetDriverId(const LCD_DRIVER *d);
/*@}*/


/** @name Finding, Adding and Removing Readers
 *
 */
/*@{*/

CHIPCARD_API
LCD_READER_LIST *LCD_Driver_GetReaders(const LCD_DRIVER *d);

CHIPCARD_API
LCD_READER *LCD_Driver_FindReader(const LCD_DRIVER *d);

CHIPCARD_API
LCD_READER *LCD_Driver_FindReaderByName(const LCD_DRIVER *d, const char *name);

CHIPCARD_API
LCD_READER *LCD_Driver_FindReaderByPort(const LCD_DRIVER *d, int port);

CHIPCARD_API
LCD_READER *LCD_Driver_FindReaderById(const LCD_DRIVER *d, GWEN_TYPE_UINT32 id);

CHIPCARD_API
LCD_READER *LCD_Driver_FindReaderByDriversId(const LCD_DRIVER *d,
                                           GWEN_TYPE_UINT32 id);

CHIPCARD_API
void LCD_Driver_AddReader(LCD_DRIVER *d, LCD_READER *r);

CHIPCARD_API
void LCD_Driver_DelReader(LCD_DRIVER *d, LCD_READER *r);
/*@}*/


/** @name Working on Incoming Requests
 *
 */
/*@{*/

CHIPCARD_API
int LCD_Driver_Work(LCD_DRIVER *d);

CHIPCARD_API
int LCD_Driver_SendStatusChangeNotification(LCD_DRIVER *d,
                                           LCD_SLOT *sl);

CHIPCARD_API
int LCD_Driver_SendReaderErrorNotification(LCD_DRIVER *d,
                                          LCD_READER *r,
                                          const char *text);
/*@}*/



CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Driver_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen);

CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Driver_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);

CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Driver_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Driver_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);

CHIPCARD_API
GWEN_TYPE_UINT32 LCD_Driver_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);









#endif /* CHIPCARD_DRIVER_DRIVER_H */




