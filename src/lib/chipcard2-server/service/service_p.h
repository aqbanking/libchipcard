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


#ifndef CHIPCARD_SERVICE_SERVICE_P_H
#define CHIPCARD_SERVICE_SERVICE_P_H

#define LC_SERVICE_STARTTIMEOUT 20

#define LC_SERVICE_MARK_SERVICE 1

#define LC_SERVICE_IPC_MAXWORK 256

#include <gwenhywfar/logger.h>
#include <chipcard2-server/service/service.h>



struct LC_SERVICE {
  GWEN_INHERIT_ELEMENT(LC_SERVICE)
  /* arguments */
  int verbous;                   /* -v */
  int secure;                    /* --secure */
  char *logFile;                 /* --logfile ARG */
  GWEN_LOGGER_LOGTYPE logType;   /* --logtype ARG */
  GWEN_LOGGER_LEVEL logLevel;    /* --loglevel ARG */
  const char *serviceDataDir;    /* -d ARG */
  const char *serverAddr;        /* -a ARG */
  int serverPort;                /* -p ARG */
  const char *libraryFile;       /* -l ARG */
  const char *serviceId;         /* -i ARG */
  const char *typ;               /* -t ARG */
  const char *certFile;          /* -c ARG */
  const char *certDir;           /* -C ARG */

  /* runtime data */
  int stopService;
  GWEN_IPCMANAGER *ipcManager;
  GWEN_TYPE_UINT32 ipcId;

  LC_SERVICECLIENT_LIST *clients;

  LC_SERVICE_OPEN_FN openFn;
  LC_SERVICE_CLOSE_FN closeFn;
  LC_SERVICE_COMMAND_FN commandFn;
  LC_SERVICE_GETERRORTEXT_FN getErrorTextFn;
};


int LC_Service__Work(LC_SERVICE *d, int timeout, int maxMsg);


LC_SERVICE_CHECKARGS_RESULT LC_Service_CheckArgs(LC_SERVICE *d,
                                                 int argc, char **argv);


int LC_Service_HandleServiceOpen(LC_SERVICE *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);
int LC_Service_HandleServiceClose(LC_SERVICE *d,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq);
int LC_Service_HandleServiceCommand(LC_SERVICE *d,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);


#endif /* CHIPCARD_SERVICE_SERVICE_P_H */




