/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Fri Dec 13 2002
 copyright   : (C) 2002 by Martin Preuss
 email       : martin@libchipcard.de

 ****************************************************************************
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA *
 ****************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define I18N(m) m
#define I18NT(m) m

#include <stdio.h>
#include <errno.h>
#include <cctype>
#include <string>

#include <gwenhywfar/logger.h>
#include <gwenhywfar/args.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/text.h>

#include <chipcard2-client/chipcard2.h>
#include <chipcard2-client/client/client.h>

using namespace std;

#define k_PRG_VERSION_INFO \
    "cardcommander v0.3  (part of libchipcard v"k_CHIPCARD_VERSION_STRING")\n"\
    "(c) 2004 Martin Preuss<martin@libchipcard.de>\n" \
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
          I18N("CardCommander - A command line tool to manipulate a chip card.\n"
               "(c) 2003,2004 Martin Preuss<martin@libchipcard.de>\n"
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



int execCommand(GWEN_DB_NODE *dbArgs,
                LC_CLIENT *cl,
                LC_CARD **card,
                const string &cmd) {
  unsigned int i;
  string cm;
  static string lastAPDU;
  LC_CARD *tcard;
  LC_CLIENT_RESULT res;

  // skip leading blanks
  i=0;
  while(i<cmd.length()) {
    if (!isspace(cmd[i]))
      break;
    i++;
  } //while

  // now read command
  while(i<cmd.length()) {
    if (isspace(cmd[i]))
      break;
    cm+=cmd[i];
    i++;
  } //while

  // ok, we have the command now, check it
  if (strcasecmp(cm.c_str(), "open")==0) {
    // open card

    if (*card) {
      fprintf(stderr, I18N("Card is already open, try \"close\" first.\n"));
      return 3;
    }

    fprintf(stdout, I18N("Waiting for a card to be inserted...\n"));
    tcard=LC_Client_WaitForNextCard(cl, 20);
    if (!tcard) {
      fprintf(stderr, I18N("No card.\n"));
      return 3;
    }
    *card=tcard;

    res=LC_Card_Open(*card);
    if (res!=LC_Client_ResultOk) {
      showError(*card, res, "CardOpen");
      return 3;
    }
    fprintf(stdout, I18N("Card is open, info follows:\n"));
    LC_Card_Dump(*card, stderr, 2);
  }
  else if (strcasecmp(cm.c_str(), "close")==0) {
    if (!*card) {
      fprintf(stderr, I18N("Card is not open, try \"open\" first.\n"));
      return 3;
    }

    fprintf(stdout, I18N("Closing card.\n"));
    res=LC_Card_Close(*card);
    if (res!=LC_Client_ResultOk) {
      showError(*card, res, "CardClose");
      LC_Card_free(*card);
      *card=0;
      return 3;
    }
    LC_Card_free(*card);
    *card=0;
  }
  else if (strcasecmp(cm.c_str(), "apdu")==0 ||
	   strcasecmp(cm.c_str(), "last")==0) {
    GWEN_BUFFER *abuf;
    GWEN_BUFFER *rbuf;

    if (!*card) {
      fprintf(stderr, I18N("Card is not open, try \"open\" first.\n"));
      return 3;
    }

    abuf=GWEN_Buffer_new(0, 32, 0, 1);
    if (strcasecmp(cm.c_str(), "apdu")==0) {
      if (GWEN_Text_FromHexBuffer(cmd.substr(i).c_str(), abuf)) {
	fprintf(stderr,I18N("Only hex bytes are allowed.\n"));
	GWEN_Buffer_free(abuf);
	return 2;
      }
      fprintf(stdout, I18N("Sending APDU:\n"));
    }
    else {
      if (lastAPDU.empty()) {
	fprintf(stderr, I18N("No last APDU.\n"));
	return 2;
      }
      GWEN_Buffer_AppendBytes(abuf,
			      lastAPDU.data(),
                              lastAPDU.length());
      fprintf(stdout, I18N("Resending APDU:\n"));
    }
    if (GWEN_Buffer_GetUsedBytes(abuf)<4) {
      fprintf(stderr,I18N("An APDU needs at least 4 bytes.\n"));
      GWEN_Buffer_free(abuf);
      return 2;
    }
    GWEN_Text_DumpString(GWEN_Buffer_GetStart(abuf),
			 GWEN_Buffer_GetUsedBytes(abuf),
			 stdout, 2);

    lastAPDU=string(GWEN_Buffer_GetStart(abuf),
		    GWEN_Buffer_GetUsedBytes(abuf));
    rbuf=GWEN_Buffer_new(0, 256, 0, 1);
    res=LC_Card_ExecAPDU(*card, GWEN_Buffer_GetStart(abuf),
			 GWEN_Buffer_GetUsedBytes(abuf),
                         rbuf,
                         LC_Client_CmdTargetCard,
                         30);
    GWEN_Buffer_free(abuf);
    if (res!=LC_Client_ResultOk) {
      showError(*card, res, "ExecAPDU");
      GWEN_Buffer_free(rbuf);
      return 3;
    }

    fprintf(stdout, I18N("Result: %02x/%02x\nResponse: "),
	    LC_Card_GetLastSW1(*card),
	    LC_Card_GetLastSW2(*card));
    GWEN_Text_DumpString(GWEN_Buffer_GetStart(rbuf),
			 GWEN_Buffer_GetUsedBytes(rbuf),
			 stdout, 2);
    GWEN_Buffer_free(rbuf);
  }
  else if (strcasecmp(cm.c_str(), "info")==0) {
    if (!*card) {
      fprintf(stderr, I18N("Card is not open, try \"open\" first.\n"));
      return 3;
    }
    LC_Card_Dump(*card, stderr, 2);
  }
  else if (strcasecmp(cm.c_str(), "quit")==0 ||
           strcasecmp(cm.c_str(), "exit")==0) {
    if (*card) {
      fprintf(stdout,I18N("Closing card before exiting...\n"));
      res=LC_Card_Close(*card);
      if (res!=LC_Client_ResultOk) {
	showError(*card, res, "CardClose");
	LC_Card_free(*card);
	*card=0;
      }
      LC_Card_free(*card);
      *card=0;
    }
    fprintf(stdout,I18N("Exiting.\n"));
    return -1;
  }

  else if (strcasecmp(cm.c_str(), "help")==0 ||
           strcasecmp(cm.c_str(), "?")==0 ||
           strcasecmp(cm.c_str(), "h")==0) {
    fprintf(stdout,
	    I18N("List of commands:\n"
		 "open  - connects the card\n"
		 "close - disconnects the card \n"
		 "apdu xx xx xx xx [xx...] - sends a command to the card\n"
		 "info  - shows some information about the reader the \n"
		 "        currently open card is inserted in\n"
		 "help  - shows this little help screen\n"
		 "quit  - exits\n"
		)
	   );
  }
  else {
    fprintf(stderr,"Unknown command \"%s\"\n",cm.c_str());
    return 1;
  }
  return 0;
}





int main(int argc, char **argv) {
  int rv;
  string cmdstring;
  char *cmd;
  char buffer[256];
  LC_CLIENT *cl=0;
  LC_CARD *card=0;
  GWEN_DB_NODE *db;
  const char *s;
  GWEN_DB_NODE *dbConfig;
  GWEN_LOGGER_LOGTYPE logType;
  GWEN_LOGGER_LEVEL logLevel;
  LC_CLIENT_RESULT res;

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
      return 1;
    }
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }
  if (rv<1) {
    fprintf(stderr, "ERROR: Error in argument list (%d)\n", rv);
    return 1;
  }

  /* setup logging */
  s=GWEN_DB_GetCharValue(db, "loglevel", 0, "warning");
  logLevel=GWEN_Logger_Name2Level(s);
  if (logLevel==GWEN_LoggerLevelUnknown) {
    fprintf(stderr, "ERROR: Unknown log level (%s)\n", s);
    return 1;
  }
  s=GWEN_DB_GetCharValue(db, "logtype", 0, "console");
  logType=GWEN_Logger_Name2Logtype(s);
  if (logType==GWEN_LoggerTypeUnknown) {
    fprintf(stderr, "ERROR: Unknown log type (%s)\n", s);
    return 1;
  }
  rv=GWEN_Logger_Open(LC_LOGDOMAIN,
		      "cardcommander2",
		      GWEN_DB_GetCharValue(db, "logfile", 0,
					   "cardcommander2.log"),
		      logType,
		      GWEN_LoggerFacilityUser);
  if (rv) {
    fprintf(stderr, "ERROR: Could not setup logging (%d).\n", rv);
    return 2;
  }
  GWEN_Logger_SetLevel(LC_LOGDOMAIN, logLevel);

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
    return 2;
  }

  cl=LC_Client_new("cardcommander2", "0", 0);
  if (LC_Client_ReadConfig(cl, dbConfig)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    GWEN_DB_Group_free(dbConfig);
    GWEN_DB_Group_free(db);
    return 2;
  }
  GWEN_DB_Group_free(dbConfig); dbConfig=0;

  fprintf(stderr, "Connecting to server.\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StartWait");
    return 3;
  }
  fprintf(stderr, "Connected.\n");

  while(rv!=-1) {
    fprintf(stdout,"Card: ");
    cmd=fgets(buffer,sizeof(buffer),stdin);
    cmdstring=cmd;
    rv=execCommand(db, cl, &card, cmdstring);
  }

  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
  }

  LC_Client_free(cl);
  return rv;
}






