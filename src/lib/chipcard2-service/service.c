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


#include "service_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/net.h>

#include <chipcard2/chipcard2.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <time.h>

GWEN_INHERIT_FUNCTIONS(LC_SERVICE)


void LC_Service_Usage(const char *prgName) {
  fprintf(stdout,
          "%s OPTONS \n"
          "[-v]              verbous\n"
          "[--logfile ARG]   name of the logfile\n"
          "[--logtype ARG]   log type\n"
          "[--loglevel ARG]  log level\n"
          "[-d ARG]          service data folder\n"
          "-b ARG            server id\n"
          "[-u ARG]          customer id of this service\n"
          "[-a ARG]          server IP address (or hostname)\n"
          "[-p ARG]          server TCP port\n"
          "-i ARG            service id for this session\n"
          "[--secure]        use encryption and signatures\n"
          , prgName
         );
}



LC_SERVICE_CHECKARGS_RESULT LC_Service_CheckArgs(LC_SERVICE *d,
                                                 int argc, char **argv) {
  int i;

  assert(d);

  d->verbous=0;
  d->secure=0;
  d->logType=GWEN_LoggerTypeConsole;
  d->logFile=strdup("service.log");
  d->logLevel=GWEN_LoggerLevelNotice; // debug
  d->serverPort=LC_DEFAULT_PORT;
  d->clients=LC_ServiceClient_List_new();
  d->typ="local";
  d->certFile=0;
  d->certDir=0;
  d->serverAddr=0;

  i=1;
  while (i<argc){
    if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      free(d->logFile);
      d->logFile=strdup(argv[i]);
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (d->logType==GWEN_LoggerTypeUnknown) {
        DBG_ERROR(0, "Unknown log type \"%s\"\n", argv[i]);
        return LC_ServiceCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (d->logLevel==GWEN_LoggerLevelUnknown) {
        DBG_ERROR(0, "Unknown log level \"%s\"\n", argv[i]);
        return LC_ServiceCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"-d")==0) {
      i++;
      if (i>=argc)
        return -1;
      d->serviceDataDir=argv[i];
    }
    else if (strcmp(argv[i],"-t")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->typ=argv[i];
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->serviceId=argv[i];
    }
    else if (strcmp(argv[i],"-a")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->serverAddr=argv[i];
    }
    else if (strcmp(argv[i],"-p")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->serverPort=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-c")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->certFile=argv[i];
    }
    else if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      d->certDir=argv[i];
    }
    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      LC_Service_Usage(argv[0]);
      return LC_ServiceCheckArgsResultHelp;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      return LC_ServiceCheckArgsResultVersion;
    }
    else if (strcmp(argv[i],"-v")==0) {
      d->verbous=1;
    }
    else {
      DBG_ERROR(0, "Unknown argument \"%s\"", argv[i]);
      return LC_ServiceCheckArgsResultError;
    }
    i++;
  } /* while */

  /* check for missing arguments */
  if (!d->serverAddr) {
    DBG_ERROR(0, "Server address missing");
    return LC_ServiceCheckArgsResultError;
  }

  if (!d->serviceId) {
    DBG_ERROR(0, "Driver id missing");
    return LC_ServiceCheckArgsResultError;
  }

  if (d->logFile==0) {
    GWEN_BUFFER *mbuf;

    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(mbuf, "service");
    GWEN_Buffer_AppendString(mbuf, ".log");
    d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
    GWEN_Buffer_free(mbuf);
  }

  return 0;
}



LC_SERVICE *LC_Service_new(int argc, char **argv) {
  LC_SERVICE *d;
  LC_SERVICE_CHECKARGS_RESULT res;
  GWEN_NETTRANSPORT *tr;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  GWEN_TYPE_UINT32 sid;

  GWEN_NEW_OBJECT(LC_SERVICE, d);
  GWEN_INHERIT_INIT(LC_SERVICE, d);

  res=LC_Service_CheckArgs(d, argc, argv);
  if (res!=LC_ServiceCheckArgsResultOk) {
    GWEN_FREE_OBJECT(d);
    return 0;
  }

  GWEN_Logger_Open(0, "service",
                   d->logFile,
                   d->logType,
                   GWEN_LoggerFacilityUser);
  GWEN_Logger_SetLevel(0, d->logLevel);

  DBG_NOTICE(0, "Starting service \"%s\"", argv[0]);

  d->ipcManager=GWEN_IPCManager_new();

  if (strcasecmp(d->typ, "local")==0) {
    /* HTTP over UDS */
    sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
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
      LC_Service_free(d);
      return 0;
    }
  }

  GWEN_NetTransport_SetPeerAddr(tr, addr);
  GWEN_InetAddr_free(addr);
  sid=GWEN_IPCManager_AddClient(d->ipcManager,
                                tr,
                                0,0,
                                LC_SERVICE_MARK_SERVICE);
  if (sid==0) {
    DBG_ERROR(0, "Could not add IPC client");
    LC_Service_free(d);
    return 0;
  }

  d->ipcId=sid;
  DBG_INFO(0, "IPC stuff initialized");


  return d;
}



void LC_Service_free(LC_SERVICE *d) {
  if (d) {
    GWEN_INHERIT_FINI(LC_SERVICE, d);
    LC_ServiceClient_List_free(d->clients);
    GWEN_IPCManager_free(d->ipcManager);
    free(d->logFile);
    GWEN_FREE_OBJECT(d);
  }
}



const char *LC_Service_GetServiceDataDir(const LC_SERVICE *d){
  assert(d);
  return d->serviceDataDir;
}



const char *LC_Service_GetLibraryFile(const LC_SERVICE *d){
  assert(d);
  return d->libraryFile;
}



const char *LC_Service_GetServiceId(const LC_SERVICE *d){
  assert(d);
  return d->serviceId;
}



GWEN_TYPE_UINT32 LC_Service_SendCommand(LC_SERVICE *d,
                                       GWEN_DB_NODE *dbCommand) {
  return GWEN_IPCManager_SendRequest(d->ipcManager,
                                     d->ipcId, dbCommand);
}



int LC_Service_SendResponse(LC_SERVICE *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand) {
  return GWEN_IPCManager_SendResponse(d->ipcManager,
                                      rid, dbCommand);
}



int LC_Service_SendResult(LC_SERVICE *d,
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
  return LC_Service_SendResponse(d, rid, db);
}



GWEN_TYPE_UINT32 LC_Service_GetNextInRequest(LC_SERVICE *d) {
  assert(d);
  return GWEN_IPCManager_GetNextInRequest(d->ipcManager,
                                          LC_SERVICE_MARK_SERVICE);
}



GWEN_DB_NODE *LC_Service_GetInRequestData(LC_SERVICE *d,
                                         GWEN_TYPE_UINT32 rid) {
  assert(d);
  return GWEN_IPCManager_GetInRequestData(d->ipcManager, rid);
}



int LC_Service__Work(LC_SERVICE *d, int timeout, int maxmsg){
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
    DBG_DEBUG(0, "Service: Working");
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



int LC_Service_CheckResponses(GWEN_DB_NODE *db) {
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



int LC_Service_Connect(LC_SERVICE *d, const char *code, const char *text){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  time_t startt;
  GWEN_TYPE_UINT32 rid;

  assert(d);

  startt=time(0);

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("ServiceReady");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", d->serviceId);
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
    if (LC_Service__Work(d, 1000, 0)) {
      DBG_ERROR(0, "Error at work");
      GWEN_IPCManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }

    if (difftime(time(0), startt)>=LC_SERVICE_STARTTIMEOUT) {
      DBG_ERROR(0, "Timeout");
      GWEN_IPCManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }
  } /* while */

  DBG_DEBUG(0, "Answer received");
  if (LC_Service_CheckResponses(dbRsp)) {
    DBG_ERROR(0, "Error returned by server, aborting");
    GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, 1);

  DBG_NOTICE(0, "Connected to server");
  return 0;
}



void LC_Service_Disconnect(LC_SERVICE *d){
  assert(d);
  if (GWEN_IPCManager_Disconnect(d->ipcManager, d->ipcId)) {
    DBG_ERROR(0, "Error while disconnecting");
  }
}



LC_SERVICECLIENT *LC_Service_FindClientById(const LC_SERVICE *d, GWEN_TYPE_UINT32 id){
  LC_SERVICECLIENT *cl;

  assert(d);
  cl=LC_ServiceClient_List_First(d->clients);
  while(cl) {
    if (id==LC_ServiceClient_GetClientId(cl))
      return cl;
    cl=LC_ServiceClient_List_Next(cl);
  } /* while */
  return 0;
}



void LC_Service_AddClient(LC_SERVICE *d, LC_SERVICECLIENT *cl){
  assert(d);
  assert(cl);

  LC_ServiceClient_List_Add(cl, d->clients);
}



void LC_Service_DelClient(LC_SERVICE *d, LC_SERVICECLIENT *cl){
  assert(d);
  assert(cl);

  LC_ServiceClient_List_Del(cl);
}



LC_SERVICECLIENT_LIST *LC_Service_GetClients(const LC_SERVICE *d){
  assert(d);
  return d->clients;
}



GWEN_TYPE_UINT32 LC_Service_Open(LC_SERVICE *d, LC_SERVICECLIENT *cl,
                                 GWEN_DB_NODE *dbData){
  assert(d);
  assert(d->openFn);
  return d->openFn(d, cl, dbData);
}



GWEN_TYPE_UINT32 LC_Service_Close(LC_SERVICE *d, LC_SERVICECLIENT *cl,
                                  GWEN_DB_NODE *dbData){
  assert(d);
  assert(d->closeFn);
  return d->closeFn(d, cl, dbData);
}



GWEN_TYPE_UINT32 LC_Service_Command(LC_SERVICE *d, LC_SERVICECLIENT *cl,
                                    GWEN_DB_NODE *dbRequest,
                                    GWEN_DB_NODE *dbResponse){
  assert(d);
  assert(d->commandFn);
  return d->commandFn(d, cl, dbRequest, dbResponse);
}



const char *LC_Service_GetErrorText(LC_SERVICE *d, GWEN_TYPE_UINT32 err){
  assert(d);
  assert(d->getErrorTextFn);
  return d->getErrorTextFn(d, err);
}



void LC_Service_SetOpenFn(LC_SERVICE *d, LC_SERVICE_OPEN_FN fn){
  assert(d);
  d->openFn=fn;
}



void LC_Service_SetCloseFn(LC_SERVICE *d, LC_SERVICE_CLOSE_FN fn){
  assert(d);
  d->closeFn=fn;
}



void LC_Service_SetCommandFn(LC_SERVICE *d, LC_SERVICE_COMMAND_FN fn){
  assert(d);
  d->commandFn=fn;
}



void LC_Service_SetGetErrorTextFn(LC_SERVICE *d,
                                  LC_SERVICE_GETERRORTEXT_FN fn){
  assert(d);
  d->getErrorTextFn=fn;
}



int LC_Service_RemoveCommand(LC_SERVICE *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound){
  assert(d);
  return GWEN_IPCManager_RemoveRequest(d->ipcManager, rid, outbound);
}



int LC_Service_HandleServiceOpen(LC_SERVICE *d,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *cl;
  GWEN_DB_NODE *dbRsp;
  GWEN_DB_NODE *dbData;

  assert(d);
  assert(dbReq);
  dbData=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                          "body/command");
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/clientId", 0, "0"),
                "%x",
                &clientId)) {
    DBG_ERROR(0, "Bad clientId");
    /* TODO: send error result */
    return -1;
  }
  name=GWEN_DB_GetCharValue(dbReq, "body/name", 0, 0);

  /* prepare response */
  dbRsp=GWEN_DB_Group_new("ServiceOpenResponse");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);

  cl=LC_Service_FindClientById(d, clientId);
  if (cl) {
    DBG_ERROR(0, "A client with id \"%08x\" already exists", clientId);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "Client already connected");
  }
  else {
    GWEN_TYPE_UINT32 res;

    cl=LC_ServiceClient_new(clientId);
    res=LC_Service_Open(d, cl, dbData);
    if (res!=0) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Service_GetErrorText(d, res));
      LC_ServiceClient_free(cl);
    }
    else {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Client registered");
      LC_Service_AddClient(d, cl);
    }
  }

  if (LC_Service_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Service_RemoveCommand(d, rid, 0);
    return -1;
  }

  LC_Service_RemoveCommand(d, rid, 0);

  return 0;
}



int LC_Service_HandleServiceClose(LC_SERVICE *d,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *cl;
  GWEN_DB_NODE *dbRsp;
  GWEN_DB_NODE *dbData;

  assert(d);
  assert(dbReq);
  dbData=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                          "body/command");
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/clientId", 0, "0"),
                "%x",
                &clientId)) {
    DBG_ERROR(0, "Bad clientId");
    /* TODO: send error result */
    return -1;
  }
  name=GWEN_DB_GetCharValue(dbReq, "body/name", 0, 0);

  /* prepare response */
  dbRsp=GWEN_DB_Group_new("ServiceCloseResponse");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);

  cl=LC_Service_FindClientById(d, clientId);
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "Client not found");
  }
  else {
    GWEN_TYPE_UINT32 res;

    res=LC_Service_Close(d, cl, dbData);
    if (res!=0) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Service_GetErrorText(d, res));
    }
    else {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Client registered");
    }
    LC_ServiceClient_List_Del(cl);
    LC_ServiceClient_free(cl);
  }

  if (LC_Service_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Service_RemoveCommand(d, rid, 0);
    return -1;
  }

  LC_Service_RemoveCommand(d, rid, 0);

  return 0;
}



int LC_Service_HandleServiceCommand(LC_SERVICE *d,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *cl;
  GWEN_DB_NODE *dbRsp;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/clientId", 0, "0"),
                "%x",
                &clientId)) {
    DBG_ERROR(0, "Bad clientId");
    /* TODO: send error result */
    return -1;
  }
  name=GWEN_DB_GetCharValue(dbReq, "body/name", 0, 0);

  /* prepare response */
  dbRsp=GWEN_DB_Group_new("ServiceCommandResponse");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);

  cl=LC_Service_FindClientById(d, clientId);
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "Client not found");
  }
  else {
    GWEN_DB_NODE *dbCommandRequest;

    dbCommandRequest=GWEN_DB_GetGroup(dbReq,
                                      GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                      "body/command");
    if (!dbCommandRequest) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Bad request: No command data given");
    }
    else {
      GWEN_TYPE_UINT32 res;
      GWEN_DB_NODE *dbCommandResponse;

      dbCommandResponse=GWEN_DB_GetGroup(dbRsp,
                                         GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                         "response");
      assert(dbCommandResponse);
      res=LC_Service_Command(d, cl, dbCommandRequest, dbCommandResponse);
      if (res!=0) {
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "ERROR");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text",
                             LC_Service_GetErrorText(d, res));
      }
      else {
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "OK");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Command executed");
      }
    }
  }

  if (LC_Service_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Service_RemoveCommand(d, rid, 0);
    return -1;
  }

  LC_Service_RemoveCommand(d, rid, 0);

  return 0;
}








int LC_Service_Work(LC_SERVICE *d) {

  while(!d->stopService) {
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

    needHeartbeat=0;
    while(!needHeartbeat) {
      int j;

      for(j=0; ; j++) {
        int rv;

        if (j>LC_SERVICE_IPC_MAXWORK) {
          DBG_ERROR(0, "IPC running wild, aborting service");
          return -1;
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

      rid=LC_Service_GetNextInRequest(d);
      if (rid) {
        GWEN_DB_NODE *dbReq;
        const char *name;
        int didWhat;

        dbReq=LC_Service_GetInRequestData(d, rid);
        assert(dbReq);

        /* we have an incoming message */
        didWhat=1;
        name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
        if (!name) {
          DBG_ERROR(0, "Bad IPC command (no command name), discarding");
          LC_Service_RemoveCommand(d, rid, 0);
        }
        DBG_NOTICE(0, "Incoming request \"%s\"", name);
        if (strcasecmp(name, "ServiceOpen")==0) {
          LC_Service_HandleServiceOpen(d, rid, dbReq);
        }
        else if (strcasecmp(name, "ServiceClose")==0) {
          LC_Service_HandleServiceClose(d, rid, dbReq);
        }
        else if (strcasecmp(name, "ServiceCommand")==0) {
          LC_Service_HandleServiceCommand(d, rid, dbReq);
        }
        else {
          DBG_WARN(0, "Unknown command \"%s\", discarding", name);
          didWhat=0;
        }

        if (didWhat) {
          for(j=0; ; j++) {
            int rv;

            if (j>LC_SERVICE_IPC_MAXWORK) {
              DBG_ERROR(0, "IPC running wild, aborting service");
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
  } /* while service is not to be stopped */
  return 0;
}
















