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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef BUILDING_LIBCHIPCARD2_DLL

/* define this to dump the server state upon reception of signal USR1 */
#define USR1_DUMPS 1

/* Internationalization */
#ifdef HAVE_GETTEXT_ENVIRONMENT
# include <libintl.h>
# include <locale.h>
# define I18N(m) gettext(m)
#else
# define I18N(m) m
#endif
#define I18NT(m) m


#include "chipcardrd_p.h"

#include <gwenhywfar/inetsocket.h>
#include <gwenhywfar/directory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#ifdef OS_WIN32
# include <process.h> /* for getpid */
#endif


#define CHIPCARDRD_RESTART_TIME 10


GWEN_LIST_FUNCTIONS(DRIVER, Driver)

static DRIVER_LIST *DriverList=0;
static char *ServerAddr=0;
static int ServerPort;
static char *ServerType=0;

static int RestartTime=CHIPCARDRD_RESTART_TIME;

static int DaemonMode=1;
static int DriverDaemonStop=0;
static int DriverDaemonHangup=0;
static int DriverNannyStop=0;
static int DriverNannySuspend=0;
static int DriverNannyResume=0;
static time_t LastFailedTime=0;
static int ShortFailCounter=0;


#define k_PRG "chipcardrd"
#define k_PRG_VERSION_INFO \
  "chipcardrd v1.9.0 (part of libchipcard v"CHIPCARD_VERSION_STRING")\n"\
  "(c) 2005 Martin Preuss<martin@libchipcard.de>\n"\
  "This program is free software licensed under GPL.\n"\
  "See COPYING for details.\n"




FREEPARAM *FreeParam_new(const char *s) {
  FREEPARAM *fr;

  fr=(FREEPARAM*)malloc(sizeof(FREEPARAM));
  assert(fr);
  memset(fr, 0, sizeof(FREEPARAM));
  fr->param=s;
  return fr;
}


void FreeParam_free(FREEPARAM *fr) {
  if (fr)
    free(fr);
}



ARGUMENTS *Arguments_new() {
  ARGUMENTS *ar;

  ar=(ARGUMENTS*)malloc(sizeof(ARGUMENTS));
  assert(ar);
  memset(ar, 0, sizeof(ARGUMENTS));
  ar->verbous=0;
  ar->configFile=LC_DEFAULT_DATADIR "/chipcardrd.conf";
  ar->pidFile=LC_DEFAULT_PIDDIR "/chipcardrd.pid";
  ar->logFile=LC_DEFAULT_LOGDIR "/chipcardrd.log";
  ar->logLevel=GWEN_LoggerLevelNotice;
#ifdef HAVE_SYSLOG_H
  ar->logType=GWEN_LoggerTypeSyslog;
#else
  ar->logType=GWEN_LoggerTypeFile;
#endif
  ar->exitOnSetupError=0;

  return ar;
}


void Arguments_free(ARGUMENTS *ar) {
  if (ar) {
    FREEPARAM *fr;
    FREEPARAM *next;

    fr=ar->params;
    while(fr) {
      next=fr->next;
      FreeParam_free(fr);
      fr=next;
    } /* while */
    free(ar);
  }
}


void usage(const char *name) {
  fprintf(stdout,"%s%s%s%s%s%s",
          I18N("RemoteDriverDaemon - "
               "A remote driver daemon for Libchipcard2\n"
               "                 Part of LibChipCard2 "
               CHIPCARD_VERSION_STRING"\n"
               "(c) 2004,2005 Martin Preuss<martin@libchipcard.de>\n"
	       "This library is free software; you can redistribute it and/or\n"
	       "modify it under the terms of the GNU Lesser General Public\n"
	       "License as published by the Free Software Foundation; either\n"
	       "version 2.1 of the License, or (at your option) any later version.\n"
	       "\n"
	       "Usage:\n"
               k_PRG" COMMAND [OPTIONS]\n"
               "\nCommands:\n"
               " server\n"
               "  Starts configured drivers\n"
               "\n"
               "\nOptions:\n"
               " -C CONFIGFILE    - path and name of the configuration file\n"),
#ifdef HAVE_FORK
	  I18N(" -f               - stay in foreground (do not fork)\n"),
#else
	  "",
#endif
	  I18N(" -h               - show this help\n"
	       " -V               - show version information\n"
	       " -v               - be more verbous\n"
	       " --pidfile FILE   - use given FILE as PID file\n"
	       " --logfile FILE   - use given FILE as log file\n"
	       " --logtype TYPE   - use given TYPE as log type\n"
	       "                    These are the valid types:\n"
	       "                     console (log to standard error channel)\n"
	       "                     file   (log to the file given by --logfile)\n"),
#ifdef HAVE_SYSLOG_H
	  I18N("                     syslog (log via syslog)\n"),
#else
	  "",
#endif
#ifdef HAVE_SYSLOG_H
	  I18N("                    Default is syslog\n"),
#else
	  I18N("                    Default is stderr\n"),
#endif
	  I18N(" --loglevel LEVEL - set the loglevel\n"
	       "                    Valid levels are:\n"
	       "                     emergency, alert, critical, error,\n"
	       "                     warning, notice, info and debug\n"
               "                    Default is \"notice\".\n"
               " --exit-on-error  - makes chipcardrd exit on setup errors\n"
               "\n"
              )
	 );
}



void Arguments_AddParam(ARGUMENTS *ar, const char *pr) {
  FREEPARAM *curr;
  FREEPARAM *nfp;

  assert(ar);
  assert(pr);

  nfp=FreeParam_new(pr);

  curr=ar->params;
  if (!curr) {
    ar->params=nfp;
  }
  else {
    /* find last */
    while(curr->next) {
      curr=curr->next;
    } /* while */
    curr->next=nfp;
  }
}




int checkArgs(ARGUMENTS *args, int argc, char **argv) {
  int i;

  i=1;
  while (i<argc){
    /* global options */
    if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
	return RETURNVALUE_PARAM;
      args->configFile=argv[i];
    }
    else if (strcmp(argv[i],"--pidfile")==0) {
      i++;
      if (i>=argc)
	return RETURNVALUE_PARAM;
      args->pidFile=argv[i];
    }

    else if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
	return RETURNVALUE_PARAM;
      args->logFile=argv[i];
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (args->logType==GWEN_LoggerTypeUnknown) {
        fprintf(stderr,
		I18N("Unknown log type \"%s\"\n"),
		argv[i]);
	return RETURNVALUE_PARAM;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
	return RETURNVALUE_PARAM;
      args->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (args->logLevel==GWEN_LoggerLevelUnknown) {
        fprintf(stderr,
                I18N("Unknown log level \"%s\"\n"),
                argv[i]);
        return RETURNVALUE_PARAM;
      }
    }
    else if (strcmp(argv[i],"--exit-on-error")==0) {
      args->exitOnSetupError=1;
    }
#ifdef HAVE_FORK
    else if (strcmp(argv[i],"-f")==0) {
      DaemonMode=0;
    }
#endif

    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      usage(argv[0]);
      return -1;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      fprintf(stdout, k_PRG_VERSION_INFO);
      return -1;
    }
    else if (strcmp(argv[i],"-v")==0) {
      args->verbous=1;
    }
    else {
      /* otherwise add param */
      if (argv[i][0]=='-') {
	fprintf(stderr,I18N("Unknown option \"%s\"\n"),argv[i]);
	return RETURNVALUE_PARAM;
      }
      else
	Arguments_AddParam(args, argv[i]);
    }
    i++;
  } /* while */

  /* that's it */
  return 0;
}



#ifdef HAVE_SIGACTION
/* Signal handler */

void familySignalHandler(int s, int child) {
  switch(s) {
  case SIGINT:
    if (child) {
      DBG_NOTICE(0, "Daemon got an interrupt signal, will go down.");
      DriverDaemonStop=1;
    }
    else {
      DBG_NOTICE(0, "Nanny got an interrupt signal, will terminate child");
      DriverNannyStop=1;
    }
    break;

  case SIGTERM:
    if (child) {
      DBG_NOTICE(0, "Daemon got a termination signal");
      DriverDaemonStop=1;
    }
    else {
      DBG_NOTICE(0, "Nanny got a termination signal, will terminate child.");
      DriverNannyStop=1;
    }
    break;

#ifdef SIGINFO
  case SIGINFO:
    if (child) {
      DBG_NOTICE(0, "Daemon got an info signal");
    }
    else {
      DBG_NOTICE(0, "Nanny got an info signal");
    }
    break;
#endif

#ifdef SIGCHLD
  case SIGCHLD:
    if (!child) {
      DBG_NOTICE(0, "Nanny got a child signal");
    }
    break;
#endif

#ifdef SIGHUP
  case SIGHUP:
    if (child) {
      /* child exits on this signal and will be respawned by the nanny */
      DBG_NOTICE(0, "Daemon got a hangup signal, going down.");
      DriverDaemonStop=1;
      DriverDaemonHangup=1;
    }
    else {
      if (DriverNannySuspend) {
	DBG_NOTICE(0, "Restarting daemon");
	DriverNannyResume=1;
      }
    }
    break;
#endif

#ifdef SIGALRM
  case SIGALRM:
    DBG_NOTICE(0, "Got an alarm signal");
    break;
#endif

#ifdef SIGUSR1
  case SIGUSR1:
    if (GWEN_Logger_GetLevel(0)<GWEN_LoggerLevelDebug) {
      DBG_NOTICE(0, "Got signal USR1, will increase log level");
      GWEN_Logger_SetLevel(0, GWEN_Logger_GetLevel(0)+1);
    }
    else {
      DBG_NOTICE(0, "Got signal USR1, but log level already is lowest");
    }
#endif
    break;
#endif

#ifdef SIGUSR2
  case SIGUSR2:
    if (GWEN_Logger_GetLevel(0)>GWEN_LoggerLevelEmergency) {
      DBG_NOTICE(0, "Got signal USR2, will decrease log level");
      GWEN_Logger_SetLevel(0, GWEN_Logger_GetLevel(0)-1);
    }
    else {
      DBG_NOTICE(0, "Got signal USR2, but log level already is highest");
    }
    break;
#endif

#ifdef SIGTSTP
  case SIGTSTP:
    if (!child) {
      DBG_NOTICE(0, "Suspending daemon");
      DriverNannySuspend=1;
    }
    break;
#endif

#ifdef SIGCONT
  case SIGCONT:
    if (!child && DriverNannySuspend) {
      DBG_NOTICE(0, "Resuming daemon");
      DriverNannyResume=1;
    }
    break;
#endif

  default:
    DBG_WARN(0, "Unknown signal  %d",s);
    break;
  } /* switch */
}


void childSignalHandler(int s) {
  return familySignalHandler(s, 1);
}


void fatherSignalHandler(int s) {
  return familySignalHandler(s, 0);
}



struct sigaction saINT,saTERM, saINFO, saHUP;
struct sigaction saUSR1, saUSR2, saALRM, saCHLD;
struct sigaction saTSTP, saCONT;


int setSingleSignalHandler(struct sigaction *sa,
			   int sig,
			   int child) {
  if (child)
    sa->sa_handler=childSignalHandler;
  else
    sa->sa_handler=fatherSignalHandler;
  sigemptyset(&sa->sa_mask);
  sa->sa_flags=0;
  if (sigaction(sig, sa,0)) {
    DBG_ERROR(0, "Could not setup signal handler for signal %d", sig);
    return RETURNVALUE_SETUP;
  }
  return 0;
}


int setSignalHandler(int child) {
  int rv;

  rv=setSingleSignalHandler(&saINT, SIGINT, child);
  if (rv)
    return rv;
  rv=setSingleSignalHandler(&saCHLD, SIGCHLD, child);
  if (rv)
    return rv;
  rv=setSingleSignalHandler(&saTERM, SIGTERM, child);
  if (rv)
    return rv;
#ifdef SIGINFO
  rv=setSingleSignalHandler(&saINFO, SIGINFO, child);
  if (rv)
    return rv;
#endif

#ifdef SIGHUP
  rv=setSingleSignalHandler(&saHUP, SIGHUP, child);
  if (rv)
    return rv;
#endif

#ifdef SIGALRM
  rv=setSingleSignalHandler(&saALRM, SIGALRM, child);
  if (rv)
    return rv;
#endif

#ifdef SIGUSR1
  rv=setSingleSignalHandler(&saUSR1, SIGUSR1, child);
  if (rv)
    return rv;
#endif

#ifdef SIGUSR2
  rv=setSingleSignalHandler(&saUSR2, SIGUSR2, child);
  if (rv)
    return rv;
#endif

#ifdef SIGTSTP
  rv=setSingleSignalHandler(&saTSTP, SIGTSTP, child);
  if (rv)
    return rv;
#endif

#ifdef SIGCONT
  rv=setSingleSignalHandler(&saCONT, SIGCONT, child);
  if (rv)
    return rv;
#endif

  return 0;
}



int createPidFile(const char *pidfile) {
  FILE *f;
  int pidfd;

  /* the PID file code has been provided by Thomas Viehmann and adapted
   * by me */
  /* create pid file */
  if(remove(pidfile)==0)
    DBG_ERROR(0, "Old PID file existed, removed. (Unclean shutdown?)");

#ifdef HAVE_SYS_STAT_H
  pidfd = open(pidfile, O_EXCL|O_CREAT|O_WRONLY,
#ifndef OS_WIN32
               S_IRGRP|S_IROTH |
#endif
               S_IRUSR|S_IWUSR);
  if (pidfd < 0) {
    DBG_ERROR(0, "Could not create PID file \"%s\" (%s), aborting.",
	      pidfile,
	      strerror(errno));
    return RETURNVALUE_SETUP;
  }

  f = fdopen(pidfd, "w");
#else /* HAVE_STAT_H */
  f=fopen(pidfile,"w+");
#endif /* HAVE_STAT_H */

  /* write pid */
#ifdef HAVE_GETPID
  fprintf(f,"%d\n",getpid());
#else
  fprintf(f,"-1\n");
#endif
  if (fclose(f)) {
    DBG_ERROR(0, "Could not close PID file \"%s\" (%s), aborting.",
	      pidfile,
	      strerror(errno));
    return RETURNVALUE_SETUP;
  }
  return 0;
}



DRIVER *Driver_new() {
  DRIVER *d;

  GWEN_NEW_OBJECT(DRIVER, d);
  GWEN_LIST_INIT(DRIVER, d);

  return d;
}



void Driver_free(DRIVER *d) {
  if (d) {
    GWEN_LIST_FINI(DRIVER, d);

    GWEN_Process_free(d->process);
    free(d->driverType);
    free(d->driverName);
    free(d->libraryFile);
    free(d->certFile);
    free(d->certDir);
    free(d->logFile);
    free(d->readerLogFile);
    free(d->logType);
    free(d->logLevel);
    free(d->driverDataDir);
    free(d->readerName);
    free(d->readerType);

    GWEN_FREE_OBJECT(d);
  }
}



DRIVER *Driver_fromDb(GWEN_DB_NODE *db) {
  DRIVER *d;
  const char *s;
  int i;
  GWEN_DB_NODE *dbReader;

  d=Driver_new();
  s=GWEN_DB_GetCharValue(db, "driverType", 0, 0);
  if (!s) {
    DBG_ERROR(0, "DriverType missing");
    Driver_free(d);
    return 0;
  }
  d->driverType=strdup(s);

  s=GWEN_DB_GetCharValue(db, "driverName", 0, 0);
  if (!s) {
    DBG_ERROR(0, "DriverName missing");
    Driver_free(d);
    return 0;
  }
  d->driverName=strdup(s);

  s=GWEN_DB_GetCharValue(db, "libraryFile", 0, 0);
  if (!s) {
    DBG_ERROR(0, "LibraryFile missing");
    Driver_free(d);
    return 0;
  }
  d->libraryFile=strdup(s);

  s=GWEN_DB_GetCharValue(db, "certFile", 0, 0);
  if (s)
    d->certFile=strdup(s);

  s=GWEN_DB_GetCharValue(db, "certDir", 0, 0);
  if (s)
    d->certDir=strdup(s);

  s=GWEN_DB_GetCharValue(db, "logFile", 0, 0);
  if (s)
    d->logFile=strdup(s);

  s=GWEN_DB_GetCharValue(db, "readerLogFile", 0, 0);
  if (s)
    d->readerLogFile=strdup(s);

  s=GWEN_DB_GetCharValue(db, "logType", 0, 0);
  if (s)
    d->logType=strdup(s);

  s=GWEN_DB_GetCharValue(db, "logLevel", 0, 0);
  if (s)
    d->logLevel=strdup(s);

  s=GWEN_DB_GetCharValue(db, "driverDataDir", 0, 0);
  if (s)
    d->driverDataDir=strdup(s);

  d->secure=GWEN_DB_GetIntValue(db, "secure", 0, 0);
  d->acceptAllCerts=GWEN_DB_GetIntValue(db, "acceptAllCerts", 0, 0);

  dbReader=GWEN_DB_FindFirstGroup(db, "reader");
  if (dbReader==0) {
    DBG_ERROR(0, "No reader section for driver");
    Driver_free(d);
    return 0;
  }

  s=GWEN_DB_GetCharValue(dbReader, "readerName", 0, 0);
  if (!s) {
    DBG_ERROR(0, "ReaderName missing");
    Driver_free(d);
    return 0;
  }
  d->readerName=strdup(s);

  s=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
  if (!s) {
    DBG_ERROR(0, "ReaderType missing");
    Driver_free(d);
    return 0;
  }
  d->readerType=strdup(s);

  i=GWEN_DB_GetIntValue(dbReader, "port", 0, -1);
  if (i==-1) {
    DBG_ERROR(0, "Port missing");
    Driver_free(d);
    return 0;
  }
  d->rport=i;

  i=GWEN_DB_GetIntValue(dbReader, "slots", 0, -1);
  if (i==-1) {
    DBG_ERROR(0, "ReaderPort missing");
    Driver_free(d);
    return 0;
  }
  d->rslots=i;

  return d;
}



int initFromDb(GWEN_DB_NODE *db) {
  GWEN_DB_NODE *dbDriver;
  const char *s;
  int drivers=0;

  DriverList=Driver_List_new();

  s=GWEN_DB_GetCharValue(db, "serverAddr", 0, 0);
  if (!s) {
    DBG_ERROR(0, "ServerAddr missing");
    return -1;
  }
  ServerAddr=strdup(s);

  ServerPort=GWEN_DB_GetIntValue(db, "serverPort", 0, 0);
  s=GWEN_DB_GetCharValue(db, "serverType", 0, 0);
  if (!s) {
    DBG_ERROR(0, "ServerType missing");
    return -1;
  }
  ServerType=strdup(s);

  RestartTime=GWEN_DB_GetIntValue(db, "restartTime", 0,
                                  CHIPCARDRD_RESTART_TIME);

  dbDriver=GWEN_DB_FindFirstGroup(db, "driver");
  while(dbDriver) {
    DRIVER *d;

    d=Driver_fromDb(dbDriver);
    if (d) {
      drivers++;
      Driver_List_Add(d, DriverList);
    }
    dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
  } /* while */

  if (!drivers) {
    DBG_ERROR(0, "No drivers found");
    return -1;
  }

  return 0;
}



int Driver_Start(DRIVER *d) {
  GWEN_PROCESS *p;
  GWEN_BUFFER *pbuf;
  GWEN_BUFFER *abuf;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;

  assert(d);

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=d->driverDataDir;
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=d->libraryFile;
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -l ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=d->logFile;
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=getenv("LC_DRIVER_LOGLEVEL");
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --loglevel ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=ServerType;
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=ServerAddr;
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", ServerPort);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  /* use driver id 0 */
  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", 0);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  if (d->acceptAllCerts)
    GWEN_Buffer_AppendString(abuf, " --accept-all-certs");

  GWEN_Buffer_AppendString(abuf, " -dt ");
  GWEN_Buffer_AppendString(abuf, d->driverName);

  GWEN_Buffer_AppendString(abuf, " -rt ");
  GWEN_Buffer_AppendString(abuf, d->readerType);

  GWEN_Buffer_AppendString(abuf, " -rn ");
  GWEN_Buffer_AppendString(abuf, d->readerName);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", d->rport);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -rp ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", d->rslots);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -rs ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  GWEN_Buffer_AppendString(abuf, " --remote");

  s=d->driverType;
  pbuf=GWEN_Buffer_new(0, 128, 0, 1);
  GWEN_Directory_OsifyPath(LC_DEVICEDRIVER_PATH, pbuf, 1);
#ifdef OS_WIN32
  GWEN_Buffer_AppendByte(pbuf, '\\');
#else
  GWEN_Buffer_AppendByte(pbuf, '/');
#endif
  while(*s) {
    GWEN_Buffer_AppendByte(pbuf, tolower(*s));
    s++;
  } /* while */

  p=GWEN_Process_new();
  DBG_INFO(0, "Starting driver process for driver \"%s\" (%s)",
           d->driverName, GWEN_Buffer_GetStart(pbuf));
  DBG_INFO(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

  pst=GWEN_Process_Start(p,
                         GWEN_Buffer_GetStart(pbuf),
                         GWEN_Buffer_GetStart(abuf));
  if (pst!=GWEN_ProcessStateRunning) {
    DBG_ERROR(0, "Unable to execute \"%s %s\"",
              GWEN_Buffer_GetStart(pbuf),
              GWEN_Buffer_GetStart(abuf));
    GWEN_Process_free(p);
    GWEN_Buffer_free(pbuf);
    GWEN_Buffer_free(abuf);
    return -1;
  }
  DBG_INFO(0, "Process started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  d->process=p;
  return 0;
}



int Driver_Check(DRIVER *d) {
  GWEN_PROCESS *p;

  /* check whether the driver really is still up and running */
  p=d->process;
  if (p) {
    if (GWEN_Process_CheckState(p)!=GWEN_ProcessStateRunning) {
      DBG_ERROR(0, "Driver is not running anymore");
      GWEN_Process_Terminate(p);
      d->process=0;
      d->timeMark=time(0);
      return -1;
    }
  } /* if process */
  else {
    time_t now;

    now=time(0);
    if (difftime(now, d->timeMark)>RestartTime) {
      int rv;

      rv=Driver_Start(d);
      if (rv) {
        d->timeMark=now;
        return rv;
      }
    }
  }

  return 0;
}



/* __________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *                               Server code
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */

int server(ARGUMENTS *args) {
  const char *pidfile;
  int rv;
  int enabled;
  int loopCount;
  GWEN_DB_NODE *db;

  pidfile=args->pidFile;

  DBG_NOTICE(0,
             "Chipcard Remote Driver v"
             CHIPCARD_VERSION_FULL_STRING
             " started.");

#ifdef HAVE_FORK
  if (DaemonMode) {
    rv=fork();
    if (rv==-1) {
        DBG_ERROR(0, "Error on fork, aborting.");
        if (args->logType!=GWEN_LoggerTypeConsole)
            fprintf(stderr,I18N("Error on fork, aborting.\n"));
        remove(pidfile);
        return RETURNVALUE_SETUP;
    }
    else if (rv!=0) {
        /* father process, does nothing more */
        DBG_DEBUG(0, "Daemon forked, father exiting");
        return 0;
    }
  
    /* this will be executed by the child */
    /* create pid file */
    rv=createPidFile(pidfile);
    if (rv!=0) {
      return rv;
    }
  #ifdef HAVE_SIGACTION
    /* setup some signal handler */
    if (setSignalHandler(0)) {
      DBG_ERROR(0, "Error setting up signal handler, aborting");
      remove(pidfile);
      remove(pidfile);
      return RETURNVALUE_SETUP;
    }
  #endif
    /* now spawn a child process which really does the work */
  
    while (1) {
      DriverDaemonStop=0;
      DriverDaemonHangup=0;
      rv=fork();
      if (rv==-1) {
        DBG_ERROR(0, "Error on fork, aborting.");
        if (args->logType!=GWEN_LoggerTypeConsole)
          fprintf(stderr,I18N("Error on fork, aborting.\n"));
        remove(pidfile);
        return RETURNVALUE_SETUP;
      }
      else if (rv!=0) {
        /* father process */
        int childPID;
  
        /* store process id of the child */
        childPID=rv;
        DBG_NOTICE(0, "Nanny now supervising daemon %d", childPID);
        while(1) {
          int status;
  
          if (DriverNannySuspend) {
            if (DriverNannyStop) {
              DBG_NOTICE(0, "Nanny exiting, no daemon.");
              remove(pidfile);
              return 0;
            }
            else if (DriverNannyResume) {
              DriverNannySuspend=0;
              DriverNannyResume=0;
              DBG_NOTICE(0, "Resuming daemon");
              break;
            }
            else {
              DBG_NOTICE(0, "Daemon still suspended.");
              GWEN_Socket_Select(0, 0, 0, 300000);
            }
          }
          else {
            rv=wait(&status);
            if (rv==-1) {
              /* error */
              if (errno==EINTR) {
                if (DriverNannyStop || DriverNannySuspend) {
                  /* nanny is to stop or suspend, so terminate child, wait for
                   * its termination and if we are to stop then go down */
                  time_t startedToWait;
  
                  startedToWait=time(0);
                  DBG_NOTICE(0, "Terminating daemon.");
                  kill(childPID, SIGTERM);
                  /* wait for child to go down */
                  while(1) {
                    rv=wait(&status);
                    if (rv==-1) {
                      if (errno!=EINTR) {
                        DBG_ERROR(0, "Error while waiting for child's "
                                  "termination (%s).",
                                  strerror(errno));
                        remove(pidfile);
                        return RETURNVALUE_SETUP;
                      }
                    }
                    else
                      break;
                    if (difftime(time(0),startedToWait)>10) {
                      DBG_WARN(0, "Daemon seems to hang, have to kill it");
                      kill(childPID, SIGKILL);
                      break;
                    }
                  } /* while waiting for child to go down */
                  if (DriverNannySuspend) {
                    DBG_NOTICE(0, "Daemon suspended, "
                               "waiting for SIGCONT to resume.");
                  }
                  else {
                    DBG_NOTICE(0, "Daemon terminated, exiting.");
                    remove(pidfile);
                    return 0;
                  }
                }
              }
              else if (errno==ECHILD) {
                DBG_ERROR(0, "No child to wait for (%s), respawning.",
                          strerror(errno));
                break;
              }
              else {
                DBG_ERROR(0, "Error while waiting for child (%s).",
                          strerror(errno));
              }
            }
            else {
              /* ok, child exited */
              if (WIFEXITED(status)) {
                /* child exited normally */
                if (DriverNannySuspend) {
                  DBG_NOTICE(0, "Daemon suspended.");
                }
                else {
                  if (WEXITSTATUS(status)==RETURNVALUE_HANGUP) {
                    DBG_NOTICE(0, "Restarting daemon");
                    break;
                  }
                  else if ((WEXITSTATUS(status)==RETURNVALUE_SETUP ||
                            WEXITSTATUS(status)==RETURNVALUE_NOSTART) &&
                           args->exitOnSetupError) {
                    DBG_NOTICE(0, "Daemon setup error, exiting.");
                    remove(pidfile);
                    return WEXITSTATUS(status);
                  }
                  else if (WEXITSTATUS(status)==RETURNVALUE_SETUP ||
                           WEXITSTATUS(status)==RETURNVALUE_NOSTART) {
                    DBG_NOTICE(0, "Could not init daemon, suspended.");
                    DriverNannySuspend=1;
                    DriverNannyResume=0;
                  }
                  else {
                    DBG_NOTICE(0, "Daemon exited normally, exiting, too.");
                    remove(pidfile);
                    return WEXITSTATUS(status);
                  }
                }
              }
              else {
                /* child died unexpectedly */
                if (WIFSIGNALED(status)) {
                  time_t lft;
  
                  lft=LastFailedTime;
                  LastFailedTime=time(0);
  
                  DBG_ERROR(0, "Daemon died due to uncaught "
                            "signal %d.",
                            WTERMSIG(status));
  
                  /* check for respawn frequency, don't make the daemon
                   * eat all the processor power */
                  if (difftime(LastFailedTime, lft)<2) {
                    ShortFailCounter++;
                    if (ShortFailCounter>10) {
                      DBG_NOTICE(0, "Daemon's respawn frequency too high, "
                                 "suspended.");
                      DriverNannySuspend=1;
                      DriverNannyResume=0;
                    }
                    else if (ShortFailCounter>5) {
                      DBG_NOTICE(0, "Daemon dies to often, will wait");
                      GWEN_Socket_Select(0, 0, 0, 1000);
                    }
                    else {
                      DBG_NOTICE(0, "Respawning daemon.");
                    }
                  }
                  else {
                    /* respawn frequency ok, reset fail counter */
                    ShortFailCounter=0;
                    DBG_NOTICE(0, "Respawning daemon.");
                  }
                  /* DEBUG end */
                }
                else {
                  DBG_WARN(0, "Daemon died unexpectedly, respawning.\n");
                }
                /* break wait loop */
                break;
              }
            }
          }
        } /* while */
      }
      else {
        /* child process, break spawn loop */
        break;
      }
    } /* while */
  } /* if DaemonMode */
#endif /* ifdef HAVE_FORK */

  /* this is executed by the child */
  DBG_NOTICE(0, "Initializing daemon.");

#ifdef HAVE_SIGACTION
  if (setSignalHandler(1)) {
    DBG_ERROR(0, "Error setting up signal handler, aborting");
    return RETURNVALUE_SETUP;
  }
#endif

  db=GWEN_DB_Group_new("config");
  if (GWEN_DB_ReadFile(db,
                       args->configFile,
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr,I18N("Could not read configuration file, aborting.\n"));
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  enabled=GWEN_DB_GetIntValue(db, "enabled", 0, 0);
  if (!enabled) {
    DBG_NOTICE(0, "RemoteDriver daemon is disabled by config file, aborting.");
    DBG_NOTICE(0, "Please set \"enabled=1\" in config file to enable it.");
    fprintf(stderr,
	    I18N("RemoteDriver daemon is disabled by config file, aborting.\n"));
    GWEN_DB_Group_free(db);
    return RETURNVALUE_NOSTART;
  }

  /* init daemon */
  DBG_INFO(0, "Will now initialize daemon.");
  rv=initFromDb(db);
  if (rv) {
    DBG_ERROR(0, "Could not setup RemoteDriver (%d)", rv);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  /* loop */
  DBG_INFO(0, "Ready to run drivers.");
  loopCount=0;
  while (DriverDaemonStop==0 && DriverDaemonHangup==0) {
    DRIVER *d;

    d=Driver_List_First(DriverList);
    while(d) {
      int rv;

      rv=Driver_Check(d);
      if (rv) {
        DBG_INFO(0, "Error in driver %s", d->driverName);
      }
      d=Driver_List_Next(d);
    }
    /* wait for up to two seconds */
    sleep(2);
  } /* while */

  DBG_INFO(0, "Will now deinitialize server.\n");

  DBG_INFO(0, "RemoteDriver Daemon exiting.\n");
  if (DriverDaemonHangup)
    return RETURNVALUE_HANGUP;
  return 0;
}






int main(int argc, char **argv) {
  ARGUMENTS *args;
  int rv;
  GWEN_ERRORCODE err;
  FREEPARAM *param;
  const char *command;

  err=GWEN_Init();
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    return RETURNVALUE_SETUP;
  }

  args=Arguments_new();
  rv=checkArgs(args, argc, argv);
  if (rv==-1) {
    return 0;
  }
  if (rv) {
    return RETURNVALUE_PARAM;
  }

  if (GWEN_Logger_Open(0,
                       "chipcardrd",
                       args->logFile,
                       args->logType,
                       GWEN_LoggerFacilityDaemon)) {
    fprintf(stderr,I18N("Could not setup logging, aborting.\n"));
    Arguments_free(args);
    GWEN_Fini();
    return RETURNVALUE_SETUP;
  }
  GWEN_Logger_SetLevel(0, args->logLevel);
  GWEN_Logger_SetLevel(GWEN_LOGDOMAIN, GWEN_LoggerLevelNotice);

#ifdef HAVE_GETTEXT_ENVIRONMENT
  setlocale(LC_ALL,"");
  if (bindtextdomain("chipcardrd",  I18N_PATH)==0) {
    fprintf(stderr," Error bindtextdomain()\n");
  }
  if (textdomain("chipcardrd")==0) {
    fprintf(stderr," Error textdomain()\n");
  }
#endif

  param=args->params;
  if (!param)
    command="server";
  else
    command=param->param;

  if (strcasecmp(command, "server")==0)
    rv=server(args);
  else {
    DBG_ERROR(0, "Unknown command \"%s\"", command);
    rv=RETURNVALUE_PARAM;
  }
  GWEN_Logger_Close(0);
  err=GWEN_Fini();
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
  }
  Arguments_free(args);
  if (DriverDaemonHangup)
    return RETURNVALUE_HANGUP;
  return rv;
}






