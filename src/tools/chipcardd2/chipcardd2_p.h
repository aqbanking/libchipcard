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
#include <chipcard2-server/server/cardserver.h>
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/nettransportssl.h>
#include "cardserver_l.h"


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
#ifdef HAVE_FORK
  int daemonMode;
#endif
  const char *configFile;
  const char *dataDir;
#ifdef HAVE_GETPID
  const char *pidFile;
#endif
  const char *logFile;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;
  int exitOnSetupError;
  int runOnce;

  const char *certFile;
  const char *dhFile;
  const char *countryName;
  const char *commonName;

  const char *dtype;
  const char *rtype;
  const char *rname;
  const char *rport;
};


int init(ARGUMENTS *args);
int mkCert(ARGUMENTS *args);
int addReader(ARGUMENTS *args);
int delReader(ARGUMENTS *args);
int testReader(ARGUMENTS *args);




#endif

