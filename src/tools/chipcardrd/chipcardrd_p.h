/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Sun May 30 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef CHIPCARDD2_P_H
#define CHIPCARDD2_P_H

#include <chipcard2/chipcard2.h>
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/process.h>
#include <gwenhywfar/nettransportssl.h>


#define RETURNVALUE_PARAM   1
#define RETURNVALUE_SETUP   2
#define RETURNVALUE_NOSTART 3
#define RETURNVALUE_DEINIT  4
#define RETURNVALUE_HANGUP  9


typedef struct _S_PARAM FREEPARAM;
typedef struct _S_ARGS ARGUMENTS;

struct _S_PARAM {
  FREEPARAM  *next;
  const char *param;
};



struct _S_ARGS {
  FREEPARAM *params;
  int verbous;
  const char *configFile;
#ifdef HAVE_GETPID
  const char *pidFile;
#endif
  const char *logFile;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;
  int exitOnSetupError;
};



typedef struct DRIVER DRIVER;
GWEN_LIST_FUNCTION_DEFS(DRIVER, Driver)

struct DRIVER {
  GWEN_LIST_ELEMENT(DRIVER)
  GWEN_PROCESS *process;

  int secure;
  int acceptAllCerts;
  char *driverType;
  char *driverName;
  char *libraryFile;

  char *certFile;
  char *certDir;

  char *logFile;
  char *readerLogFile;
  char *logType;
  char *logLevel;
  char *driverDataDir;
  char *readerName;
  char *readerType;
  int rport;
  int rslots;
  time_t timeMark;
};

DRIVER *Driver_new();
void Driver_free(DRIVER *d);
DRIVER *Driver_fromDb(GWEN_DB_NODE *db);

int initFromDb(GWEN_DB_NODE *db);




#endif

