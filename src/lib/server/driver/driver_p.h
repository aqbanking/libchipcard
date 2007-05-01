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


#ifndef CHIPCARD_DRIVER_DRIVER_P_H
#define CHIPCARD_DRIVER_DRIVER_P_H

#define LCD_DRIVER_STARTTIMEOUT 20

#define LCD_DRIVER_MARK_DRIVER 1


#include <gwenhywfar/logger.h>
#include <gwenhywfar/nl_ssl.h>
#include "driver.h"


typedef enum {
  LCD_DriverCheckArgsResultOk=0,
  LCD_DriverCheckArgsResultError,
  LCD_DriverCheckArgsResultVersion,
  LCD_DriverCheckArgsResultHelp
} LCD_DRIVER_CHECKARGS_RESULT;



struct LCD_DRIVER {
  GWEN_INHERIT_ELEMENT(LCD_DRIVER)
  /* arguments */
  int verbous;                   /* -v */
  int secure;                    /* --secure */
  char *logFile;                 /* --logfile ARG */
  char *readerLogFile;           /* --logfile ARG */
  GWEN_LOGGER_LOGTYPE logType;   /* --logtype ARG */
  GWEN_LOGGER_LEVEL logLevel;    /* --loglevel ARG */
  const char *driverDataDir;     /* -d ARG */
  const char *serverAddr;        /* -a ARG */
  int serverPort;                /* -p ARG */
  const char *libraryFile;       /* -l ARG */
  const char *driverId;          /* -i ARG */
  const char *typ;               /* -t ARG */
  int acceptAllCerts;            /* --accept-all-certs */
  const char *certFile;          /* -c ARG */
  const char *certDir;           /* -C ARG */
  int testMode;                  /* --test */
  const char *rname;             /* -rn ARG */
  int rport;                     /* -rp ARG */
  int rslots;                    /* -rs ARG */
  const char *rtype;             /* -rt ARG */
  const char *dtype;             /* -dt ARG */
  GWEN_TYPE_UINT32 rflags;       /* -rf ARG */
  const char *devicePath;        /* -dp ARG */

  /* runtime data */
  int stopDriver;
  GWEN_IPCMANAGER *ipcManager;
  LCD_READER_LIST *readers;
  GWEN_TYPE_UINT32 ipcId;

  GWEN_TYPE_UINT32 lastReaderId;

  LCD_DRIVER_SENDAPDU_FN sendApduFn;
  LCD_DRIVER_CONNECTSLOT_FN connectSlotFn;
  LCD_DRIVER_CONNECTREADER_FN connectReaderFn;
  LCD_DRIVER_DISCONNECTSLOT_FN disconnectSlotFn;
  LCD_DRIVER_DISCONNECTREADER_FN disconnectReaderFn;
  LCD_DRIVER_RESETSLOT_FN resetSlotFn;
  LCD_DRIVER_READERSTATUS_FN readerStatusFn;
  LCD_DRIVER_GETERRORTEXT_FN getErrorTextFn;
  LCD_DRIVER_READERINFO_FN readerInfoFn;
  LCD_DRIVER_EXTENDREADER_FN extendReaderFn;

  LCD_DRIVER_PERFORMVERIFICATION_FN performVerificationFn;
  LCD_DRIVER_PERFORMMODIFICATION_FN performModificationFn;

  LCD_DRIVER_HANDLEREQUEST_FN handleRequestFn;
};


static
int LCD_Driver__Work(LCD_DRIVER *d, int timeout);


static
LCD_DRIVER_CHECKARGS_RESULT LCD_Driver_CheckArgs(LCD_DRIVER *d,
                                                 int argc, char **argv);

static
int LCD_Driver_ReplaceVar(const char *path,
                         const char *var,
                         const char *value,
                         GWEN_BUFFER *nbuf);

static
GWEN_NL_SSL_ASKADDCERT_RESULT
  LCD_Driver_AskAddCert(GWEN_NETLAYER *nl,
                        const GWEN_SSLCERTDESCR *cert,
                        void *user_data);


static
int LCD_Driver_HandleRequest(LCD_DRIVER *d,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq);


static
int LCD_Driver_HandleStartReader(LCD_DRIVER *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleStopReader(LCD_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleResetCard(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleCardCommand(LCD_DRIVER *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleStopDriver(LCD_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleSuspendCheck(LCD_DRIVER *d,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq);

static
int LCD_Driver_HandleResumeCheck(LCD_DRIVER *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);




static
GWEN_TYPE_UINT32 LCD_Driver_SendCommand(LCD_DRIVER *d,
                                       GWEN_DB_NODE *dbCommand);

static
int LCD_Driver_SendResponse(LCD_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand);

static
int LCD_Driver_SendResult(LCD_DRIVER *d,
                          GWEN_TYPE_UINT32 rid,
                          const char *name,
                          int code,
                          const char *text);

static
int LCD_Driver_RemoveCommand(LCD_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound);

static
GWEN_TYPE_UINT32 LCD_Driver_GetNextInRequest(LCD_DRIVER *d);

static
GWEN_DB_NODE *LCD_Driver_GetInRequestData(LCD_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid);




static
GWEN_TYPE_UINT32 LCD_Driver_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);


static
GWEN_TYPE_UINT32 LCD_Driver_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);

static
const char *LCD_Driver_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);

static
GWEN_TYPE_UINT32 LCD_Driver_ReaderInfo(LCD_DRIVER *d,
                                      LCD_READER *r,
                                      GWEN_BUFFER *buf);

static
LCD_READER *LCD_Driver_CreateReader(LCD_DRIVER *d,
                                    GWEN_TYPE_UINT32 readerId,
                                    const char *name,
                                    int port,
                                    const char *devicePath,
                                    unsigned int slots,
                                    GWEN_TYPE_UINT32 flags);


static
void LCD_Driver_Usage(const char *prgName);

#endif /* CHIPCARD_DRIVER_DRIVER_P_H */




