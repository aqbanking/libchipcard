/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: chipcardd_p.h 220 2006-09-08 13:00:00Z martin $
    begin       : Sun May 30 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef CHIPCARDD2_P_H
#define CHIPCARDD2_P_H

#include <chipcard/chipcard.h>
#include "server_l.h"
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/buffer.h>


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


int addReader(ARGUMENTS *args);
int delReader(ARGUMENTS *args);
int testReader(ARGUMENTS *args);
int listReaders(ARGUMENTS *args);


/**
 * @return 0 if given file or system file used, 1 if example file found,
 */
int getConfigFile(ARGUMENTS *args, GWEN_BUFFER *nbuf);


#endif

