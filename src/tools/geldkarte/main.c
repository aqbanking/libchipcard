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

#include "global.h"
#include <gwenhywfar/args.h>
#include <gwenhywfar/db.h>

#define I18N(msg) msg


#define PROGRAM_VERSION "1.9"


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
  "Configuration file to load." /* long description */
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "file",                       /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "f",                          /* short option */
  "file",                       /* long option */
  "File name",                  /* short description */
  "File name. \n"
  "This filename is used when reading or writing data such as public keys,\n"
  "bank information etc."
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
    fprintf(stderr, "  Last card command result:\n");
    fprintf(stderr, "   SW1=%02x, SW2=%02x\n",
            LC_Card_GetLastSW1(card),
            LC_Card_GetLastSW2(card));
    s=LC_Card_GetLastResult(card);
    if (s)
      fprintf(stderr, "   Result: %s\n", s);
    s=LC_Card_GetLastText(card);
    if (s)
      fprintf(stderr, "   Text  : %s\n", s);
  }
}



int loaded(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  LC_GELDKARTE_VALUES *values;
  int rv;
  int v;

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

  if (v>0)
    fprintf(stderr, "Waiting for card...\n");
  card=LC_Client_WaitForNextCard(cl, 20);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Found a card.\n");

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as GELDKARTE card\n");
    return RETURNVALUE_WORK;
  }

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
    fprintf(stderr, "Card is a GELDKARTE card as expected.\n");

  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
    return RETURNVALUE_WORK;
  }

  values=LC_GeldKarte_Values_new();
  res=LC_GeldKarte_ReadValues(card, values);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ReadValues");
    rv=RETURNVALUE_WORK;
  }
  else
    rv=0;

  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card closed.\n");

  if (rv==0) {
    fprintf(stdout,
            I18N("Card is loaded with %6.2f %s\n"),
            LC_GeldKarte_Values_GetLoaded(values)/100.0,
            "EUR");
  }
  return rv;
}



int maxload(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  LC_GELDKARTE_VALUES *values;
  int rv;
  int v;

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

  if (v>0)
    fprintf(stderr, "Waiting for card...\n");
  card=LC_Client_WaitForNextCard(cl, 20);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Found a card.\n");

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as GELDKARTE card\n");
    return RETURNVALUE_WORK;
  }

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
    fprintf(stderr, "Card is a GELDKARTE card as expected.\n");

  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
    return RETURNVALUE_WORK;
  }

  values=LC_GeldKarte_Values_new();
  res=LC_GeldKarte_ReadValues(card, values);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ReadValues");
    rv=RETURNVALUE_WORK;
  }
  else
    rv=0;

  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card closed.\n");

  if (rv==0) {
    fprintf(stdout,
            I18N("Card can be loaded with %6.2f %s\n"),
            LC_GeldKarte_Values_GetMaxLoad(values)/100.0,
            "EUR");
  }
  return rv;
}



int maxxfer(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  LC_GELDKARTE_VALUES *values;
  int rv;
  int v;

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

  if (v>0)
    fprintf(stderr, "Waiting for card...\n");
  card=LC_Client_WaitForNextCard(cl, 20);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Found a card.\n");

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as GELDKARTE card\n");
    return RETURNVALUE_WORK;
  }

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
    fprintf(stderr, "Card is a GELDKARTE card as expected.\n");

  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
    return RETURNVALUE_WORK;
  }

  values=LC_GeldKarte_Values_new();
  res=LC_GeldKarte_ReadValues(card, values);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ReadValues");
    rv=RETURNVALUE_WORK;
  }
  else
    rv=0;

  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card closed.\n");

  if (rv==0) {
    fprintf(stdout,
            I18N("Card can transfer up to %6.2f %s\n"),
            LC_GeldKarte_Values_GetMaxXfer(values)/100.0,
            "EUR");
  }
  return rv;
}





int main(int argc, char **argv) {
  int rv;
  GWEN_DB_NODE *db;
  const char *s;
  LC_CLIENT *cl;
  GWEN_DB_NODE *dbConfig;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;

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
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
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
		      "geldkarte2",
		      GWEN_DB_GetCharValue(db, "logfile", 0, "geldkarte2.log"),
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

  dbConfig=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(dbConfig,
                       GWEN_DB_GetCharValue(db, "configfile", 0,
                                            LC_DEFAULT_DATADIR
                                            "/chipcardc2.conf"),
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    GWEN_DB_Group_free(dbConfig);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  cl=LC_Client_new("geldkarte2", PROGRAM_VERSION, 0);
  if (LC_Client_ReadConfig(cl, dbConfig)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    GWEN_DB_Group_free(dbConfig);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  GWEN_DB_Group_free(dbConfig); dbConfig=0;

  /* handle command */
  if (strcasecmp(s, "loaded")==0) {
    rv=loaded(cl, db);
  }
  else if (strcasecmp(s, "maxload")==0) {
    rv=maxload(cl, db);
  }
  else if (strcasecmp(s, "maxxfer")==0) {
    rv=maxxfer(cl, db);
  }
  else {
    fprintf(stderr, "Unknown command \"%s\"", s);
    rv=RETURNVALUE_PARAM;
  }

  LC_Client_free(cl);
  GWEN_DB_Group_free(db);
  return 0;
}







