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

#define LC_DRIVER_STARTTIMEOUT 20

#define LC_DRIVER_MARK_DRIVER 1


#include <gwenhywfar/logger.h>
#include <chipcard2-server/driver/driver.h>



struct LC_DRIVER {
  GWEN_INHERIT_ELEMENT(LC_DRIVER)
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
  const char *certFile;          /* -c ARG */
  const char *certDir;           /* -C ARG */
  int testMode;                  /* --test */
  const char *rname;             /* -rn ARG */
  int rport;                     /* -rp ARG */
  int rslots;                    /* -rs ARG */
  const char *rtype;             /* -rt ARG */
  const char *dtype;             /* -dt ARG */
  int remoteMode;                /* --remote */
  GWEN_TYPE_UINT32 rflags;       /* -rf ARG */

  /* runtime data */
  int stopDriver;
  GWEN_IPCMANAGER *ipcManager;
  LC_READER_LIST *readers;
  GWEN_TYPE_UINT32 ipcId;

  GWEN_TYPE_UINT32 lastReaderId;

  LC_DRIVER_SENDAPDU_FN sendApduFn;
  LC_DRIVER_CONNECTSLOT_FN connectSlotFn;
  LC_DRIVER_CONNECTREADER_FN connectReaderFn;
  LC_DRIVER_DISCONNECTSLOT_FN disconnectSlotFn;
  LC_DRIVER_DISCONNECTREADER_FN disconnectReaderFn;
  LC_DRIVER_RESETSLOT_FN resetSlotFn;
  LC_DRIVER_READERSTATUS_FN readerStatusFn;
  LC_DRIVER_GETERRORTEXT_FN getErrorTextFn;
  LC_DRIVER_READERINFO_FN readerInfoFn;
  LC_DRIVER_CREATEREADER_FN createReaderFn;
};


int LC_Driver__Work(LC_DRIVER *d, int timeout, int maxMsg);


LC_DRIVER_CHECKARGS_RESULT LC_Driver_CheckArgs(LC_DRIVER *d,
                                               int argc, char **argv);

int LC_Driver_ReplaceVar(const char *path,
                         const char *var,
                         const char *value,
                         GWEN_BUFFER *nbuf);


int LC_Driver_HandleStartReader(LC_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq);
int LC_Driver_HandleStopReader(LC_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq);
int LC_Driver_HandleResetCard(LC_DRIVER *d,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *dbReq);

int LC_Driver_HandleCardCommand(LC_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq);

int LC_Driver_HandleStopDriver(LC_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq);


#endif /* CHIPCARD_DRIVER_DRIVER_P_H */




