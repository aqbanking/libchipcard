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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef BUILDING_LIBCHIPCARD2_DLL

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "global.h"
#include <gwenhywfar/args.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/nettransportssl.h>

#define I18N(msg) msg


#define PROGRAM_VERSION "1.9"

#define k_PRG_VERSION_INFO \
    "kvkcard v1.9  (part of libchipcard v"k_CHIPCARD_VERSION_STRING")\n"\
    "(c) 2003 Martin Preuss<martin@libchipcard.de>\n" \
    "This program is free software licensed under GPL.\n"\
    "See COPYING for details.\n"


const GWEN_ARGS prg_args[]={
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "configfile",                 /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "C",                          /* short option */
  "configfile",                 /* long option */
  "Configuration file to load", /* short description */
  "Libchipcard2 configuration file to load. Uses the system file if omitted." /* long description */
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "logtype",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  0,                            /* short option */
  "logtype",                    /* long option */
  "Set the logtype",            /* short description */
  "Set the logtype (console, file)."
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "loglevel",                   /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  0,                            /* short option */
  "loglevel",                   /* long option */
  "Set the log level",          /* short description */
  "Set the log level (info, notice, warning, error)."
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "logfile",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  0,                            /* short option */
  "logfile",                   /* long option */
  "Set the log file",          /* short description */
  "Set the log file (if log type is \"file\")."
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "verbosity",                  /* name */
  0,                            /* minnum */
  10,                           /* maxnum */
  "v",                          /* short option */
  "verbous",                    /* long option */
  "Increase the verbosity",     /* short description */
  "Every occurrence of this option increases the verbosity."
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "filename",                   /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "f",                          /* short option */
  "filename",                   /* long option */
  "File to write to (stdout if omitted)",    /* short description */
  "File to write to. If omitted stdout will be used."
},
{
  GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "help",                       /* name */
  0,                            /* minnum */
  0,                            /* maxnum */
  "h",                          /* short option */
  "help",                       /* long option */
  "Show help",                  /* short description */
  "Shows this help."            /* long description */
  }
};


void usage(const char *name, const char *ustr) {
  fprintf(stderr,
          I18N("KVKCard2 - A tool to read information from a German medical card.\n"
               "(c) 2004 Martin Preuss<martin@libchipcard.de>\n"
               "This library is free software; you can redistribute it and/or\n"
               "modify it under the terms of the GNU Lesser General Public\n"
               "License as published by the Free Software Foundation; either\n"
               "version 2.1 of the License, or (at your option) any later version.\n"
               "\n"
               "Usage: %s COMMAND [OPTIONS]\n"
               "\n"
               "Available commands:\n"
               "  read  : read data from a German medical card\n"
               "\n"
               "Available options:\n"
               "%s\n"),
          name,
          ustr);
}



void showError(LC_CARD *card, LC_CLIENT_RESULT res, const char *x) {
  const char *s;

  switch(res) {
  case LC_Client_ResultOk:
    s="Ok.";
    break;
  case LC_Client_ResultWait:
    s="Timeout.";
    break;
  case LC_Client_ResultIpcError:
    s="IPC error.";
    break;
  case LC_Client_ResultCmdError:
    s="Command error.";
    break;
  case LC_Client_ResultDataError:
    s="Data error.";
    break;
  case LC_Client_ResultAborted:
    s="Aborted.";
    break;
  case LC_Client_ResultInvalid:
    s="Invalid argument to command.";
    break;
  case LC_Client_ResultInternal:
    s="Internal error.";
    break;
  case LC_Client_ResultGeneric:
    s="Generic error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  fprintf(stderr, "Error in \"%s\": %s\n", x, s);
  if (res==LC_Client_ResultCmdError) {
    int sw1;
    int sw2;

    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    fprintf(stderr, "  Last card command result:\n");
    if (sw1!=-1 && sw2!=-1)
      fprintf(stderr, "   SW1=%02x, SW2=%02x\n", sw1, sw2);
    s=LC_Card_GetLastResult(card);
    if (s)
      fprintf(stderr, "   Result: %s\n", s);
    s=LC_Card_GetLastText(card);
    if (s)
      fprintf(stderr, "   Text  : %s\n", s);
  }
}



int kvkRead(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  int rv;
  int v;
  const char *fname;
  GWEN_BUFFEREDIO *bio;
  GWEN_DB_NODE *dbData;
  GWEN_ERRORCODE err;
  int fd;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  if (v>1)
    fprintf(stderr, "Connecting to server.\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StartWait");
    return RETURNVALUE_WORK;
  }
  if (v>1)
    fprintf(stderr, "Connected.\n");

  if (v>1)
    fprintf(stderr, "Please insert your German medical card.\n");
  card=LC_Client_WaitForNextCard(cl, 30);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    LC_Client_StopWait(cl);
    return RETURNVALUE_WORK;
  }

  /* extend card */
  rv=LC_KVKCard_ExtendCard(card);
  if (rv) {
    fprintf(stderr, "Could not extend card as German medical card\n");
    return RETURNVALUE_WORK;
  }

  /* stop waiting */
  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
    return RETURNVALUE_WORK;
  }

  /* open card */
  if (v>0)
    fprintf(stderr, "Opening card.\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr,
            "ERROR: Error executing command CardOpen (%d).\n",
            res);
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card is a German medical card as expected.\n");

  dbData=LC_KVKCard_GetCardData(card);
  if (!dbData) {
    fprintf(stderr, "ERROR: No card data available.\n");
    LC_Card_Close(card);
    return RETURNVALUE_WORK;
  }

  /* open file */
  if (v>0)
    fprintf(stderr, "Writing data to file\n");
  fname=GWEN_DB_GetCharValue(dbArgs, "fileName", 0, 0);
  if (fname==0) {
    fd=1;
  }
  else {
#ifdef OS_WIN32
    fd=open(fname,
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR|S_IWUSR);
#else
    fd=open(fname,
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
#endif
  }
  if (fd==-1) {
    fprintf(stderr,
            I18N("ERROR: Could not open file (%s).\n"),
            strerror(errno));
    LC_Card_Close(card);
    LC_Card_free(card);
    return RETURNVALUE_WORK;
  }
  bio=GWEN_BufferedIO_File_new(fd);
  assert(bio);
  GWEN_BufferedIO_SetWriteBuffer(bio, 0, 1024);
  if (fname)
    GWEN_BufferedIO_AddFlags(bio, GWEN_BUFFEREDIO_FLAGS_CLOSE);
  else
    GWEN_BufferedIO_SubFlags(bio, GWEN_BUFFEREDIO_FLAGS_CLOSE);

  /* write to file */
  if (GWEN_DB_WriteToStream(dbData, bio, GWEN_DB_FLAGS_DEFAULT)) {
    fprintf(stderr, "ERROR: Could not write to file.\n");
    GWEN_BufferedIO_Abandon(bio);
    LC_Card_Close(card);
    LC_Card_free(card);
    return RETURNVALUE_WORK;
  }

  /* close file */
  err=GWEN_BufferedIO_Close(bio);
  GWEN_BufferedIO_free(bio);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    LC_Card_Close(card);
    LC_Card_free(card);
    return RETURNVALUE_WORK;
  }
  if (v>1)
    fprintf(stderr, "Data written.\n");

  /* close card */
  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    LC_Card_free(card);
    return RETURNVALUE_WORK;
  }
  else
    if (v>1)
      fprintf(stderr, "Card closed.\n");

  LC_Card_free(card);

  /* finished */
  if (v>1)
    fprintf(stderr, "Finished.\n");
  return 0;
}



GWEN_NETTRANSPORTSSL_ASKADDCERT_RESULT _askAddCert(GWEN_NETTRANSPORT *tr,
                                                   GWEN_DB_NODE *cert){
  return GWEN_NetTransportSSL_AskAddCertResultTmp;
}



int main(int argc, char **argv) {
  int rv;
  GWEN_DB_NODE *db;
  const char *s;
  LC_CLIENT *cl;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;

  GWEN_NetTransportSSL_SetAskAddCertFn(_askAddCert);

  db=GWEN_DB_Group_new("arguments");
  rv=GWEN_Args_Check(argc, argv, 1,
                     GWEN_ARGS_MODE_ALLOW_FREEPARAM,
                     prg_args,
                     db);
  if (rv==-2) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 256, 0, 1);
    if (GWEN_Args_Usage(prg_args, ubuf, GWEN_ArgsOutTypeTXT)) {
      fprintf(stderr, "Could not generate usage string.\n");
      GWEN_Buffer_free(ubuf);
      return RETURNVALUE_PARAM;
    }
    usage(argv[0], GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }
  if (rv<1) {
    fprintf(stderr, "ERROR: Error in argument list (%d)\n", rv);
    return RETURNVALUE_PARAM;
  }

  /* setup logging */
  s=GWEN_DB_GetCharValue(db, "loglevel", 0, "warning");
  logLevel=GWEN_Logger_Name2Level(s);
  if (logLevel==GWEN_LoggerLevelUnknown) {
    fprintf(stderr, "ERROR: Unknown log level (%s)\n", s);
    return RETURNVALUE_PARAM;
  }
  s=GWEN_DB_GetCharValue(db, "logtype", 0, "console");
  logType=GWEN_Logger_Name2Logtype(s);
  if (logType==GWEN_LoggerTypeUnknown) {
    fprintf(stderr, "ERROR: Unknown log type (%s)\n", s);
    return RETURNVALUE_PARAM;
  }
  rv=GWEN_Logger_Open(LC_LOGDOMAIN,
                      "kvkcard2",
		      GWEN_DB_GetCharValue(db, "logfile", 0, "kvkcard2.log"),
		      logType,
		      GWEN_LoggerFacilityUser);
  if (rv) {
    fprintf(stderr, "ERROR: Could not setup logging (%d).\n", rv);
    return RETURNVALUE_SETUP;
  }
  GWEN_Logger_SetLevel(LC_LOGDOMAIN, logLevel);

  /* get command */
  s=GWEN_DB_GetCharValue(db, "params", 0, 0);
  if (!s) {
    fprintf(stderr, "No command given.\n");
    GWEN_DB_Group_free(db);
    return RETURNVALUE_PARAM;
  }

  cl=LC_Client_new("kvkcard2", PROGRAM_VERSION, 0);
  if (LC_Client_ReadConfigFile(cl,
                               GWEN_DB_GetCharValue(db, "configfile",
                                                    0, 0))) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  /* handle command */
  if (strcasecmp(s, "read")==0) {
    rv=kvkRead(cl, db);
  }
  else {
    fprintf(stderr, "Unknown command \"%s\"", s);
    rv=RETURNVALUE_PARAM;
  }

  LC_Client_free(cl);
  GWEN_DB_Group_free(db);
  return 0;
}








