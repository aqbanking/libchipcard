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

GWEN_INHERIT(LC_CLIENT, LC_SERVICE_CLIENT)


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



LC_SERVICE_CHECKARGS_RESULT LC_Service_CheckArgs(LC_CLIENT *cl,
                                                 int argc, char **argv) {
  int i;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  sv->verbous=0;
  sv->secure=0;
  sv->logType=GWEN_LoggerTypeConsole;
  sv->logFile=strdup("service.log");
  sv->logLevel=GWEN_LoggerLevelNotice; // debug
  sv->serverPort=LC_DEFAULT_PORT;
  sv->clients=LC_ServiceClient_List_new();
  sv->typ="local";
  sv->certFile=0;
  sv->certDir=0;
  sv->serverAddr=0;

  i=1;
  while (i<argc){
    if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      free(sv->logFile);
      sv->logFile=strdup(argv[i]);
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (sv->logType==GWEN_LoggerTypeUnknown) {
        DBG_ERROR(0, "Unknown log type \"%s\"\n", argv[i]);
        return LC_ServiceCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (sv->logLevel==GWEN_LoggerLevelUnknown) {
        DBG_ERROR(0, "Unknown log level \"%s\"\n", argv[i]);
        return LC_ServiceCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"-d")==0) {
      i++;
      if (i>=argc)
        return -1;
      sv->serviceDataDir=argv[i];
    }
    else if (strcmp(argv[i],"-t")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->typ=argv[i];
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->serviceId=argv[i];
    }
    else if (strcmp(argv[i],"-a")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->serverAddr=argv[i];
    }
    else if (strcmp(argv[i],"-p")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->serverPort=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-c")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->certFile=argv[i];
    }
    else if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
        return LC_ServiceCheckArgsResultError;
      sv->certDir=argv[i];
    }
    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      LC_Service_Usage(argv[0]);
      return LC_ServiceCheckArgsResultHelp;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      return LC_ServiceCheckArgsResultVersion;
    }
    else if (strcmp(argv[i],"-v")==0) {
      sv->verbous=1;
    }
    else {
      DBG_ERROR(0, "Unknown argument \"%s\"", argv[i]);
      return LC_ServiceCheckArgsResultError;
    }
    i++;
  } /* while */

  /* check for missing arguments */
  if (!sv->serverAddr) {
    DBG_ERROR(0, "Server address missing");
    return LC_ServiceCheckArgsResultError;
  }

  if (!sv->serviceId) {
    DBG_ERROR(0, "Driver id missing");
    return LC_ServiceCheckArgsResultError;
  }

  if (sv->logFile==0) {
    GWEN_BUFFER *mbuf;

    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(mbuf, "service");
    GWEN_Buffer_AppendString(mbuf, ".log");
    sv->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
    GWEN_Buffer_free(mbuf);
  }

  return 0;
}



LC_CLIENT *LC_Service_new(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_SERVICE_CLIENT *sv;
  LC_SERVICE_CHECKARGS_RESULT res;
  GWEN_DB_NODE *dbConfig;
  GWEN_DB_NODE *dbServer;

  cl=LC_Client_new(argv[0], "0", 0);
  GWEN_NEW_OBJECT(LC_SERVICE_CLIENT, sv);
  GWEN_INHERIT_SETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl, sv,
                       LC_Service_freeData);
  LC_Client_SetHandleInRequestFn(cl, LC_Service_HandleInRequest);

  res=LC_Service_CheckArgs(cl, argc, argv);
  if (res!=LC_ServiceCheckArgsResultOk) {
    LC_Client_free(cl);
    return 0;
  }

  GWEN_Logger_Open(0, "service",
                   sv->logFile,
                   sv->logType,
                   GWEN_LoggerFacilityUser);
  GWEN_Logger_SetLevel(0, sv->logLevel);

  DBG_NOTICE(0, "Starting service \"%s\"", argv[0]);

  dbConfig=GWEN_DB_Group_new("config");
  dbServer=GWEN_DB_GetGroup(dbConfig, GWEN_DB_FLAGS_DEFAULT, "server");
  GWEN_DB_SetCharValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "typ", sv->typ);
  GWEN_DB_SetCharValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "addr", sv->serverAddr);
  GWEN_DB_SetIntValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "port", sv->serverPort);
  if (sv->certDir)
    GWEN_DB_SetCharValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "certDir", sv->certDir);
  if (sv->certDir)
    GWEN_DB_SetCharValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "certFile", sv->certFile);
  GWEN_DB_SetIntValue(dbServer, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "secure", sv->secure);

  if (LC_Client_ReadConfig(cl, dbConfig)) {
    DBG_INFO(0, "here");
    GWEN_DB_Group_free(dbConfig);
    LC_Client_free(cl);
    return 0;
  }
  GWEN_DB_Group_free(dbConfig);

  return cl;
}



void LC_Service_freeData(void *bp, void *p) {
  LC_SERVICE_CLIENT *sv;

  assert(p);
  sv=(LC_SERVICE_CLIENT*) p;
  assert(sv);

  LC_ServiceClient_List_free(sv->clients);
  free(sv->logFile);
  GWEN_FREE_OBJECT(sv);
}



const char *LC_Service_GetServiceDataDir(const LC_CLIENT *cl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  return sv->serviceDataDir;
}



const char *LC_Service_GetLibraryFile(const LC_CLIENT *cl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  return sv->libraryFile;
}



const char *LC_Service_GetServiceId(const LC_CLIENT *cl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  return sv->serviceId;
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



LC_SERVICECLIENT *LC_Service_FindClientById(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  LC_SERVICECLIENT *scl;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  scl=LC_ServiceClient_List_First(sv->clients);
  while(cl) {
    if (id==LC_ServiceClient_GetClientId(scl))
      return scl;
    scl=LC_ServiceClient_List_Next(scl);
  } /* while */
  return 0;
}



void LC_Service_AddClient(LC_CLIENT *cl, LC_SERVICECLIENT *scl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  LC_ServiceClient_List_Add(scl, sv->clients);
}



void LC_Service_DelClient(LC_CLIENT *cl, LC_SERVICECLIENT *scl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  LC_ServiceClient_List_Del(scl);
}



LC_SERVICECLIENT_LIST *LC_Service_GetClients(const LC_CLIENT *cl){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  return sv->clients;
}



GWEN_TYPE_UINT32 LC_Service_Open(LC_CLIENT *cl, LC_SERVICECLIENT *scl,
                                 GWEN_DB_NODE *dbData){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  assert(sv->openFn);
  return sv->openFn(cl, scl, dbData);
}



GWEN_TYPE_UINT32 LC_Service_Close(LC_CLIENT *cl, LC_SERVICECLIENT *scl,
                                  GWEN_DB_NODE *dbData){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  assert(sv->closeFn);
  return sv->closeFn(cl, scl, dbData);
}



GWEN_TYPE_UINT32 LC_Service_Command(LC_CLIENT *cl, LC_SERVICECLIENT *scl,
                                    GWEN_DB_NODE *dbRequest,
                                    GWEN_DB_NODE *dbResponse){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  assert(sv->commandFn);
  return sv->commandFn(cl, scl, dbRequest, dbResponse);
}



const char *LC_Service_GetErrorText(LC_CLIENT *cl, GWEN_TYPE_UINT32 err){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  assert(sv->getErrorTextFn);
  return sv->getErrorTextFn(cl, err);
}



void LC_Service_SetOpenFn(LC_CLIENT *cl, LC_SERVICE_OPEN_FN fn){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  sv->openFn=fn;
}



void LC_Service_SetCloseFn(LC_CLIENT *cl, LC_SERVICE_CLOSE_FN fn){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  sv->closeFn=fn;
}



void LC_Service_SetCommandFn(LC_CLIENT *cl, LC_SERVICE_COMMAND_FN fn){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  sv->commandFn=fn;
}



void LC_Service_SetGetErrorTextFn(LC_CLIENT *cl,
                                  LC_SERVICE_GETERRORTEXT_FN fn){
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
  sv->getErrorTextFn=fn;
}



int LC_Service_HandleServiceOpen(LC_CLIENT *cl,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *scl;
  GWEN_DB_NODE *dbRsp;
  GWEN_DB_NODE *dbData;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

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

  scl=LC_Service_FindClientById(cl, clientId);
  if (scl) {
    DBG_ERROR(0, "A client with id \"%08x\" already exists", clientId);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "Client already connected");
  }
  else {
    GWEN_TYPE_UINT32 res;

    scl=LC_ServiceClient_new(clientId);
    res=LC_Service_Open(cl, scl, dbData);
    if (res!=0) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Service_GetErrorText(cl, res));
      LC_ServiceClient_free(scl);
    }
    else {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Client registered");
      LC_Service_AddClient(cl, scl);
    }
  }

  if (LC_Client_SendResponse(cl, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Client_RemoveInRequest(cl, rid);
    return -1;
  }

  LC_Client_RemoveInRequest(cl, rid);

  return 0;
}



int LC_Service_HandleServiceClose(LC_CLIENT *cl,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *scl;
  GWEN_DB_NODE *dbRsp;
  GWEN_DB_NODE *dbData;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
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

  scl=LC_Service_FindClientById(cl, clientId);
  if (!scl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "Client not found");
  }
  else {
    GWEN_TYPE_UINT32 res;

    res=LC_Service_Close(cl, scl, dbData);
    if (res!=0) {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "ERROR");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LC_Service_GetErrorText(cl, res));
    }
    else {
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "code", "OK");
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Client registered");
    }
    LC_ServiceClient_List_Del(scl);
    LC_ServiceClient_free(scl);
  }

  if (LC_Client_SendResponse(cl, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Client_RemoveInRequest(cl, rid);
    return -1;
  }

  LC_Client_RemoveInRequest(cl, rid);

  return 0;
}



int LC_Service_HandleServiceCommand(LC_CLIENT *cl,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 clientId;
  const char *name;
  char numbuf[16];
  LC_SERVICECLIENT *scl;
  GWEN_DB_NODE *dbRsp;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);
  
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

  scl=LC_Service_FindClientById(cl, clientId);
  if (!scl) {
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
      res=LC_Service_Command(cl, scl, dbCommandRequest, dbCommandResponse);
      if (res!=0) {
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "ERROR");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text",
                             LC_Service_GetErrorText(cl, res));
      }
      else {
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "OK");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Command executed");
      }
    }
  }

  if (LC_Client_SendResponse(cl, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LC_Client_RemoveInRequest(cl, rid);
    return -1;
  }

  LC_Client_RemoveInRequest(cl, rid);

  return 0;
}



int LC_Service_HandleInRequest(LC_CLIENT *cl,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  const char *name;
  int rv;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC command (no command name), discarding");
    LC_Client_RemoveInRequest(cl, rid);
    return -1;
  }
  DBG_NOTICE(0, "Incoming request \"%s\"", name);
  if (strcasecmp(name, "ServiceOpen")==0) {
    rv=LC_Service_HandleServiceOpen(cl, rid, dbReq);
  }
  else if (strcasecmp(name, "ServiceClose")==0) {
    rv=LC_Service_HandleServiceClose(cl, rid, dbReq);
  }
  else if (strcasecmp(name, "ServiceCommand")==0) {
    rv=LC_Service_HandleServiceCommand(cl, rid, dbReq);
  }
  else {
    rv=1;
  }
  return rv;
}








int LC_Service_Work(LC_CLIENT *cl) {
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  while(!sv->stopService) {
    LC_CLIENT_RESULT res;

    /* work for 10 seconds */
    res=LC_Client_Work_Wait(cl, 10);
    if (res!=LC_Client_ResultOk &&
        res!=LC_Client_ResultWait) {
      DBG_ERROR(0, "Error while working (%d)", res);
      return 2;
    }
  } /* while service is not to be stopped */
  return 0;
}



int LC_Service_Connect(LC_CLIENT *cl, const char *code, const char *text){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 rid;
  LC_SERVICE_CLIENT *sv;

  assert(cl);
  sv=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_SERVICE_CLIENT, cl);
  assert(sv);

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("ServiceReady");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", sv->serviceId);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", code);
  if (text)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", text);

  rid=LC_Client_SendRequest(cl,
                            0, /* no card */
                            0, /* all servers */
                            dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  dbRsp=LC_Client_WaitForNextResponse(cl, rid, 10);
  if (dbRsp) {
    DBG_DEBUG(0, "Answer received");
    if (LC_Service_CheckResponses(dbRsp)) {
      DBG_ERROR(0, "Error returned by server, aborting");
      LC_Client_DeleteRequest(cl, rid);
      return -1;
    }
    LC_Client_DeleteRequest(cl, rid);
    DBG_NOTICE(0, "Connected to server");
    return 0;
  }
  else {
    DBG_ERROR(0, "Could not connect to server");
    return -1;
  }
}













