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


#include "global.h"
#include <gwenhywfar/args.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/nettransportssl.h>



const GWEN_ARGS prg_args[]={
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "account",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "a",                          /* short option */
  "account",                    /* long option */
  "account",                    /* short description */
  "Account to be used (1-5)"    /* long description */
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "cryptkey",                   /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "c",                          /* short option */
  "cryptkey",                   /* long option */
  "Use crypt key",              /* short description */
  "If given then the crypt key is being referred to." /* long description */
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "signkey",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "s",                          /* short option */
  "signkey",                    /* long option */
  "Use sign key",               /* short description */
  "If given then the sign key is being referred to." /* long description */
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "userkey",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "u",                          /* short option */
  "userkey",                    /* long option */
  "Use user key",               /* short description */
  "If given then the user key is being referred to." /* long description */
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "bankkey",                    /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "b",                          /* short option */
  "bankkey",                    /* long option */
  "Use bank key",               /* short description */
  "If given then the bank key is being referred to." /* long description */
},
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
  "cardHolderPin",              /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "p",                          /* short option */
  "pin",                        /* long option */
  "Card holder PIN",            /* short description */
  "Card holder pin."            /* long description */
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeChar,            /* type */
  "devicePin",                  /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "P",                          /* short option */
  "dpin",                       /* long option */
  "Device PIN",                 /* short description */
  "Device pin. \n"
  "This is the special pin needed to modify data on the card."
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "forcepin",                   /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "F",                          /* short option */
  "force",                      /* long option */
  "Force the PIN verification", /* short description */
  "If given then the PIN verification will be performed even when its\n"
  "error counter reaches zero."
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "keypad",                     /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "k",                          /* short option */
  "keypad",                      /* long option */
  "Use the readers keypad.", /* short description */
  "Use the readers keypad for PIN verification."
},
{
  0,                            /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "initialDevicePin",           /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  "I",                          /* short option */
  "initialdevicepin",           /* long option */
  "Verify the initial device PIN.", /* short description */
  "Verify the initial device PIN.\n"
  "Most cards have the device PIN left to their initial state since the\n"
  "card is still protected by the card holder pin.\n"
  "So for most cards you can give this option instead of \"-P\""
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "keyNumber",                  /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  0,                            /* short option */
  "keynumber",                  /* long option */
  "Key number",                 /* short description */
  "Key number. \n"
  "This value is used with HBCI to identify a key (default=1)"
},
{
  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
  GWEN_ArgsTypeInt,             /* type */
  "keyVersion",                 /* name */
  0,                            /* minnum */
  1,                            /* maxnum */
  0,                            /* short option */
  "keyversion",                 /* long option */
  "Key version",                /* short description */
  "Key version. \n"
  "This value is used with HBCI to identify the version of a key (default=1)"
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


int verifyPin(LC_CARD *card, GWEN_DB_NODE *dbArgs, int pid) {
  LC_CLIENT_RESULT res;
  int maxErrors;
  int currentErrors;

  res=LC_Starcos_GetPinStatus(card, pid, &maxErrors, &currentErrors);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "GetPinStatus");
    return RETURNVALUE_WORK;
  }

  if (currentErrors!=maxErrors) {
    if (!GWEN_DB_VariableExists(dbArgs, "forcepin")) {
      fprintf(stderr,
              "WARNING: The error counter for the pin has not its inital\n"
              "value. Therefore I will not verify this pin unless the option\n"
              "\"-F\" is given.\n"
              "This is to protect your card against blocking.\n"
              "Please only use the \"-F\" option if you are sure that the\n"
              "pin is correct.\n");
      return RETURNVALUE_WORK;
    }
  }

  if (pid==0x90) {
    /* cardholder PIN */
    if (GWEN_DB_VariableExists(dbArgs, "keypad")) {
      fprintf(stderr, "Please enter the PIN into the card reader:\n");
      res=LC_Starcos_SecureVerifyPin(card, pid);
      if (res!=LC_Client_ResultOk) {
        showError(card, res, "SecureVerifyPin");
        return RETURNVALUE_WORK;
      }
      else {
        fprintf(stderr, "PIN ok.\n");
      }
    }
    else {
      const char *s;

      s=GWEN_DB_GetCharValue(dbArgs, "cardHolderPin", 0, 0);
      if (!s) {
        fprintf(stderr, "ERROR: Cardholder PIN missing. Use \"-p ARG\"\n");
        return RETURNVALUE_PARAM;
      }
      res=LC_Starcos_VerifyPin(card, pid, s);
      if (res!=LC_Client_ResultOk) {
        showError(card, res, "VerifyPin");
        return RETURNVALUE_WORK;
      }
      else {
        fprintf(stderr, "PIN ok.\n");
      }
    }
  }
  else {
    /* device pin */
    if (GWEN_DB_VariableExists(dbArgs, "initialDevicePin")) {
      res=LC_Starcos_VerifyInitialPin(card, pid);
      if (res!=LC_Client_ResultOk) {
        showError(card, res, "VerifyInitialPin");
        return RETURNVALUE_WORK;
      }
    }
    else {
      if (GWEN_DB_VariableExists(dbArgs, "keypad")) {
        fprintf(stderr, "Please enter the PIN into the card reader:\n");
        res=LC_Starcos_SecureVerifyPin(card, pid);
        if (res!=LC_Client_ResultOk) {
          showError(card, res, "SecureVerifyPin");
          return RETURNVALUE_WORK;
        }
        else {
          fprintf(stderr, "PIN ok.\n");
        }
      }
      else {
        const char *s;

        s=GWEN_DB_GetCharValue(dbArgs, "devicePin", 0, 0);
        if (!s) {
          fprintf(stderr,
                  "ERROR: Device PIN missing.\n"
                  "Use \"-P ARG\" or \"-I\"\n");
          return RETURNVALUE_PARAM;
        }
        res=LC_Starcos_VerifyPin(card, pid, s);
        if (res!=LC_Client_ResultOk) {
          showError(card, res, "VerifyPin");
          return RETURNVALUE_WORK;
        }
	else {
	  fprintf(stderr, "PIN ok.\n");
	}
      }
    }
  }

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
		      "rsacard",
		      GWEN_DB_GetCharValue(db, "logfile", 0, "rsacard.log"),
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

  cl=LC_Client_new("rsacard", "0.1", 0);
  if (LC_Client_ReadConfigFile(cl,
                               GWEN_DB_GetCharValue(db, "configfile",
                                                    0, 0))) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    GWEN_DB_Group_free(db);
    return RETURNVALUE_SETUP;
  }

  /* handle command */
  if (strcasecmp(s, "generatekey")==0) {
    rv=generateKey(cl, db);
  }
  else if (strcasecmp(s, "activatekey")==0) {
    rv=activateKey(cl, db);
  }
  else if (strcasecmp(s, "deactivatekey")==0) {
    rv=deactivateKey(cl, db);
  }
  else if (strcasecmp(s, "exportkey")==0) {
    rv=exportKey(cl, db);
  }
  else if (strcasecmp(s, "importkey")==0) {
    rv=importKey(cl, db);
  }
  else if (strcasecmp(s, "exportbankinfo")==0) {
    rv=exportBankInfo(cl, db);
  }
  else if (strcasecmp(s, "importbankinfo")==0) {
    rv=importBankInfo(cl, db);
  }
  else if (strcasecmp(s, "initialpin")==0) {
    rv=initialPin(cl, db);
  }
  else {
    fprintf(stderr, "Unknown command \"%s\"", s);
    rv=RETURNVALUE_PARAM;
  }

  LC_Client_free(cl);
  GWEN_DB_Group_free(db);
  return 0;
}








