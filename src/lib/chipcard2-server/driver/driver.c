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
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <time.h>

static GWEN_TYPE_UINT32 LC_Driver__LastCardNum=0;


GWEN_INHERIT_FUNCTIONS(LC_DRIVER)


void LC_Driver_Usage(const char *prgName) {
  fprintf(stdout,
          "%s [OPTONS] \n"
          "[-v]              verbous\n"
          "[--logfile ARG]   name of the logfile\n"
          "[--logtype ARG]   log type\n"
          "[--loglevel ARG]  log level\n"
          "[-d ARG]          driver data folder\n"
          "-b ARG            server id\n"
          "[-u ARG]          customer id of this driver\n"
          "[-a ARG]          server IP address (or hostname)\n"
          "[-p ARG]          server TCP port\n"
          "-l ARG            name of the library driver file\n"
	  "-i ARG            driver id for this session\n"
	  "The following arguments are used in test mode only\n"
          "--test            enter test mode, check for a given reader\n"
          "-rp ARG           reader port\n"
          "-rs ARG           reader slots\n"
          "-rn ARG           reader name\n"
          , prgName
         );
}



LC_DRIVER_CHECKARGS_RESULT LC_Driver_CheckArgs(LC_DRIVER *d,
                                               int argc, char **argv) {
  int i;

  assert(d);

  d->verbous=0;
  d->rslots=1;
  d->testMode=0;
  d->rport=0;
  d->rname=0;
  d->logType=GWEN_LoggerTypeConsole;
  d->logFile=strdup("driver.log");
  d->logLevel=GWEN_LoggerLevelNotice; // debug
  d->serverPort=LC_DEFAULT_PORT;
  d->readers=LC_Reader_List_new();
  d->typ="local";
  d->certFile=0;
  d->certDir=0;
  d->serverAddr=0;

  i=1;
  while (i<argc){
    if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      free(d->logFile);
      d->logFile=strdup(argv[i]);
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (d->logType==GWEN_LoggerTypeUnknown) {
        DBG_ERROR(0, "Unknown log type \"%s\"\n", argv[i]);
        return LC_DriverCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (d->logLevel==GWEN_LoggerLevelUnknown) {
        DBG_ERROR(0, "Unknown log level \"%s\"\n", argv[i]);
        return LC_DriverCheckArgsResultError;
      }
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
        return LC_DriverCheckArgsResultError;
      d->typ=argv[i];
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->driverId=argv[i];
    }
    else if (strcmp(argv[i],"-a")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->serverAddr=argv[i];
    }
    else if (strcmp(argv[i],"-p")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->serverPort=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-l")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->libraryFile=argv[i];
    }
    else if (strcmp(argv[i],"-c")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->certFile=argv[i];
    }
    else if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
        return LC_DriverCheckArgsResultError;
      d->certDir=argv[i];
    }
    else if (strcmp(argv[i],"--test")==0) {
      d->testMode=1;
    }
    else if (strcmp(argv[i],"-rn")==0) {
      i++;
      if (i>=argc)
	return LC_DriverCheckArgsResultError;
      d->rname=argv[i];
    }
    else if (strcmp(argv[i],"-rp")==0) {
      i++;
      if (i>=argc)
	return LC_DriverCheckArgsResultError;
      d->rport=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-rs")==0) {
      i++;
      if (i>=argc)
	return LC_DriverCheckArgsResultError;
      d->rslots=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      LC_Driver_Usage(argv[0]);
      return LC_DriverCheckArgsResultHelp;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      return LC_DriverCheckArgsResultVersion;
    }
    else if (strcmp(argv[i],"-v")==0) {
      d->verbous=1;
    }
    else {
      DBG_ERROR(0, "Unknown argument \"%s\"", argv[i]);
      return LC_DriverCheckArgsResultError;
    }
    i++;
  } /* while */

  /* check for missing arguments */
  if (!d->testMode) {
    if (!d->serverAddr) {
      DBG_ERROR(0, "Server address missing");
      return LC_DriverCheckArgsResultError;
    }
    if (!d->driverId) {
      DBG_ERROR(0, "Driver id missing");
      return LC_DriverCheckArgsResultError;
    }
  }

  if (!d->libraryFile) {
    DBG_ERROR(0, "Name of driver library file missing");
    return LC_DriverCheckArgsResultError;
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
      LC_Driver_ReplaceVar(d->logFile, "reader", "driver", mbuf);
      free(d->logFile);
      d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
      GWEN_Buffer_free(mbuf);
    }
  }

  return 0;
}



LC_DRIVER *LC_Driver_new(int argc, char **argv) {
  LC_DRIVER *d;
  LC_DRIVER_CHECKARGS_RESULT res;
  GWEN_NETTRANSPORT *tr;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  GWEN_TYPE_UINT32 sid;

  GWEN_NEW_OBJECT(LC_DRIVER, d);
  GWEN_INHERIT_INIT(LC_DRIVER, d);

  res=LC_Driver_CheckArgs(d, argc, argv);
  if (res!=LC_DriverCheckArgsResultOk) {
    GWEN_FREE_OBJECT(d);
    return 0;
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

    d->ipcManager=GWEN_IPCManager_new();

    if (strcasecmp(d->typ, "local")==0) {
      /* HTTP over UDS */
      sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      tr=GWEN_NetTransportSocket_new(sk, 1);
    }
    else if (strcasecmp(d->typ, "public")==0) {
      /* HTTP over TCP */
      sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      tr=GWEN_NetTransportSocket_new(sk, 1);
    }
    else {
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      if (strcasecmp(d->typ, "private")==0) {
	/* HTTP over SSL */
	sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	tr=GWEN_NetTransportSSL_new(sk,
				    d->certDir,
				    0,
				    d->certFile,
				    0,
				    0,
				    1);
      }
      else if (strcasecmp(d->typ, "secure")==0) {
	/* HTTP over SSL with certificates */
	sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	tr=GWEN_NetTransportSSL_new(sk,
				    d->certDir,
				    0,
				    d->certFile,
				    0,
				    1,
				    1);
      }
      else {
	DBG_ERROR(0, "Unknown mode \"%s\"", d->typ);
	GWEN_InetAddr_free(addr);
	LC_Driver_free(d);
	return 0;
      }
    }

    GWEN_NetTransport_SetPeerAddr(tr, addr);
    GWEN_InetAddr_free(addr);
    sid=GWEN_IPCManager_AddClient(d->ipcManager,
				  tr,
				  0,0,
				  LC_DRIVER_MARK_DRIVER);
    if (sid==0) {
      DBG_ERROR(0, "Could not add IPC client");
      LC_Driver_free(d);
      return 0;
    }

    d->ipcId=sid;
    DBG_INFO(0, "IPC stuff initialized");
  }

  return d;
}



void LC_Driver_free(LC_DRIVER *d) {
  if (d) {
    GWEN_INHERIT_FINI(LC_DRIVER, d);
    LC_Reader_List_free(d->readers);
    GWEN_IPCManager_free(d->ipcManager);
    free(d->logFile);
    free(d->readerLogFile);
    GWEN_FREE_OBJECT(d);
  }
}



int LC_Driver_IsTestMode(const LC_DRIVER *d) {
  assert(d);
  return d->testMode;
}



int LC_Driver_Test(LC_DRIVER *d) {
  LC_READER *r;
  GWEN_TYPE_UINT32 res;

  assert(d);
  if (!d->testMode) {
    DBG_ERROR(0, "Not in test mode");
    return -1;
  }

  r=LC_Driver_CreateReader(d,
                           1,
                           d->rname,
                           d->rport,
                           d->rslots,
                           0);
  assert(r);
  fprintf(stdout, "Connecting reader...\n");
  res=LC_Driver_ConnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not connect reader (%s)\n",
	    LC_Driver_GetErrorText(d, res));
    LC_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader connected.\n");

  fprintf(stdout, "Disconnecting reader...\n");
  res=LC_Driver_DisconnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not disconnect reader (%s)\n",
	    LC_Driver_GetErrorText(d, res));
    LC_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader disconnected.\n");

  fprintf(stdout, "Reader is available.\n");
  LC_Reader_free(r);
  return 0;
}



const char *LC_Driver_GetDriverDataDir(const LC_DRIVER *d){
  assert(d);
  return d->driverDataDir;
}



const char *LC_Driver_GetLibraryFile(const LC_DRIVER *d){
  assert(d);
  return d->libraryFile;
}



const char *LC_Driver_GetDriverId(const LC_DRIVER *d){
  assert(d);
  return d->driverId;
}



GWEN_TYPE_UINT32 LC_Driver_SendCommand(LC_DRIVER *d,
                                       GWEN_DB_NODE *dbCommand) {
  return GWEN_IPCManager_SendRequest(d->ipcManager,
                                     d->ipcId, dbCommand);
}



int LC_Driver_SendResponse(LC_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand) {
  return GWEN_IPCManager_SendResponse(d->ipcManager,
                                      rid, dbCommand);
}



int LC_Driver_SendResult(LC_DRIVER *d,
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
  return LC_Driver_SendResponse(d, rid, db);
}



GWEN_TYPE_UINT32 LC_Driver_GetNextInRequest(LC_DRIVER *d) {
  assert(d);
  return GWEN_IPCManager_GetNextInRequest(d->ipcManager,
                                          LC_DRIVER_MARK_DRIVER);
}



GWEN_DB_NODE *LC_Driver_GetInRequestData(LC_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid) {
  assert(d);
  return GWEN_IPCManager_GetInRequestData(d->ipcManager, rid);
}



int LC_Driver__Work(LC_DRIVER *d, int timeout, int maxmsg){
  GWEN_NETCONNECTION_WORKRESULT res;
  int rv;

  if (!GWEN_Net_HasActiveConnections()) {
    DBG_ERROR(0, "No active connections, stopping");
    return -1;
  }

  res=GWEN_Net_HeartBeat(timeout);
  if (res==GWEN_NetConnectionWorkResult_Error) {
    DBG_ERROR(0, "Network error");
    return -1;
  }
  else if (res==GWEN_NetConnectionWorkResult_NoChange) {
    DBG_VERBOUS(0, "No activity");
  }

  while(1) {
    DBG_DEBUG(0, "Driver: Working");
    /* activity detected, work with it */
    rv=GWEN_IPCManager_Work(d->ipcManager, maxmsg);
    if (rv==-1) {
      DBG_ERROR(0, "Error while working with IPC");
      return -1;
    }
    else if (rv==1)
      break;
  }

  return 0;
}



int LC_Driver_CheckResponses(GWEN_DB_NODE *db) {
  const char *name;

  name=GWEN_DB_GetCharValue(db, "command/vars/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC message (no command)");
    return -1;
  }
  if (strcasecmp(name, "Error")==0) {
    const char *code;

    code=GWEN_DB_GetCharValue(db, "body/code", 0, 0);
    if (strcasecmp(code, "OK")!=0) {
      DBG_ERROR(0, "Error %s: %s", code,
                GWEN_DB_GetCharValue(db,
                                     "body/text", 0, "(empty)"));
      return -1;
    }
  }
  return 0;
}



int LC_Driver_Connect(LC_DRIVER *d, const char *code, const char *text){
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
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", d->driverId);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", code);
  if (text)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", text);

  rid=GWEN_IPCManager_SendRequest(d->ipcManager,
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
    dbRsp=GWEN_IPCManager_GetResponseData(d->ipcManager, rid);
    if (dbRsp) {
      DBG_DEBUG(0, "Command answered");
      break;
    }
    DBG_VERBOUS(0, "Working...");
    if (LC_Driver__Work(d, 1000, 0)) {
      DBG_ERROR(0, "Error at work");
      GWEN_IPCManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }

    if (difftime(time(0), startt)>=LC_DRIVER_STARTTIMEOUT) {
      DBG_ERROR(0, "Timeout");
      GWEN_IPCManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }
  } /* while */

  DBG_DEBUG(0, "Answer received");
  if (LC_Driver_CheckResponses(dbRsp)) {
    DBG_ERROR(0, "Error returned by server, aborting");
    GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);

  DBG_NOTICE(0, "Connected to server");
  return 0;
}



void LC_Driver_Disconnect(LC_DRIVER *d){
  assert(d);
  if (d->testMode) {
    DBG_INFO(0, "Testmode, will not disconnect (since I'm not connected)");
    return;
  }

  if (GWEN_IPCManager_Disconnect(d->ipcManager, d->ipcId)) {
    DBG_ERROR(0, "Error while disconnecting");
  }
}



LC_READER *LC_Driver_FindReaderByName(const LC_DRIVER *d, const char *name) {
  LC_READER *r;

  assert(d);
  r=LC_Reader_List_First(d->readers);
  while(r) {
    if (strcasecmp(name, LC_Reader_GetName(r))==0)
      return r;
    r=LC_Reader_List_Next(r);
  } /* while */
  return 0;
}



LC_READER *LC_Driver_FindReaderByPort(const LC_DRIVER *d, int port) {
  LC_READER *r;

  assert(d);
  r=LC_Reader_List_First(d->readers);
  while(r) {
    if (port==LC_Reader_GetPort(r))
      return r;
    r=LC_Reader_List_Next(r);
  } /* while */
  return 0;
}



LC_READER *LC_Driver_FindReaderById(const LC_DRIVER *d, GWEN_TYPE_UINT32 id){
  LC_READER *r;

  assert(d);
  r=LC_Reader_List_First(d->readers);
  while(r) {
    if (id==LC_Reader_GetReaderId(r))
      return r;
    r=LC_Reader_List_Next(r);
  } /* while */
  return 0;
}



void LC_Driver_AddReader(LC_DRIVER *d, LC_READER *r){
  assert(d);
  assert(r);

  LC_Reader_List_Add(r, d->readers);
}



void LC_Driver_DelReader(LC_DRIVER *d, LC_READER *r){
  assert(d);
  assert(r);

  LC_Reader_List_Del(r);
}



LC_READER_LIST *LC_Driver_GetReaders(const LC_DRIVER *d){
  assert(d);
  return d->readers;
}



GWEN_TYPE_UINT32 LC_Driver_SendAPDU(LC_DRIVER *d,
                                    int toReader,
                                    LC_READER *r,
                                    LC_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen){
  assert(d);
  assert(d->sendApduFn);
  return d->sendApduFn(d, toReader, r, slot, apdu, apdulen,
                       buffer, bufferlen);
}



GWEN_TYPE_UINT32 LC_Driver_ConnectSlot(LC_DRIVER *d, LC_SLOT *sl){
  assert(d);
  assert(d->connectSlotFn);
  return d->connectSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LC_Driver_ConnectReader(LC_DRIVER *d, LC_READER *r){
  assert(d);
  assert(d->connectReaderFn);
  return d->connectReaderFn(d, r);
}



GWEN_TYPE_UINT32 LC_Driver_DisconnectSlot(LC_DRIVER *d, LC_SLOT *sl){
  assert(d);
  assert(d->disconnectSlotFn);
  return d->disconnectSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LC_Driver_DisconnectReader(LC_DRIVER *d, LC_READER *r){
  assert(d);
  assert(d->disconnectReaderFn);
  return d->disconnectReaderFn(d, r);
}



GWEN_TYPE_UINT32 LC_Driver_ResetSlot(LC_DRIVER *d, LC_SLOT *sl){
  assert(d);
  assert(d->resetSlotFn);
  return d->resetSlotFn(d, sl);
}



GWEN_TYPE_UINT32 LC_Driver_ReaderStatus(LC_DRIVER *d, LC_READER *r){
  assert(d);
  assert(d->readerStatusFn);
  return d->readerStatusFn(d, r);
}



GWEN_TYPE_UINT32 LC_Driver_ReaderInfo(LC_DRIVER *d,
                                      LC_READER *r,
                                      GWEN_BUFFER *buf){
  assert(d);
  assert(d->readerInfoFn);
  return d->readerInfoFn(d, r, buf);
}



LC_READER *LC_Driver_CreateReader(LC_DRIVER *d,
                                  GWEN_TYPE_UINT32 readerId,
                                  const char *name,
                                  int port,
                                  unsigned int slots,
                                  GWEN_TYPE_UINT32 flags){
  LC_READER *r;

  assert(d);
  if (!d->createReaderFn) {
    r=LC_Reader_new(readerId, name, port, slots, flags);
  }
  else {
    r=d->createReaderFn(d, readerId, name, port, slots, flags);
  }

  return r;
}



const char *LC_Driver_GetErrorText(LC_DRIVER *d, GWEN_TYPE_UINT32 err){
  assert(d);
  assert(d->getErrorTextFn);
  return d->getErrorTextFn(d, err);
}



void LC_Driver_SetSendApduFn(LC_DRIVER *d, LC_DRIVER_SENDAPDU_FN fn){
  assert(d);
  d->sendApduFn=fn;
}



void LC_Driver_SetConnectSlotFn(LC_DRIVER *d, LC_DRIVER_CONNECTSLOT_FN fn){
  assert(d);
  d->connectSlotFn=fn;
}



void LC_Driver_SetDisconnectSlotFn(LC_DRIVER *d,
                                   LC_DRIVER_DISCONNECTSLOT_FN fn){
  assert(d);
  d->disconnectSlotFn=fn;
}



void LC_Driver_SetConnectReaderFn(LC_DRIVER *d,
                                  LC_DRIVER_CONNECTREADER_FN fn){
  assert(d);
  d->connectReaderFn=fn;
}



void LC_Driver_SetDisconnectReaderFn(LC_DRIVER *d,
                                     LC_DRIVER_DISCONNECTREADER_FN fn){
  assert(d);
  d->disconnectReaderFn=fn;
}



void LC_Driver_SetResetSlotFn(LC_DRIVER *d, LC_DRIVER_RESETSLOT_FN fn){
  assert(d);
  d->resetSlotFn=fn;
}



void LC_Driver_SetReaderStatusFn(LC_DRIVER *d,
                                 LC_DRIVER_READERSTATUS_FN fn){
  assert(d);
  d->readerStatusFn=fn;
}



void LC_Driver_SetReaderInfoFn(LC_DRIVER *d,
                               LC_DRIVER_READERINFO_FN fn){
  assert(d);
  d->readerInfoFn=fn;
}



void LC_Driver_SetCreateReaderFn(LC_DRIVER *d,
                                 LC_DRIVER_CREATEREADER_FN fn){
  assert(d);
  d->createReaderFn=fn;
}



void LC_Driver_SetGetErrorTextFn(LC_DRIVER *d,
                                 LC_DRIVER_GETERRORTEXT_FN fn){
  assert(d);
  d->getErrorTextFn=fn;
}



int LC_Driver_SendStatusChangeNotification(LC_DRIVER *d,
                                           LC_SLOT *sl) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  int slot;
  int cardnum;
  GWEN_BUFFER *atr;
  int isInserted;
  LC_READER *r;
  GWEN_TYPE_UINT32 rid;

  r=LC_Slot_GetReader(sl);
  slot=LC_Slot_GetSlotNum(sl);
  cardnum=LC_Slot_GetCardNum(sl);
  atr=LC_Slot_GetAtr(sl);
  isInserted=(LC_Slot_GetStatus(sl) & LC_SLOT_STATUS_CARD_CONNECTED);

  dbReq=GWEN_DB_Group_new(isInserted?"CardInserted":"CardRemoved");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

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
    if (LC_Slot_GetFlags(sl) & LC_SLOT_FLAGS_PROCESSORCARD)
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "PROCESSOR");
    else
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "MEMORY");
  } /* if inserted */

  rid=LC_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");
  return 0;
}



int LC_Driver_SendReaderErrorNotification(LC_DRIVER *d,
                                          LC_READER *r,
                                          const char *text) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rid;

  assert(d);
  dbReq=GWEN_DB_Group_new("ReaderError");
  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", text);

  rid=LC_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");

  return 0;
}



int LC_Driver_RemoveCommand(LC_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound){
  assert(d);
  return GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, outbound);
}



int LC_Driver_CheckStatusChanges(LC_DRIVER *d) {
  LC_READER *r;

  r=LC_Reader_List_First(LC_Driver_GetReaders(d));
  while(r) {
    LC_READER *rnext;
    GWEN_TYPE_UINT32 retval;

    rnext=LC_Reader_List_Next(r);

    retval=LC_Driver_ReaderStatus(d, r);
    if (retval) {
      DBG_ERROR(LC_Reader_GetLogger(r), "Error getting reader status");
      LC_Driver_SendReaderErrorNotification(d, r,
                                            LC_Driver_GetErrorText(d, retval));
      DBG_NOTICE(LC_Reader_GetLogger(r),
                 "Reader \"%s\" had an error, shutting down",
                 LC_Reader_GetName(r));
      LC_Reader_List_Del(r);
      LC_Reader_free(r);
    }
    else {
      LC_SLOT_LIST *slList;
      LC_SLOT *sl;

      slList=LC_Reader_GetSlots(r);
      sl=LC_Slot_List_First(slList);
      while(sl) {
        int isInserted;
        GWEN_TYPE_UINT32 newStatus, oldStatus;
        int cardNum;

        newStatus=LC_Slot_GetStatus(sl);
        if (!(newStatus & LC_SLOT_STATUS_DISABLED)) {
          oldStatus=LC_Slot_GetLastStatus(sl);
  
          if (((newStatus^oldStatus) & LC_SLOT_STATUS_CARD_INSERTED) &&
              /*!(newStatus & LC_SLOT_STATUS_CARD_CONNECTED) && */
              (newStatus & LC_SLOT_STATUS_CARD_INSERTED)){
            /* card has just been inserted, try to connect it */
            DBG_NOTICE(LC_Reader_GetLogger(r),
                       "Card inserted, trying to connect it");
            if (LC_Driver_ConnectSlot(d, sl)) {
              DBG_ERROR(0, "Card inserted, but I can't connect to it");
            }
            newStatus=LC_Slot_GetStatus(sl);
          }
  
          isInserted=(newStatus & LC_SLOT_STATUS_CARD_CONNECTED);
          if ((newStatus^oldStatus) &
              (LC_SLOT_STATUS_CARD_CONNECTED)){
            DBG_NOTICE(LC_Reader_GetLogger(r),
                       "Status changed on slot %d (%08x->%08x) (cardnum %d)",
                       LC_Slot_GetSlotNum(sl),
                       oldStatus, newStatus,
                       LC_Slot_GetCardNum(sl));
            if (isInserted) {
              DBG_INFO(LC_Reader_GetLogger(r), "Card is now connected");
              cardNum=++LC_Driver__LastCardNum;
              LC_Slot_SetCardNum(sl, cardNum);
            }
            else {
              DBG_INFO(LC_Reader_GetLogger(r), "Card is not connected");
              cardNum=LC_Slot_GetCardNum(sl);
            }

            DBG_INFO(LC_Reader_GetLogger(r), "Card number is %d", cardNum);
  
            if (LC_Driver_SendStatusChangeNotification(d,
                                                       sl)) {
              DBG_ERROR(0, "Error sending status change notification");
            }
            else {
              DBG_INFO(0, "Server informed");
            }
            LC_Slot_SetLastStatus(sl, newStatus);
          }
          else {
            DBG_DEBUG(LC_Reader_GetLogger(r), "Status on slot %d unchanged",
                       LC_Slot_GetSlotNum(sl));
          }
        }
        sl=LC_Slot_List_Next(sl);
      } /* while slots */
    } /* if getting reader status worked */
    r=rnext;
  } /* while reader */

  return 0;
}



int LC_Driver_HandleStartReader(LC_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  const char *name;
  int port;
  int slots;
  GWEN_TYPE_UINT32 flags;
  LC_READER *r;
  char numbuf[16];
  GWEN_TYPE_UINT32 retval;
  GWEN_DB_NODE *dbRsp;

  assert(d);
  assert(dbReq);
  DBG_NOTICE(0, "Command: Start reader");
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }
  name=GWEN_DB_GetCharValue(dbReq, "body/name", 0, "noname");
  port=GWEN_DB_GetIntValue(dbReq, "body/port", 0, 0);
  flags=GWEN_DB_GetIntValue(dbReq, "body/flags", 0, 0);
  slots=GWEN_DB_GetIntValue(dbReq, "body/slots", 0, 0);
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
  r=LC_Driver_FindReaderById(d, readerId);
  if (r) {
    DBG_WARN(0, "A reader with id \"%08x\" already exists", readerId);

    DBG_NOTICE(LC_Reader_GetLogger(r), "Restarting reader");
    retval=LC_Driver_DisconnectReader(d, r);
    if (retval==0)
      retval=LC_Driver_ConnectReader(d, r);

    if (retval) {
      DBG_ERROR(0, "Could not restart reader");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Driver_GetErrorText(d, retval));
    }
    else {
      if (LC_Reader_GetReaderFlags(r) & LC_READER_FLAGS_NOINFO) {
        DBG_WARN(0, "ReaderInfo disabled");
      }
      else {
        GWEN_BUFFER *ibuf;
        GWEN_TYPE_UINT32 rv;

        ibuf=GWEN_Buffer_new(0, 256, 0, 1);
        rv=LC_Driver_ReaderInfo(d, r, ibuf);
        if (rv) {
          DBG_WARN(0, "ReaderInfo not available (%s)",
                   LC_Driver_GetErrorText(d, rv));
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

    if (LC_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LC_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
  }
  else {
    /* check whether we have a reader at that port */
    r=LC_Driver_FindReaderByPort(d, port);
    if (r) {
      DBG_ERROR(0, "A reader with port \"%08x\" already exists", port);
      /* send error result */
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           "There already is a reader at the given port");

      LC_Driver_SendResponse(d, rid, dbRsp);
      LC_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }

    /* ok to create the reader */
    r=LC_Driver_CreateReader(d, readerId, name, port, slots, flags);
    if (d->readerLogFile) {
      GWEN_BUFFER *mbuf;

      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LC_Driver_ReplaceVar(d->readerLogFile, "reader", name, mbuf);
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
          LC_Reader_SetLogger(r, name);
          GWEN_Logger_SetLevel(name, d->logLevel);
        }
        GWEN_Buffer_free(mbuf);
      }
    } /* if reader log file */

    /* init reader */
    DBG_NOTICE(LC_Reader_GetLogger(r),
               "Init reader %s", LC_Reader_GetName(r));
    retval=LC_Driver_ConnectReader(d, r);
    if (retval) {
      DBG_ERROR(LC_Reader_GetLogger(r),
                "Could not connect reader %s (%d: %s)",
                LC_Reader_GetName(r),
                retval,
                LC_Driver_GetErrorText(d, retval));
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Driver_GetErrorText(d, retval));
    }
    else {
      GWEN_BUFFER *ibuf;
      GWEN_TYPE_UINT32 rv;

      ibuf=GWEN_Buffer_new(0, 256, 0, 1);
      rv=LC_Driver_ReaderInfo(d, r, ibuf);
      if (rv) {
        DBG_WARN(0, "ReaderInfo not available (%s)",
                 LC_Driver_GetErrorText(d, rv));
      }
      else {
        DBG_NOTICE(LC_Reader_GetLogger(r), "ReaderInfo: %s",
                   GWEN_Buffer_GetStart(ibuf));
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "info",
                             GWEN_Buffer_GetStart(ibuf));
      }
      GWEN_Buffer_free(ibuf);

      DBG_NOTICE(LC_Reader_GetLogger(r), "Reader up and waiting");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Reader up and waiting");
    }

    if (LC_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LC_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
    DBG_NOTICE(LC_Reader_GetLogger(r), "Reader start handled");
    LC_Driver_AddReader(d, r);
  }
  LC_Driver_RemoveCommand(d, rid, 0);

  return 0;
}


int LC_Driver_ReplaceVar(const char *path,
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


int LC_Driver_HandleStopReader(LC_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LC_READER *r;
  char numbuf[16];
  GWEN_TYPE_UINT32 retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LC_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* TODO: send error result */
    return -1;
  }

  /* deinit reader */
  DBG_NOTICE(LC_Reader_GetLogger(r), "Disconnecting reader");
  dbRsp=GWEN_DB_Group_new("StopReaderResponse");
  retval=LC_Driver_DisconnectReader(d, r);
  if (retval!=0) {
    DBG_INFO(LC_Reader_GetLogger(r), "Could not disconnect reader");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         LC_Driver_GetErrorText(d, retval));
  }
  else {
    /* init ok */
    DBG_NOTICE(LC_Reader_GetLogger(r), "Deinit succeeded");
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
  if (LC_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LC_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Reader down");
  LC_Driver_DelReader(d, r);
  LC_Reader_free(r);
  return 0;
}



int LC_Driver_HandleResetCard(LC_DRIVER *d,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  LC_READER *r;
  int slotNum;
  int cardNum;
  LC_SLOT *slot;
  char retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LC_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LC_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LC_Slot_GetStatus(slot) & LC_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LC_Slot_GetSlotNum(slot));
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check card number and reader status */
  if ((LC_Slot_GetCardNum(slot)!=cardNum) ||
      !(LC_Slot_GetStatus(slot) & LC_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(0, "Card \"%d\" has been removed", cardNum);
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  DBG_NOTICE(LC_Reader_GetLogger(r), "Resetting card");
  retval=LC_Driver_ResetSlot(d, slot);
  if (retval!=0) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Error resetting slot (%d: %s)",
              retval,
              LC_Driver_GetErrorText(d, retval));
    LC_Driver_SendReaderErrorNotification(d, r,
                                          LC_Driver_GetErrorText(d, retval));
    DBG_NOTICE(LC_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LC_Reader_GetName(r));
    LC_Reader_List_Del(r);
    LC_Reader_free(r);
  }
  else {
    /* reset ok */
    DBG_INFO(LC_Reader_GetLogger(r), "Reset succeeded");
  }

  LC_Driver_RemoveCommand(d, rid, 0);

  return 0;
}



int LC_Driver_HandleCardCommand(LC_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LC_READER *r;
  char numbuf[16];
  unsigned char rspbuffer[300];
  const unsigned char *apdu;
  unsigned int apdulen;
  int rsplen;
  int slotNum;
  int cardNum;
  LC_SLOT *slot;
  char retval;
  const char *target;
  int toReader;
  int readerError;

  assert(d);
  assert(dbReq);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  apdu=GWEN_DB_GetBinValue(dbReq, "body/data", 0, 0, 0, &apdulen);
  if (!apdu || apdulen<4) {
    DBG_ERROR(0, "APDU too small");
    /* send error result */
    LC_Driver_SendResult(d,
                         rid,
                         "CardCommandResponse",
                         "ERROR", "APDU too small");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad slot number");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad card number");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "body/target", 0, 0);
  if (!target) {
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "No target");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  if (strcasecmp(target, "reader")==0)
    toReader=1;
  else if (strcasecmp(target, "card")==0)
    toReader=0;
  else {
    DBG_ERROR(0, "Bad target \"%s\"", target);
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Bad target");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LC_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Reader not found");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LC_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Slot not found");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LC_Slot_GetStatus(slot) & LC_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LC_Slot_GetSlotNum(slot));
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Slot diabled");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check card number and reader status */
  if ((LC_Slot_GetCardNum(slot)!=cardNum) ||
      !(LC_Slot_GetStatus(slot) & LC_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Card \"%d\" has been removed", cardNum);
    /* send error result */
    LC_Driver_SendResult(d, rid, "CardCommandResponse",
                         "ERROR", "Card has been removed");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  DBG_DEBUG(LC_Reader_GetLogger(r), "Executing command");
  GWEN_Text_LogString(apdu, apdulen, 0, GWEN_LoggerLevelDebug);
  dbRsp=GWEN_DB_Group_new("CardCommandResponse");
  rsplen=sizeof(rspbuffer)-1;
  retval=LC_Driver_SendAPDU(d, toReader, r, slot, apdu, apdulen,
                              rspbuffer, &rsplen);
  if (retval!=0) {
    DBG_ERROR(LC_Reader_GetLogger(r), "Error executing APDU (%08x)", retval);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", LC_Driver_GetErrorText(d, retval));
    readerError=retval;
  }
  else {
    /* init ok */
    DBG_DEBUG(LC_Reader_GetLogger(r), "Command succeeded");
    GWEN_Text_LogString(rspbuffer, rsplen, 0, GWEN_LoggerLevelDebug);

    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Command executed");
    assert(rsplen);
    GWEN_DB_SetBinValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "data", rspbuffer, rsplen);
    readerError=0;
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
  if (LC_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LC_Driver_RemoveCommand(d, rid, 0);
  DBG_DEBUG(0, "Response send");

  if (readerError) {
    DBG_NOTICE(LC_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LC_Reader_GetName(r));
    LC_Driver_SendReaderErrorNotification(d, r,
                                          LC_Driver_GetErrorText(d,
                                                                 readerError));
    LC_Reader_List_Del(r);
    LC_Reader_free(r);
  }

  return 0;
}



int LC_Driver_HandleStopDriver(LC_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  LC_READER_LIST *rl;
  LC_READER *r;

  assert(d);
  assert(dbReq);

  rl=LC_Driver_GetReaders(d);
  assert(rl);
  r=LC_Reader_List_First(rl);
  while(r) {
    LC_READER *nr;

    nr=LC_Reader_List_Next(r);
    /* deinit reader */
    DBG_INFO(LC_Reader_GetLogger(r),
             "Disconnecting reader \"%s\"", LC_Reader_GetName(r));
    if (LC_Driver_DisconnectReader(d, r)) {
      DBG_WARN(LC_Reader_GetLogger(r), "Could not disconnect reader");
    }
    else {
      DBG_INFO(LC_Reader_GetLogger(r), "Reader \"%s\" disconnected", LC_Reader_GetName(r));
    }
    LC_Reader_List_Del(r);
    LC_Reader_free(r);
    r=nr;
  } /* while r */

  LC_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Driver down");
  d->stopDriver=1;
  return 0;
}




int LC_Driver_Work(LC_DRIVER *d) {
  time_t lastStatusCheckTime;
  time_t t1;

  lastStatusCheckTime=(time_t)0;
  while(!d->stopDriver) {
    GWEN_NETCONNECTION_WORKRESULT res;
    GWEN_TYPE_UINT32 rid;
    int needHeartbeat;

    res=GWEN_Net_HeartBeat(750);
    if (res==GWEN_NetConnectionWorkResult_Error) {
      DBG_ERROR(0, "Network error");
      return -1;
    }
    else if (res==GWEN_NetConnectionWorkResult_NoChange) {
      DBG_VERBOUS(0, "No activity");
    }

    t1=time(0);
    if (difftime(t1, lastStatusCheckTime)>=1) {
      /* Do some hardware work */
      DBG_VERBOUS(0, "Checking for status changes");
      LC_Driver_CheckStatusChanges(d);
      lastStatusCheckTime=t1;
    }

    needHeartbeat=0;
    while(!needHeartbeat) {
      int j;

      t1=time(0);
      if (difftime(t1, lastStatusCheckTime)>=1) {
        /* Do some hardware work */
        DBG_VERBOUS(0, "Checking for status changes");
        LC_Driver_CheckStatusChanges(d);
        lastStatusCheckTime=t1;
      }
      for(j=0; ; j++) {
        int rv;

        if (j>LC_DRIVER_IPC_MAXWORK) {
          DBG_ERROR(0, "IPC running wild, aborting driver");
          return -1;
        }
        t1=time(0);
        if (difftime(t1, lastStatusCheckTime)>=1) {
          /* Do some hardware work */
          DBG_VERBOUS(0, "Checking for status changes");
          LC_Driver_CheckStatusChanges(d);
          lastStatusCheckTime=t1;
        }
        /* work as long as possible */
        rv=GWEN_IPCManager_Work(d->ipcManager, 10);
        if (rv==-1) {
          DBG_ERROR(0, "Error while working with IPC");
          return -1;
        }
        else if (rv==1)
          break;
      }

      rid=LC_Driver_GetNextInRequest(d);
      if (rid) {
        GWEN_DB_NODE *dbReq;
        const char *name;
        int didWhat;

        dbReq=LC_Driver_GetInRequestData(d, rid);
        assert(dbReq);

        /* we have an incoming message */
        didWhat=1;
        name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
        if (!name) {
          DBG_ERROR(0, "Bad IPC command (no command name), discarding");
          LC_Driver_RemoveCommand(d, rid, 0);
        }
        DBG_NOTICE(0, "Incoming request \"%s\"", name);
        if (strcasecmp(name, "StartReader")==0) {
          LC_Driver_HandleStartReader(d, rid, dbReq);
        }
        else if (strcasecmp(name, "StopReader")==0) {
          LC_Driver_HandleStopReader(d, rid, dbReq);
        }
        else if (strcasecmp(name, "CardCommand")==0) {
          LC_Driver_HandleCardCommand(d, rid, dbReq);
        }
        else if (strcasecmp(name, "ResetCard")==0) {
          LC_Driver_HandleResetCard(d, rid, dbReq);
        }
        else if (strcasecmp(name, "StopDriver")==0) {
          LC_Driver_HandleStopDriver(d, rid, dbReq);
        }
        else {
          DBG_WARN(0, "Unknown command \"%s\", discarding", name);
          didWhat=0;
        }

        if (didWhat) {
          for(j=0; ; j++) {
            int rv;

            if (j>LC_DRIVER_IPC_MAXWORK) {
              DBG_ERROR(0, "IPC running wild, aborting driver");
              return -1;
            }

            /* work as long as possible (flush responses) */
            rv=GWEN_IPCManager_Work(d->ipcManager, 10);
            if (rv==-1) {
              DBG_ERROR(0, "Error while working with IPC");
              return -1;
            }
            else if (rv==1)
              break;
          }
        } /* if didWhat */
      } /* if incoming request */
      else
        needHeartbeat=1;
    } /* while !needHeartbeat */
  } /* while driver is not to be stopped */
  return 0;
}















