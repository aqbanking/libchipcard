/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004-2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "global.h"
#include <gwenhywfar/args.h>
#include <gwenhywfar/cgui.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/url.h>
#include <gwenhywfar/ct.h>
#include <gwenhywfar/ctplugin.h>

#define PROGRAM_VERSION "0.1"


const GWEN_ARGS prg_args[]= {
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
    GWEN_ArgsType_Char,            /* type */
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
    GWEN_ArgsType_Char,            /* type */
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
    GWEN_ArgsType_Char,            /* type */
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
    GWEN_ArgsType_Char,            /* type */
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
    GWEN_ArgsType_Int,             /* type */
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
    GWEN_ArgsType_Int,             /* type */
    "help",                       /* name */
    0,                            /* minnum */
    0,                            /* maxnum */
    "h",                          /* short option */
    "help",                       /* long option */
    "Show help",                  /* short description */
    "Shows this help."            /* long description */
  }
};




void showError(LC_CARD *card, LC_CLIENT_RESULT res, const char *x)
{
  const char *s;

  switch (res) {
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
  if (card && res==LC_Client_ResultCmdError) {
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


int main(int argc, char **argv)
{
  int rv;
  GWEN_DB_NODE *db;
  const char *s;
  const char *cmd;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;
  GWEN_GUI *gui;


  gui=GWEN_Gui_CGui_new();
  GWEN_Gui_SetGui(gui);

  db=GWEN_DB_Group_new("arguments");
  rv=GWEN_Args_Check(argc, argv, 1,
                     GWEN_ARGS_MODE_ALLOW_FREEPARAM |
                     GWEN_ARGS_MODE_STOP_AT_FREEPARAM,
                     prg_args,
                     db);
  if (rv==GWEN_ARGS_RESULT_HELP) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
    GWEN_Buffer_AppendString(ubuf,
                             I18N("Usage: "));
    GWEN_Buffer_AppendString(ubuf, argv[0]);
    GWEN_Buffer_AppendString(ubuf,
                             I18N(" [GLOBAL OPTIONS] COMMAND"
                                  " [LOCAL OPTIONS]\n"));
    if (GWEN_Args_Usage(prg_args, ubuf, GWEN_ArgsOutType_Txt)) {
      fprintf(stderr, "Could not generate help string.\n");
      GWEN_Buffer_free(ubuf);
      return RETURNVALUE_PARAM;
    }
    GWEN_Buffer_AppendString(ubuf,
                             I18N("\nCommands:\n\n"));
    GWEN_Buffer_AppendString(ubuf,
                             I18N("  getkey:\n"
                                  "    get public part of a rsa key\n\n"));
    GWEN_Buffer_AppendString(ubuf,
                             I18N("  shownotepad:\n"
                                  "    show HBCI info from the NOTEPAD\n\n"));
    GWEN_Buffer_AppendString(ubuf,
                             I18N("  resetptc:\n"
                                  "    reset pin try counter\n\n"));
    fprintf(stdout, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }
  if (rv<1) {
    fprintf(stderr, "ERROR: Error in argument list (%d)\n", rv);
    return RETURNVALUE_PARAM;
  }
  /* get command */
  if (rv) {
    argc-=rv-1;
    argv+=rv-1;
  }

  /*GWEN_Logger_SetLevel(AQHBCI_LOGDOMAIN, GWEN_LoggerLevelInfo); */
  cmd=GWEN_DB_GetCharValue(db, "params", 0, 0);
  if (!cmd) {
    fprintf(stderr, "ERROR: Command needed.\n");
    GWEN_DB_Group_free(db);
    return 1;
  }

  /* setup logging */
  s=GWEN_DB_GetCharValue(db, "loglevel", 0, "warning");
  logLevel=GWEN_Logger_Name2Level(s);
  if (logLevel==GWEN_LoggerLevel_Unknown) {
    fprintf(stderr, "ERROR: Unknown log level (%s)\n", s);
    return RETURNVALUE_PARAM;
  }
  s=GWEN_DB_GetCharValue(db, "logtype", 0, "console");
  logType=GWEN_Logger_Name2Logtype(s);
  if (logType==GWEN_LoggerType_Unknown) {
    fprintf(stderr, "ERROR: Unknown log type (%s)\n", s);
    return RETURNVALUE_PARAM;
  }
  rv=GWEN_Logger_Open(GWEN_LOGDOMAIN,
                      "zkacard-tool",
                      GWEN_DB_GetCharValue(db, "logfile", 0, "zkacard-tool.log"),
                      logType,
                      GWEN_LoggerFacility_User);
  if (rv) {
    fprintf(stderr, "ERROR: Could not setup logging (%d).\n", rv);
    return RETURNVALUE_SETUP;
  }
  GWEN_Logger_SetLevel(GWEN_LOGDOMAIN, logLevel);

  /* handle command */

  if (strcasecmp(cmd, "getkey")==0) {
    rv=getPublicKey(db, argc, argv);
  }
  else if (strcasecmp(cmd, "shownotepad")==0) {
    rv=showNotepad(db, argc, argv);
  }
  else if (strcasecmp(cmd, "resetptc")==0) {
    rv=resetPtc(db, argc, argv);
  }
  else {
    fprintf(stderr, "Unknown command \"%s\"\n", s);
    rv=RETURNVALUE_PARAM;
  }




  GWEN_DB_Group_free(db);
  return 0;
}








