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


#include "cardserver_p.h"
#include "serverconn_l.h"
#include "card_l.h"
#include <gwenhywfar/version.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/server/usbmonitor.h>
#include <chipcard2-server/server/usbttymonitor.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
# define DIRSEPC '\\'
#else
# define DIRSEP "/"
# define DIRSEPC '/'
#endif



int LC_CardServer_CheckServiceRsp(LC_CARDSERVER *cs,
                                  LC_REQUEST *rq,
                                  GWEN_DB_NODE *dbServiceRsp,
                                  GWEN_DB_NODE *dbRsp){
  const char *cmdName;
  const char *rspName;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbServiceRspBody;

  assert(rq);
  dbReq=LC_Request_GetInRequestData(rq);
  assert(dbReq);
  cmdName=GWEN_DB_GetCharValue(dbReq, "body/command/name", 0, 0);
  assert(cmdName);

  DBG_VERBOUS(0, "Service response was this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbServiceRsp, stderr, 2);

  dbServiceRspBody=GWEN_DB_GetGroup(dbServiceRsp,
                                    GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                    "body");
  assert(dbServiceRspBody);
  rspName=GWEN_DB_GetCharValue(dbServiceRsp, "command/vars/cmd",0, 0);
  assert(rspName);

  /* check for error */
  if (strcasecmp(rspName, "error")==0) {
    DBG_ERROR(0, "Service responded with error msg");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);
    return 0;
  }
  else if (strcasecmp(rspName, "ServiceCommandResponse")==0) {
    GWEN_DB_NODE *dbCommandRsp;
    GWEN_BUFFER *buf;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    GWEN_Buffer_AppendString(buf, cmdName);
    GWEN_Buffer_AppendString(buf, "Response");
    GWEN_DB_GroupRename(dbRsp, GWEN_Buffer_GetStart(buf));
    GWEN_Buffer_free(buf);

    dbCommandRsp=GWEN_DB_GetGroup(dbServiceRspBody,
                                  GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                  "command");
    if (dbCommandRsp)
      GWEN_DB_AddGroupChildren(dbRsp, dbCommandRsp);
  }
  else {
    DBG_ERROR(0, "Unknown service response \"%s\"", rspName);
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "internal error "
                         "(unknown service response)");
    return 0;
  }

  return 0;
}



int LC_CardServer_HandleServiceReady(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq){
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 nodeId;
  LC_SERVICE *as;
  LC_CLIENT *cl;
  const char *code;
  const char *text;
  int i;
  GWEN_NETCONNECTION *conn;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  DBG_NOTICE(0, "Service %08x: ServiceReady", nodeId);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* find client for service */
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==nodeId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" for service not found", nodeId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown client id");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid service id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  serviceId=i;
  if (serviceId==0) {
    DBG_ERROR(0, "Invalid service id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* find service */
  as=LC_Service_List_First(cs->services);
  while(as) {
    if (LC_Service_GetServiceId(as)==serviceId)
      break;
    as=LC_Service_List_Next(as);
  } /* while */
  if (!as) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Service not found");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* store node id */
  LC_Service_SetIpcId(as, nodeId);

  /* take over connection */
  conn=GWEN_IPCManager_GetConnection(cs->ipcManager, nodeId);
  LC_ServerConn_SetService(conn, as);

  /* check code */
  code=GWEN_DB_GetCharValue(dbReq, "body/code", 0, "<none>");
  text=GWEN_DB_GetCharValue(dbReq, "body/text", 0, "<none>");
  if (strcasecmp(code, "OK")!=0) {
    DBG_ERROR(0, "Error in service \"%08x\": %s",
              serviceId, text);
    LC_Service_SetStatus(as, LC_ServiceStatusAborted);
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  DBG_NOTICE(0, "Service \"%08x\" is up (%s)", serviceId, text);
  LC_Service_SetStatus(as, LC_ServiceStatusUp);
  LC_CardServer_SendServiceNotification(cs, 0,
                                        LC_NOTIFY_CODE_SERVICE_UP,
                                        as,
                                        "Service up.");

  dbRsp=GWEN_DB_Group_new("ServiceReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "text", "Service registered");
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
  }

  return 0;
}



int LC_CardServer_HandleServiceOpen(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  LC_SERVICE *sv;
  const char *serviceName;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Unknown client id");
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: ServiceOpen [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service name */
  serviceName=GWEN_DB_GetCharValue(dbReq, "body/serviceName", 0, 0);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &serviceId)) {
    DBG_ERROR(0, "Invalid service id given");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Invalid service id");
    return -1;
  }
  if ((serviceName!=0) ^  (serviceId!=0)) {
    DBG_ERROR(0, "Either service name *or* service id must be given");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Need service name *or* id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (serviceId) {
      GWEN_TYPE_UINT32 sid;

      sid=LC_Service_GetIpcId(sv);
      if (sid==serviceId)
        break;
    }
    else {
      const char *sn;

      sn=LC_Service_GetServiceName(sv);
      if (sn)
        if (strcasecmp(sn, serviceName)==0)
          break;
    }
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    if (serviceId) {
      DBG_ERROR(0, "Service with id \"%08x\" not found", serviceId);
    }
    else {
      DBG_ERROR(0, "Service with name \"%s\" not found", serviceName);
    }
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceOpen");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  LC_Service_IncActiveClientsCount(sv);
  LC_Client_AddService(cl, LC_Service_GetServiceId(sv));

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceClose(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 contextId;
  LC_SERVICE *sv;
  const char *serviceName=0;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Unknown client id");
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: ServiceClose [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &serviceId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing service id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    const char *sn;

    sn=LC_Service_GetServiceName(sv);
    if (sn)
      if (strcasecmp(sn, serviceName)==0)
        break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%s\" not found", serviceName);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service name");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceClose");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  LC_Service_DecActiveClientsCount(sv);
  LC_Client_DelService(cl, LC_Service_GetServiceId(sv));

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceCommand(LC_CARDSERVER *cs,
                                       GWEN_TYPE_UINT32 rid,
                                       GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 contextId;
  GWEN_TYPE_UINT32 serviceId;
  LC_SERVICE *sv;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Unknown client id");
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: ServiceCommand [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &serviceId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing service id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceCommand");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceNotification(LC_CARDSERVER *cs,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 contextId;
  LC_SERVICE *sv;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];
  GWEN_TYPE_UINT32 ridOut;

  assert(dbReq);
  serviceId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(serviceId);

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get client id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/clientId", 0, "0"),
                "%x", &clientId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing client id");
    return -1;
  }

  /* find client */
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Unknown client id");
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: ServiceNotification for %s/%s",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceNotification");

  /* send service id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  /* send client id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  /* send context id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  /* send some info about the sending service */
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceName",
                       LC_Service_GetServiceName(sv));
  /* send service flags */
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "autoload");
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "silent");
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "client");

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  ridOut=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Client_GetClientId(cl),
                                     dbServiceReq);
  if (ridOut==0) {
    DBG_ERROR(0, "Could not send \"ServiceNotification\" to client");
  }
  else {
    /* remove request, we don't expect an answer */
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridOut, 1);
  }

  /* remove incoming request */
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  DBG_INFO(0, "Request forwarded");
  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_SendStopService(LC_CARDSERVER *cs,
                                               const LC_SERVICE *as) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;

  assert(as);
  dbReq=GWEN_DB_Group_new("StopService");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Service_GetServiceId(as));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Service_GetIpcId(as),
                                     dbReq);
}



int LC_CardServer_StartService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  LC_SERVICE_STATUS st;
  GWEN_PROCESS *p;
  GWEN_BUFFER *pbuf;
  GWEN_BUFFER *abuf;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;

  assert(cs);
  assert(as);

  DBG_INFO(0, "Starting service \"%s\"", LC_Service_GetServiceName(as));
  st=LC_Service_GetStatus(as);
  if (st!=LC_ServiceStatusDown) {
    DBG_ERROR(0, "Bad service status (%d)", st);
    return -1;
  }

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LC_Service_GetServiceDataDir(as);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LC_Service_GetLogFile(as);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  if (cs->typeForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, cs->typeForDrivers);
  }

  if (cs->addrForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, cs->addrForDrivers);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", cs->portForDrivers);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Service_GetServiceId(as));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LC_Service_GetServiceName(as);
  if (!s) {
    DBG_ERROR(0, "No service type");
    LC_Service_SetStatus(as, LC_ServiceStatusDisabled);
    GWEN_Buffer_free(abuf);
    return -1;
  }

  pbuf=GWEN_Buffer_new(0, 128, 0, 1);
  GWEN_Buffer_AppendString(pbuf, LC_SERVICE_PATH);
  GWEN_Buffer_AppendByte(pbuf, '/');
  while(*s) {
    GWEN_Buffer_AppendByte(pbuf, tolower(*s));
    s++;
  } /* while */

  p=GWEN_Process_new();
  DBG_NOTICE(0, "Starting service process for service \"%s\" (%s)",
             LC_Service_GetServiceName(as), GWEN_Buffer_GetStart(pbuf));
  DBG_NOTICE(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

  pst=GWEN_Process_Start(p,
                         GWEN_Buffer_GetStart(pbuf),
                         GWEN_Buffer_GetStart(abuf));
  if (pst!=GWEN_ProcessStateRunning) {
    DBG_ERROR(0, "Unable to execute \"%s %s\"",
              GWEN_Buffer_GetStart(pbuf),
              GWEN_Buffer_GetStart(abuf));
    GWEN_Process_free(p);
    GWEN_Buffer_free(pbuf);
    GWEN_Buffer_free(abuf);
    return -1;
  }
  DBG_INFO(0, "Process started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LC_Service_SetProcess(as, p);
  LC_Service_SetStatus(as, LC_ServiceStatusStarted);
  LC_CardServer_SendServiceNotification(cs, 0,
                                        LC_NOTIFY_CODE_SERVICE_START,
                                        as,
                                        "Service started.");
  return 0;
}



int LC_CardServer_StopService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  GWEN_TYPE_UINT32 rid;

  assert(as);
  rid=LC_Service_GetCurrentRequestId(as);
  if (rid) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
    LC_Service_SetCurrentRequestId(as, 0);
  }
  rid=LC_CardServer_SendStopService(cs, as);
  if (!rid) {
    DBG_ERROR(0, "Could not send StopService command for service \"%s\"",
              LC_Service_GetServiceName(as));
    LC_Service_SetStatus(as, LC_ServiceStatusAborted);
    LC_CardServer_SendServiceNotification(cs, 0,
                                          LC_NOTIFY_CODE_SERVICE_ERROR,
                                          as,
                                          "Service IPC error.");
    return -1;
  }
  DBG_DEBUG(0, "Sent StopService command for service \"%s\"",
            LC_Service_GetServiceName(as));
  LC_Service_SetStatus(as, LC_ServiceStatusStopping);
  return 0;
}



int LC_CardServer_CheckService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  int done;

  assert(cs);
  assert(as);

  done=0;
  if (LC_Service_GetStatus(as)==LC_ServiceStatusAborted) {
    if (cs->driverRestartTime &&
        difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
        cs->driverRestartTime) {
      LC_Service_SetStatus(as, LC_ServiceStatusDown);
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusStarted) {
    /* service started, check timeout */
    if (cs->driverStartTimeout &&
        difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
        cs->driverStartTimeout) {
      GWEN_PROCESS *p;
      GWEN_PROCESS_STATE pst;

      DBG_WARN(0, "Service \"%s\" timed out", LC_Service_GetServiceName(as));
      p=LC_Service_GetProcess(as);
      assert(p);

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_SILENT) {
          LC_Service_SetStatus(as, LC_ServiceStatusSilentRunning);
          DBG_NOTICE(0, "Silent service \"%s\" is running",
                     LC_Service_GetServiceName(as));
          LC_CardServer_SendServiceNotification(cs, 0,
                                                LC_NOTIFY_CODE_SERVICE_UP,
                                                as,
                                                "Service running silently.");
          return 1;
        }
        else {
          DBG_WARN(0,
                   "Service is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Service_SetProcess(as, 0);
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          LC_CardServer_SendServiceNotification(cs, 0,
                                                LC_NOTIFY_CODE_SERVICE_ERROR,
                                                as,
                                                "Service not ready, "
                                                "killing it");
          return -1;
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Service terminated normally");
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusDown);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_DOWN,
                                              as,
                                              "Service terminated normally.");
        done++;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Service terminated abnormally");
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Service terminated abnormally");
        return -1;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Service has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Service stopped, killed.");
        return -1;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Unknown status, killed.");
        return -1;
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for service start");
      return 1;
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusStopping) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;

    p=LC_Service_GetProcess(as);
    assert(p);

    pst=GWEN_Process_CheckState(p);
    if (pst==GWEN_ProcessStateRunning) {
      if (cs->serviceStopTimeout &&
          difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
          cs->serviceStopTimeout) {
        DBG_WARN(0, "Service is still running, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Service still running, "
                                              "killing it");
        return -1;
      }
      else {
        /* otherwise give the process a little bit time ... */
        DBG_DEBUG(0, "still waiting for service to go down");
        return 1;
      }
    }
    else if (pst==GWEN_ProcessStateExited) {
      DBG_WARN(0, "Service terminated normally");
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusDown);
      LC_CardServer_SendServiceNotification(cs, 0,
                                            LC_NOTIFY_CODE_SERVICE_DOWN,
                                            as,
                                            "Service terminated normally.");
      done++;
      /* no return here, we want to continue below */
    }
    else if (pst==GWEN_ProcessStateAborted) {
      DBG_WARN(0, "Service terminated abnormally");
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      LC_CardServer_SendServiceNotification(cs, 0,
                                            LC_NOTIFY_CODE_SERVICE_ERROR,
                                            as,
                                            "Service terminated abnormally.");
      return 0;
    }
    else if (pst==GWEN_ProcessStateStopped) {
      DBG_WARN(0, "Service has been stopped, killing it");
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      LC_CardServer_SendServiceNotification(cs, 0,
                                            LC_NOTIFY_CODE_SERVICE_ERROR,
                                            as,
                                            "Service stopped, killed.");
      return 0;
    }
    else {
      DBG_ERROR(0, "Unknown process status %d, killing", pst);
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      LC_CardServer_SendServiceNotification(cs, 0,
                                            LC_NOTIFY_CODE_SERVICE_ERROR,
                                            as,
                                            "Unknown status, killed.");
      return 0;
    }
  } /* if stopping */

  if (LC_Service_GetStatus(as)==LC_ServiceStatusDown) {
    if (LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_AUTOLOAD) {
      DBG_NOTICE(0, "Loading service \"%s\"", LC_Service_GetServiceName(as));
      if (LC_CardServer_StartService(cs, as)) {
        DBG_ERROR(0, "Could not start service \"%08x\"",
                  LC_Service_GetServiceId(as));
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Could not start service");
      }
      else {
        DBG_NOTICE(0, "Started service \"%08x\"",
                   LC_Service_GetServiceId(as));
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_START,
                                              as, "Service started");
      }
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusUp ||
      LC_Service_GetStatus(as)==LC_ServiceStatusSilentRunning) {
    GWEN_TYPE_UINT32 rid;

    if (!(LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_CLIENT)) {
      GWEN_PROCESS *p;
      GWEN_PROCESS_STATE pst;

      /* check whether the service really is still up and running */
      p=LC_Service_GetProcess(as);
      assert(p);
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Service is not running anymore");
        GWEN_Process_Terminate(p);
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        LC_CardServer_SendServiceNotification(cs, 0,
                                              LC_NOTIFY_CODE_SERVICE_ERROR,
                                              as,
                                              "Service lost.");
        /* TODO: send error responses for all listed requests */

        return -1;
      }
    }

    /* check for requests to send */
    DBG_DEBUG(0, "Service still running");
    rid=LC_Service_GetCurrentRequestId(as);
    if (rid) {
      LC_REQUEST *rq;
      int rv;

      /* there is a current request, check whether is is answered */
      rq=LC_CardServer_GetRequest(cs, rid);
      assert(rq);
      rv=LC_CardServer_CheckRequest(cs, rq);
      if (rv==1) {
        if (cs->serviceCommandTimeout &&
            difftime(time(0), LC_Service_GetCommandTime(as))>=
            cs->serviceCommandTimeout) {
          /* service timed out */
          DBG_WARN(0, "Service \"%s\" timed out",
                   LC_Service_GetServiceName(as));
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          LC_CardServer_SendServiceNotification(cs, 0,
                                                LC_NOTIFY_CODE_SERVICE_ERROR,
                                                as,
                                                "Service timed out.");
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
          LC_Service_SetCurrentRequestId(as, 0);
          return -1;
        } /* if timeout */
        DBG_DEBUG(0, "Still some time left");
        return 1;
      }
      else {
        DBG_DEBUG(0, "Removing request");
        LC_CardServer_RemoveRequest(cs, rq);
        LC_Service_SetCurrentRequestId(as, 0);
        rid=0;
        done++;
      }
    } /* if rid */

    if (rid==0) {
      LC_REQUEST *rq;

      /* no current command, get the next one from queue */
      rq=LC_Service_GetNextRequest(as);
      if (rq) {
        GWEN_TYPE_UINT32 rid;
        GWEN_DB_NODE *dbReq;
        const char *cmdName;

        /* found a request */
        LC_Request_List_Add(rq, cs->requests);
        dbReq=LC_Request_GetOutRequestData(rq);
        assert(dbReq);
        cmdName=GWEN_DB_GroupName(dbReq);
        assert(cmdName);
        rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                        LC_Service_GetIpcId(as),
                                        GWEN_DB_Group_dup(dbReq));
        if (!rid) {
          DBG_ERROR(0, "Could not send command \"%s\" for service \"%s\"",
                    cmdName,
                    LC_Service_GetServiceName(as));
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          LC_CardServer_SendServiceNotification(cs, 0,
                                                LC_NOTIFY_CODE_SERVICE_ERROR,
                                                as,
                                                "Service IPC error, aborted");
          return -1;
        }
        DBG_DEBUG(0, "Sent request \"%s\" for service \"%s\" (%08x)",
                  cmdName,
                  LC_Service_GetServiceName(as),
                  rid);
        LC_Request_SetOutRequestId(rq, rid);
        LC_Service_SetCurrentRequestId(as, LC_Request_GetRequestId(rq));
        done++;
      } /* if there was a next request */
    } /* if there was no current command */

    if (!(LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_CLIENT)) {
      if (LC_Service_GetActiveClientsCount(as)==0 &&
          cs->serviceIdleTimeout){
        time_t t;

        /* check for idle timeout */
        t=LC_Service_GetIdleSince(as);
        assert(t);

        if (cs->serviceIdleTimeout &&
            difftime(time(0), t)>cs->serviceIdleTimeout) {
          if (!LC_CardServer_StopService(cs, as)) {
            DBG_INFO(0, "Could not stop service \"%s\"",
                     LC_Service_GetServiceName(as));
            return -1;
          }
          done++;
        } /* if timeout */
        else {
          /* otherwise service is not idle */
          return 1;
        }
      }
    } /* if not a client-service */
  }

  return done?0:1;
}



int LC_CardServer_SendServiceNotification(LC_CARDSERVER *cs,
                                          LC_CLIENT *cl,
                                          const char *ncode,
                                          const LC_SERVICE *sv,
                                          const char *info){
  GWEN_DB_NODE *dbData;
  const char *s;
  char numbuf[16];
  int rv;

  assert(cs);
  assert(ncode);
  assert(sv);
  dbData=GWEN_DB_Group_new("serviceData");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Service_GetServiceId(sv));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);

  s=LC_Service_GetServiceName(sv);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceName", s);
  if (info)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", info);
  rv=LC_CardServer_SendNotification(cs, cl,
                                    LC_NOTIFY_TYPE_SERVICE,
                                    ncode, dbData);
  GWEN_DB_Group_free(dbData);
  if (rv) {
    DBG_INFO(0, "here");
    return rv;
  }
  return 0;
}





