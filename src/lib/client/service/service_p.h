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

#include "service.h"

#include <gwenhywfar/logger.h>


typedef struct LC_SERVICE_CLIENT LC_SERVICE_CLIENT;
struct LC_SERVICE_CLIENT {
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
  const char *configDir;         /* -C ARG */

  /* runtime data */
  int stopService;

  LC_SERVICECLIENT_LIST *clients;

  LC_SERVICE_OPEN_FN openFn;
  LC_SERVICE_CLOSE_FN closeFn;
  LC_SERVICE_COMMAND_FN commandFn;
  LC_SERVICE_GETERRORTEXT_FN getErrorTextFn;
  LC_SERVICE_WORK_FN workFn;
};
void GWENHYWFAR_CB LC_Service_freeData(void *bp, void *p);


LC_SERVICE_CHECKARGS_RESULT LC_Service_CheckArgs(LC_CLIENT *cl,
                                                 int argc, char **argv);


int LC_Service_HandleServiceOpen(LC_CLIENT *cl,
                                 uint32_t rid,
                                 GWEN_DB_NODE *dbReq);
int LC_Service_HandleServiceClose(LC_CLIENT *cl,
                                  uint32_t rid,
                                  GWEN_DB_NODE *dbReq);
int LC_Service_HandleServiceCommand(LC_CLIENT *cl,
                                    uint32_t rid,
                                    GWEN_DB_NODE *dbReq);
int LC_Service_HandleInRequest(LC_CLIENT *cl,
                               uint32_t rid,
                               const char *name,
                               GWEN_DB_NODE *dbReq);


#endif /* CHIPCARD_SERVICE_SERVICE_P_H */




