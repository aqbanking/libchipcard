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


#include "driver_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/nl_socket.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_http.h>
#include <gwenhywfar/net2.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>

static GWEN_TYPE_UINT32 LCD_Driver__LastCardNum=0;


GWEN_INHERIT_FUNCTIONS(LCD_DRIVER)


void LCD_Driver_Usage(const char *prgName) {
  fprintf(stdout,
          "%s [OPTONS] \n"
          "[-v]               verbous\n"
          "[--logfile ARG]    name of the logfile\n"
          "[--logtype ARG]    log type\n"
          "[--loglevel ARG]   log level\n"
          "[-d ARG]           driver data folder\n"
          "-b ARG             server id\n"
          "[-u ARG]           customer id of this driver\n"
          "[-a ARG]           server IP address (or hostname)\n"
          "[-p ARG]           server TCP port\n"
          "-l ARG             name of the library driver file\n"
          "-i ARG             driver id for this session\n"
          "\n"
          "The following arguments are used in test/remote mode only\n"
          "--test             enter test mode, check for a given reader\n"
          "-rp ARG            reader port\n"
          "-rs ARG            reader slots\n"
          "-rn ARG            reader name\n"
          "--remote           driver is in remote mode\n"
          "[-rt ARG]          reader type (only for remote drivers)\n"
          "[-dt ARG]          driver type (only for remote drivers)\n"
          "--accept-all-certs accept all server certificates"
          , prgName
         );
}



LCD_DRIVER_CHECKARGS_RESULT LCD_Driver_CheckArgs(LCD_DRIVER *d,
                                               int argc, char **argv) {
  int i;

  assert(d);

  d->verbous=0;
  d->rslots=1;
  d->testMode=0;
  d->rport=0;
  d->rname=0;
  d->rtype=0;
  d->dtype=0;
  d->remoteMode=0;
  d->logType=GWEN_LoggerTypeConsole;
  d->logFile=strdup("driver.log");
  d->logLevel=GWEN_LoggerLevelNotice;
  d->serverPort=LC_DEFAULT_PORT;
  d->typ="local";
  d->certFile=0;
  d->certDir=0;
  d->serverAddr=0;
  d->acceptAllCerts=0;

  i=1;
  while (i<argc){
    if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      free(d->logFile);
      d->logFile=strdup(argv[i]);
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (d->logType==GWEN_LoggerTypeUnknown) {
        DBG_ERROR(0, "Unknown log type \"%s\"\n", argv[i]);
        return LCD_DriverCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (d->logLevel==GWEN_LoggerLevelUnknown) {
        DBG_ERROR(0, "Unknown log level \"%s\"\n", argv[i]);
        return LCD_DriverCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i], "--accept-all-certs")==0) {
      d->acceptAllCerts=1;
    }
    else if (strcmp(argv[i],"-d")==0) {
      i++;
      if (i>=argc)
        return -1;
      d->driverDataDir=argv[i];
    }
    else if (strcmp(argv[i],"-t")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->typ=argv[i];
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->driverId=argv[i];
    }
    else if (strcmp(argv[i],"-a")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->serverAddr=argv[i];
    }
    else if (strcmp(argv[i],"-p")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->serverPort=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-l")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->libraryFile=argv[i];
    }
    else if (strcmp(argv[i],"-c")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->certFile=argv[i];
    }
    else if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->certDir=argv[i];
    }
    else if (strcmp(argv[i],"--test")==0) {
      d->testMode=1;
    }
    else if (strcmp(argv[i],"-rn")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rname=argv[i];
    }
    else if (strcmp(argv[i],"-rp")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rport=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-rs")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rslots=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-rt")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rtype=argv[i];
    }
    else if (strcmp(argv[i],"-dt")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->dtype=argv[i];
    }
    else if (strcmp(argv[i],"--remote")==0) {
      DBG_ERROR(0, "Remote mode is on");
      d->remoteMode=1;
    }
    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      LCD_Driver_Usage(argv[0]);
      return LCD_DriverCheckArgsResultHelp;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      return LCD_DriverCheckArgsResultVersion;
    }
    else if (strcmp(argv[i],"-v")==0) {
      d->verbous=1;
    }
    else {
      DBG_ERROR(0, "Unknown argument \"%s\"", argv[i]);
      return LCD_DriverCheckArgsResultError;
    }
    i++;
  } /* while */

  /* check for missing arguments */
  if (!d->testMode) {
    if (!d->serverAddr) {
      DBG_ERROR(0, "Server address missing");
      return LCD_DriverCheckArgsResultError;
    }
    if (!d->driverId && !d->remoteMode) {
      DBG_ERROR(0, "Driver id missing");
      return LCD_DriverCheckArgsResultError;
    }
  }

  if (!d->libraryFile) {
    DBG_ERROR(0, "Name of driver library file missing");
    return LCD_DriverCheckArgsResultError;
  }

  if (d->logFile==0) {
    GWEN_BUFFER *mbuf;

    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(mbuf, "driver");
    GWEN_Buffer_AppendString(mbuf, ".log");
    d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
    GWEN_Buffer_free(mbuf);
  }

  if (d->logFile) {
    GWEN_BUFFER *mbuf;

    if (strstr(d->logFile, "@reader@")) {
      free(d->readerLogFile);
      d->readerLogFile=strdup(d->logFile);
      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LCD_Driver_ReplaceVar(d->logFile, "reader", "driver", mbuf);
      free(d->logFile);
      d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
      GWEN_Buffer_free(mbuf);
    }
  }

  if (d->remoteMode) {
    if (!d->rname ||
        !d->rtype ||
        !d->dtype) {
      DBG_ERROR(0, "Reader properties missing");
      return LCD_DriverCheckArgsResultError;
    }
  }

  /* now setup reader if in remote mode */
  if (d->remoteMode) {
    LCD_READER *r;

    /* setup remote mode */
    DBG_ERROR(0, "Creating reader...");
    r=LCD_Driver_CreateReader(d, 0,
                             d->rname,
                             d->rport, d->rslots,
                             d->rflags);
    if (!r) {
      DBG_ERROR(0, "Could not create reader, aborting.");
      return LCD_DriverCheckArgsResultError;
    }
    LCD_Reader_SetReaderType(r, d->rtype);
    LCD_Reader_SetDriversReaderId(r, ++(d->lastReaderId));
    LCD_Driver_AddReader(d, r);
  }

  return 0;
}



int LCD_Driver_Init(LCD_DRIVER *d, int argc, char **argv) {
  LCD_DRIVER_CHECKARGS_RESULT res;
  GWEN_NETLAYER *nl;
  GWEN_NETLAYER *nlBase;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  GWEN_TYPE_UINT32 sid;
  GWEN_URL *url;

  res=LCD_Driver_CheckArgs(d, argc, argv);
  if (res!=LCD_DriverCheckArgsResultOk) {
    return -1;
  }

  url=GWEN_Url_fromString(d->serverAddr);
  if (!url) {
    DBG_ERROR(GWEN_LOGDOMAIN, "Bad URL: %s", d->serverAddr);
    return -1;
  }

  if (GWEN_Directory_GetPath(d->logFile,
                             GWEN_PATH_FLAGS_VARIABLE)) {
    DBG_ERROR(0, "Could not create log file for driver ");
    GWEN_Logger_Open(0, "driver",
                     0,
                     GWEN_LoggerTypeConsole,
                     GWEN_LoggerFacilityUser);
  }
  else {
    GWEN_Logger_Open(0, "driver",
                     d->logFile,
                     d->logType,
                     GWEN_LoggerFacilityUser);
  }
  GWEN_Logger_SetLevel(0, d->logLevel);

  if (!d->testMode) {
    DBG_NOTICE(0, "Starting driver \"%s\" with lowlevel \"%s\"",
               argv[0], d->libraryFile);
    if (d->driverDataDir) {
      if (chdir(d->driverDataDir)) {
        DBG_WARN(0, "chdir(%s): %s", d->driverDataDir, strerror(errno));
      }
    }

    d->ipcManager=GWEN_IpcManager_new();

    if (strcasecmp(d->typ, "local")==0) {
      /* HTTP over UDS */
      sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);
    }
    else if (strcasecmp(d->typ, "public")==0) {
      /* HTTP over TCP */
      sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);
    }
    else {
      sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);

      if (strcasecmp(d->typ, "private")==0) {
	/* HTTP over SSL */
        nl=GWEN_NetLayerSsl_new(nlBase,
                                d->certDir,
                                0,
                                d->certFile,
                                0,
                                0);
        GWEN_NetLayer_free(nlBase);
        GWEN_NetLayerSsl_SetAskAddCertFn(nl, LCD_Driver_AskAddCert, (void*)d);
        nlBase=nl;
      }
      else if (strcasecmp(d->typ, "secure")==0) {
	/* HTTP over SSL with certificates */
        nl=GWEN_NetLayerSsl_new(nlBase,
                                d->certDir,
                                0,
                                d->certFile,
                                0,
                                1);
        GWEN_NetLayer_free(nlBase);
        nlBase=nl;
      }
      else {
	DBG_ERROR(0, "Unknown mode \"%s\"", d->typ);
        GWEN_InetAddr_free(addr);
        GWEN_Url_free(url);
        return -1;
      }
    }
    GWEN_InetAddr_free(addr);

    nl=GWEN_NetLayerHttp_new(nlBase);
    GWEN_NetLayer_free(nlBase);
    GWEN_NetLayerHttp_SetOutCommand(nl, "POST", url);
    GWEN_Url_free(url);

    sid=GWEN_IpcManager_AddClient(d->ipcManager,
                                  nl,
                                  LCD_DRIVER_MARK_DRIVER);
    if (sid==0) {
      DBG_ERROR(0, "Could not add IPC client");
      return -1;
    }

    d->ipcId=sid;
    DBG_INFO(0, "IPC stuff initialized");
  }
  return 0;
}



LCD_DRIVER *LCD_Driver_new() {
  LCD_DRIVER *d;

  GWEN_NEW_OBJECT(LCD_DRIVER, d);
  GWEN_INHERIT_INIT(LCD_DRIVER, d);
  d->readers=LCD_Reader_List_new();

  return d;
}



void LCD_Driver_free(LCD_DRIVER *d) {
  if (d) {
    GWEN_INHERIT_FINI(LCD_DRIVER, d);
    LCD_Reader_List_free(d->readers);
    GWEN_IpcManager_free(d->ipcManager);
    free(d->logFile);
    free(d->readerLogFile);
    GWEN_FREE_OBJECT(d);
  }
}



int LCD_Driver_IsTestMode(const LCD_DRIVER *d) {
  assert(d);
  return d->testMode;
}



int LCD_Driver_Test(LCD_DRIVER *d) {
  LCD_READER *r;
  GWEN_TYPE_UINT32 res;

  assert(d);
  if (!d->testMode) {
    DBG_ERROR(0, "Not in test mode");
    return -1;
  }

  r=LCD_Driver_CreateReader(d,
                           1,
                           d->rname,
                           d->rport,
                           d->rslots,
                           0);
  assert(r);
  fprintf(stdout, "Connecting reader...\n");
  res=LCD_Driver_ConnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not connect reader (%s)\n",
	    LCD_Driver_GetErrorText(d, res));
    LCD_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader connected.\n");

  fprintf(stdout, "Disconnecting reader...\n");
  res=LCD_Driver_DisconnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not disconnect reader (%s)\n",
	    LCD_Driver_GetErrorText(d, res));
    LCD_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader disconnected.\n");

  fprintf(stdout, "Reader is available.\n");
  LCD_Reader_free(r);
  return 0;
}



const char *LCD_Driver_GetDriverDataDir(const LCD_DRIVER *d){
  assert(d);
  return d->driverDataDir;
}



const char *LCD_Driver_GetLibraryFile(const LCD_DRIVER *d){
  assert(d);
  return d->libraryFile;
}



const char *LCD_Driver_GetDriverId(const LCD_DRIVER *d){
  assert(d);
  return d->driverId;
}



GWEN_TYPE_UINT32 LCD_Driver_SendCommand(LCD_DRIVER *d,
                                       GWEN_DB_NODE *dbCommand) {
  return GWEN_IpcManager_SendRequest(d->ipcManager,
                                     d->ipcId, dbCommand);
}



int LCD_Driver_SendResponse(LCD_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand) {
  return GWEN_IpcManager_SendResponse(d->ipcManager,
                                      rid, dbCommand);
}



int LCD_Driver_SendResult(LCD_DRIVER *d,
                         GWEN_TYPE_UINT32 rid,
                         const char *name,
                         const char *code,
                         const char *text){
  GWEN_DB_NODE *db;

  db=GWEN_DB_Group_new(name);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                       "code", code);
  if (text)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "text", text);
  return LCD_Driver_SendResponse(d, rid, db);
}



GWEN_TYPE_UINT32 LCD_Driver_GetNextInRequest(LCD_DRIVER *d) {
  assert(d);
  return GWEN_IpcManager_GetNextInRequest(d->ipcManager,
                                          LCD_DRIVER_MARK_DRIVER);
}



GWEN_DB_NODE *LCD_Driver_GetInRequestData(LCD_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid) {
  assert(d);
  return GWEN_IpcManager_GetInRequestData(d->ipcManager, rid);
}



int LCD_Driver__Work(LCD_DRIVER *d, int timeout){
  GWEN_NETLAYER_RESULT res;
  int rv;

  if (!GWEN_Net_HasActiveConnections()) {
    DBG_ERROR(0, "No active connections, stopping");
    return -1;
  }

  res=GWEN_Net_HeartBeat(timeout);
  if (res==GWEN_NetLayerResult_Error) {
    DBG_ERROR(0, "Network error");
    return -1;
  }
  else if (res==GWEN_NetLayerResult_Idle) {
    DBG_VERBOUS(0, "No activity");
  }

  while(1) {
    DBG_DEBUG(0, "Driver: Working");
    /* activity detected, work with it */
    rv=GWEN_IpcManager_Work(d->ipcManager);
    if (rv==-1) {
      DBG_ERROR(0, "Error while working with IPC");
      return -1;
    }
    else if (rv==1)
      break;
  }

  return 0;
}



int LCD_Driver_CheckResponses(GWEN_DB_NODE *db) {
  const char *name;

  if (strcasecmp(GWEN_DB_GroupName(db), "error")==0) {
    int numCode;
    const char *txt;

    numCode=GWEN_DB_GetIntValue(db, "code", 0, LC_ERROR_GENERIC);
    txt=GWEN_DB_GetCharValue(db, "text", 0, "<empty>");
    DBG_ERROR(0, "Error %d: %s", numCode, txt);
    return numCode;
  }

  name=GWEN_DB_GetCharValue(db, "ipc/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC message (no command)");
    return -1;
  }

  if (strcasecmp(name, "Error")==0) {
    int numCode;

    numCode=GWEN_DB_GetIntValue(db, "data/code", 0, -1);
    if (numCode==-1) {
      const char *code;

      code=GWEN_DB_GetCharValue(db, "data/code", 0, "ERROR");
      if (strcasecmp(code, "OK")!=0)
        numCode=LC_ERROR_GENERIC;
      else
        numCode=0;
    }

    if (numCode) {
      DBG_ERROR(0, "Error %d: %s", numCode,
                GWEN_DB_GetCharValue(db,
                                     "data/text", 0, "(empty)"));
      return -1;
    }
  }
  return 0;
}



int LCD_Driver_Connect(LCD_DRIVER *d, const char *code, const char *text){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  time_t startt;
  GWEN_TYPE_UINT32 rid;

  assert(d);

  if (d->testMode) {
    DBG_INFO(0, "Testmode, will not connect");
    return 0;
  }
  startt=time(0);

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("DriverReady");
  if (d->driverId)
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", d->driverId);

  if (d->remoteMode) {
    /* send some additional information in remote mode */
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverType", d->dtype);
  } /* if remote mode */

  /* send information about every reader we already now.
   * Normally we don't know any reader by now, since the server informs us
   * about that later upon a StartReader request.
   * However, in remote mode (or later for PC/SC drivers) we in fact do have
   * some readers already, so we now inform the server about readers we can
   * offer.
   */
  if (LCD_Reader_List_GetCount(d->readers)) {
    GWEN_DB_NODE *dbReaders;
    LCD_READER *r;

    dbReaders=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                               "readers");
    assert(dbReaders);

    r=LCD_Reader_List_First(d->readers);
    while(r) {
      GWEN_DB_NODE *dbReader;
      GWEN_TYPE_UINT32 flags;

      dbReader=GWEN_DB_GetGroup(dbReaders, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                "reader");
      assert(dbReader);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "driversReaderId",
                          LCD_Reader_GetDriversReaderId(r));
      GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "readerType", d->rtype);
      GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "readerName", d->rname);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "port", d->rport);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "slots", d->rslots);

      flags=LCD_Reader_GetReaderFlags(r);
      LC_ReaderFlags_toDb(dbReader, "readerFlags", flags);

      r=LCD_Reader_List_Next(r);
    } /* while */
  } /* if readers */


  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", code);
  if (text)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", text);

  rid=GWEN_IpcManager_SendRequest(d->ipcManager,
                                  d->ipcId,
                                  dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  /* this sends the message and hopefully receives an answer */
  DBG_INFO(0, "Sending Ready Report");
  dbRsp=0;
  while (1) {
    dbRsp=GWEN_IpcManager_GetResponseData(d->ipcManager, rid);
    if (dbRsp) {
      DBG_DEBUG(0, "Command answered");
      break;
    }
    DBG_VERBOUS(0, "Working...");
    if (LCD_Driver__Work(d, 1000)) {
      DBG_ERROR(0, "Error at work");
      GWEN_IpcManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }

    if (difftime(time(0), startt)>=LCD_DRIVER_STARTTIMEOUT) {
      DBG_ERROR(0, "Timeout");
      GWEN_IpcManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }
  } /* while */

  DBG_DEBUG(0, "Answer received");
  if (LCD_Driver_CheckResponses(dbRsp)) {
    DBG_ERROR(0, "Error returned by server, aborting");
    GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
    return -1;
  }
  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);

  DBG_NOTICE(0, "Connected to server");
  return 0;
}



void LCD_Driver_Disconnect(LCD_DRIVER *d){
  assert(d);
  if (d->testMode) {
    DBG_INFO(0, "Testmode, will not disconnect (since I'm not connected)");
    return;
  }

  if (GWEN_IpcManager_Disconnect(d->ipcManager, d->ipcId)) {
    DBG_ERROR(0, "Error while disconnecting");
  }
}



LCD_READER *LCD_Driver_FindReaderByName(const LCD_DRIVER *d, const char *name) {
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (strcasecmp(name, LCD_Reader_GetName(r))==0)
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderByPort(const LCD_DRIVER *d, int port) {
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (port==LCD_Reader_GetPort(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderById(const LCD_DRIVER *d, GWEN_TYPE_UINT32 id){
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (id==LCD_Reader_GetReaderId(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderByDriversId(const LCD_DRIVER *d,
                                           GWEN_TYPE_UINT32 id){
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (id==LCD_Reader_GetDriversReaderId(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



void LCD_Driver_AddReader(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(r);

  LCD_Reader_List_Add(r, d->readers);
}



void LCD_Driver_DelReader(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(r);

  LCD_Reader_List_Del(r);
}



LCD_READER_LIST *LCD_Driver_GetReaders(const LCD_DRIVER *d){
  assert(d);
  return d->readers;
}



GWEN_TYPE_UINT32 LCD_Driver_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen){
  assert(d);
  assert(d->sendApduFn);
  return d->sendApduFn(d, toReader, r, slot, apdu, apdulen,
                       buffer, bufferlen);
}



GWEN_TYPE_UINT32 LCD_Driver_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->connectSlotFn);
  return d->connectSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LCD_Driver_ConnectReader(LCD_DRIVER *d, LCD_READER *r){
  GWEN_TYPE_UINT32 rv;

  assert(d);
  assert(d->connectReaderFn);
  rv=d->connectReaderFn(d, r);
  if (rv==0)
    LCD_Reader_AddStatus(r, LCD_READER_STATUS_UP);
  return rv;
}



GWEN_TYPE_UINT32 LCD_Driver_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->disconnectSlotFn);
  return d->disconnectSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LCD_Driver_DisconnectReader(LCD_DRIVER *d, LCD_READER *r){
  GWEN_TYPE_UINT32 rv;

  assert(d);
  assert(d->disconnectReaderFn);
  rv=d->disconnectReaderFn(d, r);
  LCD_Reader_SubStatus(r, LCD_READER_STATUS_UP);
  return rv;
}



GWEN_TYPE_UINT32 LCD_Driver_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl){
  assert(d);
  assert(d->resetSlotFn);
  return d->resetSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LCD_Driver_ReaderStatus(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(d->readerStatusFn);
  return d->readerStatusFn(d, r);
}



GWEN_TYPE_UINT32 LCD_Driver_ReaderInfo(LCD_DRIVER *d,
                                      LCD_READER *r,
                                      GWEN_BUFFER *buf){
  assert(d);
  assert(d->readerInfoFn);
  return d->readerInfoFn(d, r, buf);
}



LCD_READER *LCD_Driver_CreateReader(LCD_DRIVER *d,
                                  GWEN_TYPE_UINT32 readerId,
                                  const char *name,
                                  int port,
                                  unsigned int slots,
                                  GWEN_TYPE_UINT32 flags){
  LCD_READER *r;

  assert(d);
  if (d->createReaderFn==0) {
    r=LCD_Reader_new(readerId, name, port, slots, flags);
  }
  else {
    r=d->createReaderFn(d, readerId, name, port, slots, flags);
  }

  return r;
}



const char *LCD_Driver_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err){
  assert(d);
  assert(d->getErrorTextFn);
  return d->getErrorTextFn(d, err);
}



void LCD_Driver_SetSendApduFn(LCD_DRIVER *d, LCD_DRIVER_SENDAPDU_FN fn){
  assert(d);
  d->sendApduFn=fn;
}



void LCD_Driver_SetConnectSlotFn(LCD_DRIVER *d, LCD_DRIVER_CONNECTSLOT_FN fn){
  assert(d);
  d->connectSlotFn=fn;
}



void LCD_Driver_SetDisconnectSlotFn(LCD_DRIVER *d,
                                   LCD_DRIVER_DISCONNECTSLOT_FN fn){
  assert(d);
  d->disconnectSlotFn=fn;
}



void LCD_Driver_SetConnectReaderFn(LCD_DRIVER *d,
                                  LCD_DRIVER_CONNECTREADER_FN fn){
  assert(d);
  d->connectReaderFn=fn;
}



void LCD_Driver_SetDisconnectReaderFn(LCD_DRIVER *d,
                                     LCD_DRIVER_DISCONNECTREADER_FN fn){
  assert(d);
  d->disconnectReaderFn=fn;
}



void LCD_Driver_SetResetSlotFn(LCD_DRIVER *d, LCD_DRIVER_RESETSLOT_FN fn){
  assert(d);
  d->resetSlotFn=fn;
}



void LCD_Driver_SetReaderStatusFn(LCD_DRIVER *d,
                                 LCD_DRIVER_READERSTATUS_FN fn){
  assert(d);
  d->readerStatusFn=fn;
}



void LCD_Driver_SetReaderInfoFn(LCD_DRIVER *d,
                               LCD_DRIVER_READERINFO_FN fn){
  assert(d);
  d->readerInfoFn=fn;
}



void LCD_Driver_SetCreateReaderFn(LCD_DRIVER *d,
                                 LCD_DRIVER_CREATEREADER_FN fn){
  assert(d);
  d->createReaderFn=fn;
}



void LCD_Driver_SetGetErrorTextFn(LCD_DRIVER *d,
                                 LCD_DRIVER_GETERRORTEXT_FN fn){
  assert(d);
  d->getErrorTextFn=fn;
}



void LCD_Driver_SetHandleRequestFn(LCD_DRIVER *d,
                                   LCD_DRIVER_HANDLEREQUEST_FN fn) {
  assert(d);
  d->handleRequestFn=fn;
}



int LCD_Driver_SendStatusChangeNotification(LCD_DRIVER *d,
                                           LCD_SLOT *sl) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  int slot;
  int cardnum;
  GWEN_BUFFER *atr;
  int isInserted;
  LCD_READER *r;
  GWEN_TYPE_UINT32 rid;

  r=LCD_Slot_GetReader(sl);
  slot=LCD_Slot_GetSlotNum(sl);
  cardnum=LCD_Slot_GetCardNum(sl);
  atr=LCD_Slot_GetAtr(sl);
  isInserted=(LCD_Slot_GetStatus(sl) & LCD_SLOT_STATUS_CARD_CONNECTED);

  dbReq=GWEN_DB_Group_new(isInserted?"CardInserted":"CardRemoved");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCD_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCD_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slot);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardnum);

  if (isInserted) {
    if (atr)
      if (GWEN_Buffer_GetUsedBytes(atr))
        GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "atr",
                            GWEN_Buffer_GetStart(atr),
                            GWEN_Buffer_GetUsedBytes(atr));
    if (LCD_Slot_GetFlags(sl) & LCD_SLOT_FLAGS_PROCESSORCARD)
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "PROCESSOR");
    else
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "MEMORY");
  } /* if inserted */

  rid=LCD_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");
  return 0;
}



int LCD_Driver_SendReaderErrorNotification(LCD_DRIVER *d,
                                          LCD_READER *r,
                                          const char *text) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rid;

  assert(d);
  dbReq=GWEN_DB_Group_new("ReaderError");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCD_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCD_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", text);

  rid=LCD_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");

  return 0;
}



int LCD_Driver_RemoveCommand(LCD_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound){
  assert(d);
  return GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, outbound);
}



int LCD_Driver_CheckStatusChanges(LCD_DRIVER *d) {
  LCD_READER *r;

  r=LCD_Reader_List_First(LCD_Driver_GetReaders(d));
  while(r) {
    LCD_READER *rnext;
    GWEN_TYPE_UINT32 retval;

    rnext=LCD_Reader_List_Next(r);

    if (LCD_Reader_GetStatus(r) & LCD_READER_STATUS_UP &&
        !(LCD_Reader_GetReaderFlags(r) & LC_READER_FLAGS_SUSPENDED_CHECKS)) {
      retval=LCD_Driver_ReaderStatus(d, r);
      if (retval) {
        DBG_ERROR(LCD_Reader_GetLogger(r), "Error getting reader status");
        LCD_Driver_SendReaderErrorNotification(d, r,
                                              LCD_Driver_GetErrorText(d, retval));
        DBG_NOTICE(LCD_Reader_GetLogger(r),
                   "Reader \"%s\" had an error, shutting down",
                   LCD_Reader_GetName(r));
        LCD_Reader_List_Del(r);
        LCD_Reader_free(r);
      }
      else {
        LCD_SLOT_LIST *slList;
        LCD_SLOT *sl;
  
        slList=LCD_Reader_GetSlots(r);
        sl=LCD_Slot_List_First(slList);
        while(sl) {
          int isInserted;
          GWEN_TYPE_UINT32 newStatus, oldStatus;
          int cardNum;
  
          newStatus=LCD_Slot_GetStatus(sl);
          if (!(newStatus & LCD_SLOT_STATUS_DISABLED)) {
            oldStatus=LCD_Slot_GetLastStatus(sl);
    
            if (((newStatus^oldStatus) & LCD_SLOT_STATUS_CARD_INSERTED) &&
                /*!(newStatus & LCD_SLOT_STATUS_CARD_CONNECTED) && */
                (newStatus & LCD_SLOT_STATUS_CARD_INSERTED)){
              /* card has just been inserted, try to connect it */
              DBG_NOTICE(LCD_Reader_GetLogger(r),
                         "Card inserted, trying to connect it");
              if (LCD_Driver_ConnectSlot(d, sl)) {
                DBG_ERROR(0, "Card inserted, but I can't connect to it");
              }
              newStatus=LCD_Slot_GetStatus(sl);
            }
    
            isInserted=(newStatus & LCD_SLOT_STATUS_CARD_CONNECTED);
            if ((newStatus^oldStatus) &
                (LCD_SLOT_STATUS_CARD_CONNECTED)){
              DBG_NOTICE(LCD_Reader_GetLogger(r),
                         "Status changed on slot %d (%08x->%08x) (cardnum %d)",
                         LCD_Slot_GetSlotNum(sl),
                         oldStatus, newStatus,
                         LCD_Slot_GetCardNum(sl));
              if (isInserted) {
                DBG_INFO(LCD_Reader_GetLogger(r), "Card is now connected");
                cardNum=++LCD_Driver__LastCardNum;
                LCD_Slot_SetCardNum(sl, cardNum);
              }
              else {
                DBG_INFO(LCD_Reader_GetLogger(r), "Card is not connected");
                cardNum=LCD_Slot_GetCardNum(sl);
              }
  
              DBG_INFO(LCD_Reader_GetLogger(r), "Card number is %d", cardNum);
    
              if (LCD_Driver_SendStatusChangeNotification(d,
                                                         sl)) {
                DBG_ERROR(0, "Error sending status change notification");
              }
              else {
                DBG_INFO(0, "Server informed about new card");
              }
              LCD_Slot_SetLastStatus(sl, newStatus);
            }
            else {
              DBG_DEBUG(LCD_Reader_GetLogger(r), "Status on slot %d unchanged",
                         LCD_Slot_GetSlotNum(sl));
            }
          }
          sl=LCD_Slot_List_Next(sl);
        } /* while slots */
      } /* if getting reader status worked */
    } /* if reader is up */
    r=rnext;
  } /* while reader */

  return 0;
}



int LCD_Driver_HandleStartReader(LCD_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 driversReaderId;
  const char *name;
  int port;
  int slots;
  GWEN_TYPE_UINT32 flags;
  LCD_READER *r;
  char numbuf[16];
  GWEN_TYPE_UINT32 retval;
  GWEN_DB_NODE *dbRsp;

  assert(d);
  assert(dbReq);
  DBG_NOTICE(0, "Command: Start reader");

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, "0"),
                "%x",
                &driversReaderId)) {
    DBG_ERROR(0, "Bad driversReaderId");
    /* TODO: send error result */
    return -1;
  }

  name=GWEN_DB_GetCharValue(dbReq, "data/name", 0, "noname");
  port=GWEN_DB_GetIntValue(dbReq, "data/port", 0, 0);
  flags=GWEN_DB_GetIntValue(dbReq, "data/flags", 0, 0);
  slots=GWEN_DB_GetIntValue(dbReq, "data/slots", 0, 0);
  if (!slots || slots>16) {
    DBG_ERROR(0, "Bad number of slots (%d)", slots);
    /* TODO: send error result */
    return -1;
  }

  /* prepare response */
  dbRsp=GWEN_DB_Group_new("StartReaderResponse");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (r) {
    DBG_WARN(0, "A reader with id \"%08x\" already exists", readerId);

    DBG_NOTICE(LCD_Reader_GetLogger(r), "Restarting reader");
    retval=LCD_Driver_DisconnectReader(d, r);
    if (retval==0)
      retval=LCD_Driver_ConnectReader(d, r);

    if (retval) {
      DBG_ERROR(0, "Could not restart reader");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LCD_Driver_GetErrorText(d, retval));
    }
    else {
      if (LCD_Reader_GetReaderFlags(r) & LC_READER_FLAGS_NOINFO) {
        DBG_WARN(0, "ReaderInfo disabled");
      }
      else {
        GWEN_BUFFER *ibuf;
        GWEN_TYPE_UINT32 rv;

        ibuf=GWEN_Buffer_new(0, 256, 0, 1);
        rv=LCD_Driver_ReaderInfo(d, r, ibuf);
        if (rv) {
          DBG_WARN(0, "ReaderInfo not available (%s)",
                   LCD_Driver_GetErrorText(d, rv));
        }
        else {
          GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "info",
                               GWEN_Buffer_GetStart(ibuf));
        }
        GWEN_Buffer_free(ibuf);
      }

      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Reader up and waiting");
    }

    if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LCD_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
  } /* if reader found */
  else {
    /* search by driversReaderId */
    if (driversReaderId) {
      r=LCD_Driver_FindReaderByDriversId(d, driversReaderId);
      if (r) {
        if (LCD_Reader_GetReaderId(r)==0) {
          /* The reader exists but has no reader id. So this is the first time
           * the reader has been accessed. Assign the reader id from the
           * server so that the next calls will find it. */
          LCD_Reader_SetReaderId(r, readerId);
        }
        else {
          DBG_ERROR(0, "Uups, reader already has an id ?");
          /* send error result */
          GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "code", "ERROR");
          GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text",
                               "Internal error (reader already has an id)");

          LCD_Driver_SendResponse(d, rid, dbRsp);
          LCD_Driver_RemoveCommand(d, rid, 0);
          return -1;
        }
      }
      else {
        DBG_ERROR(0, "Reader not found");
      }
    }
    else {
      DBG_ERROR(0, "No DriversReaderId");
    }

    if (!r) {
      /* check whether we have a reader at that port */
      r=LCD_Driver_FindReaderByPort(d, port);
      if (r) {
        DBG_ERROR(0, "A reader with port \"%08x\" already exists", port);
        /* send error result */
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "ERROR");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text",
                             "There already is a reader at the given port");
  
        LCD_Driver_SendResponse(d, rid, dbRsp);
        LCD_Driver_RemoveCommand(d, rid, 0);
        return -1;
      }
      /* if not found it is ok to create the reader */
      r=LCD_Driver_CreateReader(d, readerId, name, port, slots, flags);
      assert(r);
      LCD_Driver_AddReader(d, r);
    }

    if (d->readerLogFile) {
      GWEN_BUFFER *mbuf;

      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LCD_Driver_ReplaceVar(d->readerLogFile, "reader", name, mbuf);
      if (GWEN_Directory_GetPath(GWEN_Buffer_GetStart(mbuf),
                                 GWEN_PATH_FLAGS_VARIABLE)) {
        DBG_ERROR(0, "Could not create log file for reader \"%s\"", name);
      }
      else {
        if (GWEN_Logger_Open(name,
                             name,
                             GWEN_Buffer_GetStart(mbuf),
                             GWEN_LoggerTypeFile,
                             GWEN_LoggerFacilityDaemon)) {
          DBG_ERROR(0, "Could not open logger for reader \"%s\"", name);
        }
        else {
          DBG_NOTICE(0, "Reader \"%s\" logs to \"%s\"", name,
                     GWEN_Buffer_GetStart(mbuf));
          LCD_Reader_SetLogger(r, name);
        }
        GWEN_Buffer_free(mbuf);
      }
    } /* if reader log file */
    else {
      if (GWEN_Logger_Open(name,
                           name,
                           0,
                           GWEN_LoggerTypeConsole,
                           GWEN_LoggerFacilityDaemon)) {
        DBG_ERROR(0, "Could not open logger for reader \"%s\"", name);
      }
    }
    GWEN_Logger_SetLevel(name, d->logLevel);

    /* init reader */
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Init reader %s", LCD_Reader_GetName(r));
    retval=LCD_Driver_ConnectReader(d, r);
    if (retval) {
      DBG_ERROR(LCD_Reader_GetLogger(r),
                "Could not connect reader %s (%d: %s)",
                LCD_Reader_GetName(r),
                retval,
                LCD_Driver_GetErrorText(d, retval));
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LCD_Driver_GetErrorText(d, retval));
    }
    else {
      GWEN_BUFFER *ibuf;
      GWEN_TYPE_UINT32 rv;

      ibuf=GWEN_Buffer_new(0, 256, 0, 1);
      rv=LCD_Driver_ReaderInfo(d, r, ibuf);
      if (rv) {
        DBG_WARN(0, "ReaderInfo not available (%s)",
                 LCD_Driver_GetErrorText(d, rv));
      }
      else {
        DBG_NOTICE(LCD_Reader_GetLogger(r), "ReaderInfo: %s",
                   GWEN_Buffer_GetStart(ibuf));
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "info",
                             GWEN_Buffer_GetStart(ibuf));
      }
      GWEN_Buffer_free(ibuf);

      DBG_NOTICE(LCD_Reader_GetLogger(r), "Reader up and waiting");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Reader up and waiting");
    }

    if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LCD_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
    DBG_NOTICE(LCD_Reader_GetLogger(r), "Reader start handled");
  }
  LCD_Driver_RemoveCommand(d, rid, 0);

  return 0;
}


int LCD_Driver_ReplaceVar(const char *path,
                         const char *var,
                         const char *value,
                         GWEN_BUFFER *nbuf) {
  unsigned int vlen;

  vlen=strlen(var);

  while(*path) {
    int handled;

    handled=0;
    if (*path=='@') {
      if (strncmp(path+1, var, vlen)==0) {
        if (path[vlen+1]=='@') {
          /* found variable, replace it */
          GWEN_Buffer_AppendString(nbuf, value);
          path+=vlen+2;
          handled=1;
        }
      }
    }
    if (!handled) {
      GWEN_Buffer_AppendByte(nbuf, *path);
      path++;
    }
  } /* while */

  return 0;
}


int LCD_Driver_HandleStopReader(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LCD_READER *r;
  char numbuf[16];
  GWEN_TYPE_UINT32 retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* TODO: send error result */
    return -1;
  }

  /* deinit reader */
  DBG_NOTICE(LCD_Reader_GetLogger(r), "Disconnecting reader");
  dbRsp=GWEN_DB_Group_new("StopReaderResponse");
  retval=LCD_Driver_DisconnectReader(d, r);
  if (retval!=0) {
    DBG_INFO(LCD_Reader_GetLogger(r), "Could not disconnect reader");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         LCD_Driver_GetErrorText(d, retval));
  }
  else {
    /* init ok */
    DBG_NOTICE(LCD_Reader_GetLogger(r), "Deinit succeeded");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Reader down as requested");
  }

  /* create response */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LCD_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Reader down");
  LCD_Driver_DelReader(d, r);
  LCD_Reader_free(r);
  return 0;
}



int LCD_Driver_HandleResetCard(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  LCD_READER *r;
  int slotNum;
  int cardNum;
  LCD_SLOT *slot;
  char retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LCD_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LCD_Slot_GetSlotNum(slot));
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  /* check card number and reader status */
  if ((LCD_Slot_GetCardNum(slot)!=cardNum) ||
      !(LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(0, "Card \"%d\" has been removed", cardNum);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  DBG_NOTICE(LCD_Reader_GetLogger(r), "Resetting card");
  retval=LCD_Driver_ResetSlot(d, slot);
  if (retval!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Error resetting slot (%d: %s)",
              retval,
              LCD_Driver_GetErrorText(d, retval));
    LCD_Driver_SendReaderErrorNotification(d, r,
                                          LCD_Driver_GetErrorText(d, retval));
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LCD_Reader_GetName(r));
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
  }
  else {
    /* reset ok */
    DBG_INFO(LCD_Reader_GetLogger(r), "Reset succeeded");
  }

  LCD_Driver_RemoveCommand(d, rid, 0);

  return 0;
}



int LCD_Driver_HandleCardCommand(LCD_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LCD_READER *r;
  char numbuf[16];
  unsigned char rspbuffer[300];
  const unsigned char *apdu;
  unsigned int apdulen;
  int rsplen;
  int slotNum;
  int cardNum;
  LCD_SLOT *slot;
  char retval;
  const char *target;
  int toReader;
  int readerError;

  assert(d);
  assert(dbReq);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  apdu=GWEN_DB_GetBinValue(dbReq, "data/data", 0, 0, 0, &apdulen);
  if (!apdu || apdulen<4) {
    DBG_ERROR(0, "APDU too small");
    /* send error result */
    LCD_Driver_SendResult(d,
                         rid,
                         "CardCommandResponse",
                         "ERROR", "APDU too small");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad slot number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad card number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "data/target", 0, 0);
  if (!target) {
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "No target");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  if (strcasecmp(target, "reader")==0)
    toReader=1;
  else if (strcasecmp(target, "card")==0)
    toReader=0;
  else {
    DBG_ERROR(0, "Bad target \"%s\"", target);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad target");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Reader not found");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LCD_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Slot not found");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LCD_Slot_GetSlotNum(slot));
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Slot diabled");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  /* check card number and reader status */
  if ((LCD_Slot_GetCardNum(slot)!=cardNum) ||
      !(LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Card \"%d\" has been removed", cardNum);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Card has been removed");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  DBG_DEBUG(LCD_Reader_GetLogger(r), "Executing command");
  GWEN_Text_LogString((const char*)apdu, apdulen, 0, GWEN_LoggerLevelDebug);
  dbRsp=GWEN_DB_Group_new("CardCommandResponse");
  rsplen=sizeof(rspbuffer)-1;
  retval=LCD_Driver_SendAPDU(d, toReader, r, slot, apdu, apdulen,
                              rspbuffer, &rsplen);
  if (retval!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Error executing APDU (%08x)", retval);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", LCD_Driver_GetErrorText(d, retval));
    readerError=retval;
  }
  else {
    if (rsplen<2) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Too short answer");
      readerError=-1;
    }
    else {
      /* init ok */
      DBG_DEBUG(LCD_Reader_GetLogger(r), "Command succeeded");
      GWEN_Text_LogString((const char*)rspbuffer, rsplen, 0, GWEN_LoggerLevelDebug);

      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Command executed");
      assert(rsplen);
      GWEN_DB_SetBinValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "data", rspbuffer, rsplen);
      readerError=0;
    }
  }

  /* create response */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slotNum);
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardNum);
  if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LCD_Driver_RemoveCommand(d, rid, 0);
  DBG_DEBUG(0, "Response send");

  if (readerError) {
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LCD_Reader_GetName(r));
    LCD_Driver_SendReaderErrorNotification(d, r,
                                           LCD_Driver_GetErrorText(d,
                                                                   readerError));
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
  }

  return 0;
}



int LCD_Driver_HandleStopDriver(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  LCD_READER_LIST *rl;
  LCD_READER *r;

  assert(d);
  assert(dbReq);

  rl=LCD_Driver_GetReaders(d);
  assert(rl);
  r=LCD_Reader_List_First(rl);
  while(r) {
    LCD_READER *nr;

    nr=LCD_Reader_List_Next(r);
    /* deinit reader */
    DBG_INFO(LCD_Reader_GetLogger(r),
             "Disconnecting reader \"%s\"", LCD_Reader_GetName(r));
    if (LCD_Driver_DisconnectReader(d, r)) {
      DBG_WARN(LCD_Reader_GetLogger(r), "Could not disconnect reader");
    }
    else {
      DBG_INFO(LCD_Reader_GetLogger(r), "Reader \"%s\" disconnected", LCD_Reader_GetName(r));
    }
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
    r=nr;
  } /* while r */

  LCD_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Driver down");
  d->stopDriver=1;
  return 0;
}



int LCD_Driver_HandleSuspendCheck(LCD_DRIVER *d,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  LCD_READER *r;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exist", readerId);
    /* TODO: send error result */
    return -1;
  }

  DBG_NOTICE(LCD_Reader_GetLogger(r), "Suspending checks");
  LCD_Reader_AddReaderFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
  LCD_Driver_RemoveCommand(d, rid, 0);
  return 0;
}



int LCD_Driver_HandleResumeCheck(LCD_DRIVER *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  LCD_READER *r;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exist", readerId);
    /* TODO: send error result */
    return -1;
  }

  DBG_NOTICE(LCD_Reader_GetLogger(r), "Resuming checks");
  LCD_Reader_SubReaderFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
  LCD_Driver_RemoveCommand(d, rid, 0);
  return 0;
}



int LCD_Driver_HandleRequest(LCD_DRIVER *d,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq){
  int rv;

  DBG_NOTICE(0, "Incoming request \"%s\"", name);

  /* if there is a virtual function set go ask the function first */
  if (d->handleRequestFn) {
    rv=d->handleRequestFn(d, rid, name, dbReq);
    if (rv!=1)
      return rv;
  }

  if (strcasecmp(name, "StartReader")==0) {
    rv=LCD_Driver_HandleStartReader(d, rid, dbReq);
  }
  else if (strcasecmp(name, "StopReader")==0) {
    rv=LCD_Driver_HandleStopReader(d, rid, dbReq);
  }
  else if (strcasecmp(name, "CardCommand")==0) {
    rv=LCD_Driver_HandleCardCommand(d, rid, dbReq);
   }
  else if (strcasecmp(name, "ResetCard")==0) {
    rv=LCD_Driver_HandleResetCard(d, rid, dbReq);
  }
  else if (strcasecmp(name, "StopDriver")==0) {
    rv=LCD_Driver_HandleStopDriver(d, rid, dbReq);
  }
  else if (strcasecmp(name, "SuspendCheck")==0) {
    rv=LCD_Driver_HandleSuspendCheck(d, rid, dbReq);
  }
  else if (strcasecmp(name, "ResumeCheck")==0) {
    rv=LCD_Driver_HandleResumeCheck(d, rid, dbReq);
  }

  else
    rv=1; /* not handled */

  return rv;
}




int LCD_Driver_Work(LCD_DRIVER *d) {
  time_t lastStatusCheckTime;
  time_t t1;

  lastStatusCheckTime=(time_t)0;
  while(!d->stopDriver) {
    GWEN_NETLAYER_RESULT res;
    GWEN_TYPE_UINT32 rid;
    int needHeartbeat;

    t1=time(0);
    if (difftime(t1, lastStatusCheckTime)>=1) {
      /* Do some hardware work */
      DBG_VERBOUS(0, "Checking for status changes");
      LCD_Driver_CheckStatusChanges(d);
      lastStatusCheckTime=t1;
    }

    needHeartbeat=0;
    while(!needHeartbeat) {
      int j;

      t1=time(0);
      if (difftime(t1, lastStatusCheckTime)>=1) {
        /* Do some hardware work */
        DBG_VERBOUS(0, "Checking for status changes");
        LCD_Driver_CheckStatusChanges(d);
        lastStatusCheckTime=t1;
      }
      for(j=0; ; j++) {
        int rv;

        if (j>LCD_DRIVER_IPC_MAXWORK) {
          DBG_ERROR(0, "IPC running wild, aborting driver");
          return -1;
        }
        t1=time(0);
        if (difftime(t1, lastStatusCheckTime)>=1) {
          /* Do some hardware work */
          DBG_VERBOUS(0, "Checking for status changes");
          LCD_Driver_CheckStatusChanges(d);
          lastStatusCheckTime=t1;
        }
        /* work as long as possible */
        rv=GWEN_IpcManager_Work(d->ipcManager);
        if (rv==-1) {
          DBG_ERROR(0, "Error while working with IPC");
          return -1;
        }
        else if (rv==1)
          break;
      }

      rid=LCD_Driver_GetNextInRequest(d);
      if (rid) {
        GWEN_DB_NODE *dbReq;
        int rv;
        const char *name;

        dbReq=LCD_Driver_GetInRequestData(d, rid);
        assert(dbReq);

        /* we have an incoming message */
        name=GWEN_DB_GetCharValue(dbReq, "ipc/cmd", 0, 0);
        if (!name) {
          DBG_ERROR(0, "Bad IPC command (no command name), discarding");
          LCD_Driver_RemoveCommand(d, rid, 0);
        }
        rv=LCD_Driver_HandleRequest(d, rid, name, dbReq);
        if (rv==1) {
          DBG_WARN(0, "Unknown command \"%s\", discarding", name);
          if (GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 0)) {
            DBG_ERROR(0, "Could not remove request");
            abort();
          }
        }
        else if (rv==-1) {
          DBG_ERROR(0, "Error while handling request, going down");
          return -1;
        }
        else {
          for(j=0; ; j++) {
            int rv;

            if (j>LCD_DRIVER_IPC_MAXWORK) {
              DBG_ERROR(0, "IPC running wild, aborting driver");
              return -1;
            }

            /* work as long as possible (flush responses) */
            rv=GWEN_IpcManager_Work(d->ipcManager);
            if (rv==-1) {
              DBG_ERROR(0, "Error while working with IPC");
              return -1;
            }
            else if (rv==1)
              break;
          }
        } /* if something done */
      } /* if incoming request */
      else
        needHeartbeat=1;
    } /* while !needHeartbeat */

    res=GWEN_Net_HeartBeat(750);
    if (res==GWEN_NetLayerResult_Error) {
      DBG_ERROR(0, "Network error");
      return -1;
    }
    else if (res==GWEN_NetLayerResult_Idle) {
      DBG_VERBOUS(0, "No activity");
    }
  } /* while driver is not to be stopped */
  return 0;
}



GWEN_NL_SSL_ASKADDCERT_RESULT
LCD_Driver_AskAddCert(GWEN_NETLAYER *nl,
                      const GWEN_SSLCERTDESCR *cert,
                      void *user_data) {
  LCD_DRIVER *d;

  d=(LCD_DRIVER*)user_data;
  if (!d) {
    DBG_ERROR(0, "No user data in AskAddCert function");
    return GWEN_NetLayerSsl_AskAddCertResult_No;
  }

  if (d->acceptAllCerts)
    return GWEN_NetLayerSsl_AskAddCertResult_Tmp;
  return GWEN_NetLayerSsl_AskAddCertResult_No;
}












