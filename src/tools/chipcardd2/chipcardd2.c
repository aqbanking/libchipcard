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


#include "chipcardd2_p.h"

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

#ifdef OS_WIN32
# include <process.h> /* for getpid */
#endif


static int ChipcardDaemonStop=0;
static int ChipcardDaemonHangup=0;
static int ChipcardNannyStop=0;
static int ChipcardNannySuspend=0;
static int ChipcardNannyResume=0;
static time_t LastFailedTime=0;
static int ShortFailCounter=0;
static GWEN_NETTRANSPORTSSL_ASKADDCERT_RESULT
  Chipcard_AskAddCertResult=GWEN_NetTransportSSL_AskAddCertResultNo;

static LC_CARDSERVER *cardServer=0;


#define k_PRG "chipcardd2"
#define k_PRG_VERSION_INFO \
  "chipcardd v1.9.0 (part of libchipcard v"CHIPCARD_VERSION_STRING")\n"\
  "(c) 2004 Martin Preuss<martin@libchipcard.de>\n"\
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
#ifdef HAVE_FORK
  ar->daemonMode=1;
#endif
  ar->dataDir=LC_DEFAULT_DATADIR;
  ar->configFile=LC_DEFAULT_DATADIR "/chipcardd2.conf";
  ar->pidFile=LC_DEFAULT_PIDDIR "/chipcardd2.pid";
  ar->logFile=LC_DEFAULT_LOGDIR "/chipcardd2.log";
  ar->logLevel=GWEN_LoggerLevelNotice;
#ifdef HAVE_SYSLOG_H
  ar->logType=GWEN_LoggerTypeSyslog;
#else
  ar->logType=GWEN_LoggerTypeFile;
#endif
  ar->exitOnSetupError=0;
  ar->runOnce=0;

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
	  I18N("ChipCardDaemon2 - A daemon for chipcard access\n"
               "                 Part of LibChipCard "
               CHIPCARD_VERSION_STRING"\n"
	       "(c) 2004 Martin Preuss<martin@libchipcard.de>\n"
	       "This library is free software; you can redistribute it and/or\n"
	       "modify it under the terms of the GNU Lesser General Public\n"
	       "License as published by the Free Software Foundation; either\n"
	       "version 2.1 of the License, or (at your option) any later version.\n"
	       "\n"
	       "Usage:\n"
               k_PRG" COMMAND [OPTIONS]\n"
               "\nCommands:\n"
               " init\n"
               "  Creates a self-signed server certificate, Diffie-Hellman\n"
               "  key files etc. This is needed once before the first start\n"
               "  of the chipcard daemon.\n"
               " mkcert\n"
               "  Generates a self-signed certificate to be used by chipcard\n"
               "  clients.\n"
               " addreader\n"
               "  Adds a reader to the configuration file.\n"
               " delreader\n"
               "  Removes a reader from the configuration file\n"
               " testreader\n"
               "  Checks whether the given reader works.\n"
               "\n"
               "\nOptions:\n"
               " --accept-all-certs - accept all client certificates and\n"
               "                      trust them temporarily\n"
               " --store-all-certs  - store all client certificates and\n"
               "                      trust them permanently\n"
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
               " --exit-on-error  - makes chipcardd2 exit on setup errors\n"
               " --runonce ARG    - makes chipcardd2 exit after serving "
               "ARG clients\n"
               "\n"
               " The following options apply to commands init and mkcert:\n"
               " --certfile FILE  - name of the certificate file to write\n"
               " --dhfile FILE    - name of the DH parameter file to write\n"
               " --user NAME      - name of the certificate owner (you\n"
               "                    should use something like user@host)\n"
               " [--country ARG]  - country name to be stored in the cert\n"
               "                    (defaults to \"DE\")\n"
               "\n"
               "The following option apply to commands addreader, delreader\n"
               "and testreader:\n"
               "--rname ARG       - user defined name of the reader. This\n"
               "                    name is used to recognize the reader in\n"
               "                    commands.\n"
               "[--dtype ARG]     - driver type name (use \"list\" for a\n"
               "                    list of available types)\n"
               "                    (required for command \"addreader\")\n"
               "[--rtype ARG]     - reader type name (use \"list\" for a\n"
               "                    list of available types)\n"
               "                    (required for command \"addreader\")\n"
               "[--rport ARG]     - port to which the reader is connected.\n"
               "                    This is needed for readers connected to\n"
               "                    serial ports (COM1-COMx) when adding a\n"
               "                    reader using \"addreader\"\n"
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
    else if (strcmp(argv[i],"--runonce")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->runOnce=atoi(argv[i]);
    }

    else if (strcmp(argv[i],"--accept-all-certs")==0) {
      Chipcard_AskAddCertResult=GWEN_NetTransportSSL_AskAddCertResultTmp;
    }
    else if (strcmp(argv[i],"--store-all-certs")==0) {
      Chipcard_AskAddCertResult=GWEN_NetTransportSSL_AskAddCertResultIncoming;
    }
    /* options for mkcert, init */
    else if (strcmp(argv[i],"--certfile")==0) {
      i++;
      if (i>=argc)
	return RETURNVALUE_PARAM;
      args->certFile=argv[i];
    }
    else if (strcmp(argv[i],"--dhfile")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->dhFile=argv[i];
    }
    else if (strcmp(argv[i],"--country")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->countryName=argv[i];
    }
    else if (strcmp(argv[i],"--user")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->commonName=argv[i];
    }
    /* options for addreader, delreader, testreader */
    else if (strcmp(argv[i],"--dtype")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->dtype=argv[i];
    }
    else if (strcmp(argv[i],"--rtype")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->rtype=argv[i];
    }
    else if (strcmp(argv[i],"--rname")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->rname=argv[i];
    }
    else if (strcmp(argv[i],"--rport")==0) {
      i++;
      if (i>=argc)
        return RETURNVALUE_PARAM;
      args->rport=argv[i];
    }


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
#ifdef HAVE_FORK
    else if (strcmp(argv[i],"-f")==0) {
      args->daemonMode=0;
    }
#endif
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
      ChipcardDaemonStop=1;
    }
    else {
      DBG_NOTICE(0, "Nanny got an interrupt signal, will terminate child");
      ChipcardNannyStop=1;
    }
    break;

  case SIGTERM:
    if (child) {
      DBG_NOTICE(0, "Daemon got a termination signal");
      ChipcardDaemonStop=1;
    }
    else {
      DBG_NOTICE(0, "Nanny got a termination signal, will terminate child.");
      ChipcardNannyStop=1;
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
      ChipcardDaemonStop=1;
      ChipcardDaemonHangup=1;
    }
    else {
      if (ChipcardNannySuspend) {
	DBG_NOTICE(0, "Restarting daemon");
	ChipcardNannyResume=1;
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
#ifdef USR1_DUMPS
    if (cardServer)
      LC_CardServer_DumpState(cardServer);
    GWEN_MemoryDebug_Dump(GWEN_MEMORY_DEBUG_MODE_SHORT);
    GWEN_Net_Dump();
#else
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
      ChipcardNannySuspend=1;
    }
    break;
#endif

#ifdef SIGCONT
  case SIGCONT:
    if (!child && ChipcardNannySuspend) {
      DBG_NOTICE(0, "Resuming daemon");
      ChipcardNannyResume=1;
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
#endif



GWEN_NETTRANSPORTSSL_ASKADDCERT_RESULT
askAddCert(GWEN_NETTRANSPORT *tr,
           GWEN_DB_NODE *cert){
  return Chipcard_AskAddCertResult;
}



/* TODO */
int getPassword(GWEN_NETTRANSPORT *tr,
                char *buffer, int num,
                int rwflag){
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
	       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
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

  GWEN_NetTransportSSL_SetGetPasswordFn(getPassword);
  GWEN_NetTransportSSL_SetAskAddCertFn(askAddCert);

  pidfile=args->pidFile;

  DBG_NOTICE(0, "Chipcardd v"CHIPCARD_VERSION_FULL_STRING" started.");
#ifdef USE_LIBUSB
  DBG_NOTICE(0, "USB scanning supported (using LibUSB).");
#else
  DBG_WARNING(0, "USB scanning not supported (LibUSB not available).");
#endif

#ifdef HAVE_FORK
  if (args->daemonMode) {
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
  } /* if daemon mode */

  /* this will be executed by the child (or both if not in daemon mode) */
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
    ChipcardDaemonStop=0;
    ChipcardDaemonHangup=0;
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

	if (ChipcardNannySuspend) {
	  if (ChipcardNannyStop) {
	    DBG_NOTICE(0, "Nanny exiting, no daemon.");
            remove(pidfile);
            return 0;
          }
	  else if (ChipcardNannyResume) {
	    ChipcardNannySuspend=0;
	    ChipcardNannyResume=0;
	    DBG_NOTICE(0, "Resuming daemon");
	    break;
	  }
	  else {
            DBG_NOTICE(0, "Daemon still suspended.");
	    sleep(300);
	  }
	}
	else {
	  rv=wait(&status);
	  if (rv==-1) {
	    /* error */
	    if (errno==EINTR) {
	      if (ChipcardNannyStop || ChipcardNannySuspend) {
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
		if (ChipcardNannySuspend) {
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
	      if (ChipcardNannySuspend) {
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
		  ChipcardNannySuspend=1;
		  ChipcardNannyResume=0;
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
                if (args->daemonMode==0) {
                  remove(pidfile);
                  return 0;
                }

		/* check for respawn frequency, don't make the daemon
		 * eat all the processor power */
		if (difftime(LastFailedTime, lft)<2) {
		  ShortFailCounter++;
		  if (ShortFailCounter>10) {
		    DBG_NOTICE(0, "Daemon's respawn frequency too high, "
			       "suspended.");
		    ChipcardNannySuspend=1;
		    ChipcardNannyResume=0;
		  }
		  else if (ShortFailCounter>5) {
		    DBG_NOTICE(0, "Daemon dies to often, will wait");
		    sleep(1);
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
#endif /* ifdef HAVE_FORK */

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
    DBG_NOTICE(0, "ChipCard daemon is disabled by config file, aborting.");
    DBG_NOTICE(0, "Please set \"enabled=1\" in config file to enable it.");
    fprintf(stderr,
            I18N("ChipCard daemon is disabled by config file, aborting.\n"));
    GWEN_DB_Group_free(db);
    return RETURNVALUE_NOSTART;
  }

  if (args->runOnce) {
    DBG_NOTICE(0, "ChipCard daemon will only serve %d client(s).",
               args->runOnce);
    fprintf(stderr,
            I18N("ChipCard daemon will only serve %d client(s)."),
            args->runOnce);
  }

  DBG_INFO(0, "Will now initialize server.");
  cardServer=LC_CardServer_new(0);
  if (LC_CardServer_ReadConfig(cardServer, db)) {
    fprintf(stderr,I18N("Could not initialize server.\n"));
    LC_CardServer_free(cardServer);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }
  GWEN_DB_Group_free(db);

  /* loop */
  DBG_INFO(0, "Ready to service requests.");
  loopCount=0;
  while (ChipcardDaemonStop==0 && ChipcardDaemonHangup==0) {
    GWEN_NETCONNECTION_WORKRESULT res;
    int rv;

    while(1) {
      int clientsBefore;

      clientsBefore=LC_CardServer_GetClientCount(cardServer);
      rv=LC_CardServer_Work(cardServer);
      if (rv==-1) {
        DBG_INFO(0,
                 "ERROR: Error while working on hardware (%d)", rv);
        break;
      }
      else if (rv==1)
        break;
      if (args->runOnce && clientsBefore)
        if (LC_CardServer_GetClientCount(cardServer)==0) {
          loopCount++;
          LC_CardServer_DumpState(cardServer);
          GWEN_MemoryDebug_Dump(GWEN_MEMORY_DEBUG_MODE_SHORT);
          GWEN_Net_Dump();

          if (loopCount>=args->runOnce) {
            DBG_NOTICE(0, "%d client(s) served, going down.", loopCount);
            ChipcardDaemonStop=1;
            break;
          }
        }
    }
    res=GWEN_Net_HeartBeat(2000);
    if (res==GWEN_NetConnectionWorkResult_Error) {
      DBG_INFO(0, "ERROR: Error while working (%d)", res);
    }

  } /* while */

  DBG_INFO(0, "Will now deinitialize server.\n");
  LC_CardServer_free(cardServer);

  DBG_INFO(0, "Chipcard Daemon exiting.\n");
  if (ChipcardDaemonHangup)
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
                       "chipcardd",
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
  if (bindtextdomain("chipcardd",  I18N_PATH)==0) {
    fprintf(stderr," Error bindtextdomain()\n");
  }
  if (textdomain("chipcardd")==0) {
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
  else if (strcasecmp(command, "init")==0)
    rv=init(args);
  else if (strcasecmp(command, "mkcert")==0)
    rv=mkCert(args);
  else if (strcasecmp(command, "addreader")==0)
    rv=addReader(args);
  else if (strcasecmp(command, "delreader")==0)
    rv=delReader(args);
  else if (strcasecmp(command, "testreader")==0)
    rv=testReader(args);
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
  if (ChipcardDaemonHangup)
    return RETURNVALUE_HANGUP;
  return rv;
}






