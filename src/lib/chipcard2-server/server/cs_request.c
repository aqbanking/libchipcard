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


LC_REQUEST *LC_CardServer_GetRequest(const LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid) {
  LC_REQUEST *rq;

  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rid)
      break;
    rq=LC_Request_List_Next(rq);
  } /* while */

  return rq;
}



LC_REQUEST *LC_CardServer_FindClientCardRequest(const LC_CARDSERVER *cs,
                                                const LC_CLIENT *cl,
                                                const LC_CARD *cd,
                                                const char *name) {
  LC_REQUEST *rq;
  GWEN_TYPE_UINT32 cid;

  assert(cs);
  assert(cd);
  cid=LC_Card_GetCardId(cd);
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    if (LC_Request_GetClient(rq)==cl) {
      GWEN_DB_NODE *gr;
      const char *cmdName;

      DBG_DEBUG(0, "Found a request for this client");
      gr=LC_Request_GetInRequestData(rq);
      assert(gr);
      cmdName=GWEN_DB_GetCharValue(gr, "command/vars/cmd", 0, 0);
      assert(cmdName);
      if (strcasecmp(cmdName, name)==0) {
        GWEN_TYPE_UINT32 ccid;

        if (1==sscanf(GWEN_DB_GetCharValue(gr, "body/cardid", 0, "0"),
		      "%x", &ccid)) {
	  DBG_VERBOUS(0, "Comparing card id \"%08x\" against \"%08x\"",
                      ccid, cid);
	  if (ccid==cid)
	    return rq;
	}
      }
    } /* if client matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



int LC_CardServer_CheckRequest(LC_CARDSERVER *cs, LC_REQUEST *rq) {
  const char *name;
  LC_CARD *card;
  LC_READER *r;
  LC_DRIVER *d;

  card=LC_Request_GetCard(rq);
  assert(card);
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);

  name=GWEN_DB_GetCharValue(LC_Request_GetInRequestData(rq),
                            "command/vars/cmd", 0, 0);
  assert(name);
  DBG_DEBUG(0, "Request: %s", name);

  if (strcasecmp(name, "CommandCard")==0) {
    GWEN_DB_NODE *dbDriverResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_NOTICE(0, "Handling CommandCard request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      return -1;
    }
    DBG_DEBUG(0, "Checking for driver response");
    dbDriverResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                     ridOut);
    if (dbDriverResponse) {
      GWEN_DB_NODE *dbClientResponse;
      const char *p;
      const void *bp;
      unsigned int bs;
      const char *code;

      LC_Driver_DecPendingCommandCount(d);
      DBG_DEBUG(0, "Sending response to CommandCard");
      dbClientResponse=GWEN_DB_Group_new("CommandCardResponse");
      code=GWEN_DB_GetCharValue(dbDriverResponse,
				"body/code", 0, "ERROR");
      assert(code);
      GWEN_DB_SetCharValue(dbClientResponse,
			   GWEN_DB_FLAGS_OVERWRITE_VARS,
			   "code", code);
      p=GWEN_DB_GetCharValue(dbDriverResponse,
			     "body/text", 0, 0);
      if (p)
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", p);

      bp=GWEN_DB_GetBinValue(dbDriverResponse,
                             "body/data", 0, 0, 0, &bs);
      if (bp && bs)
        GWEN_DB_SetBinValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "data", bp, bs);

      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send CommandCard response");
        GWEN_DB_Group_free(dbDriverResponse);
        return -1;
      }
      GWEN_DB_Group_free(dbDriverResponse);
      return 0;
    }
    else {
      DBG_DEBUG(0, "No response yet");
    }
  } /* CommandCard */
  else if (strcasecmp(name, "TakeCard")==0) {
    DBG_DEBUG(0, "TakeCard");
  }
  else if (strcasecmp(name, "ExecCommand")==0) {
    GWEN_DB_NODE *dbDriverResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ExecCommand request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      return -1;
    }
    DBG_DEBUG(0, "Checking for driver response (%08x)", ridOut);
    dbDriverResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                     ridOut);
    if (dbDriverResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbRsp;
      LC_CARDMGR_RESULT res;

      LC_Driver_DecPendingCommandCount(d);
      DBG_DEBUG(0, "Sending response to ExecCommand");
      dbClientResponse=GWEN_DB_Group_new("ExecCommandResponse");
      dbRsp=GWEN_DB_Group_new("command");
      res=LC_CardMgr_CheckResponse(cs->cardManager,
                                   rq,
                                   dbDriverResponse,
                                   dbRsp);
      if (res==LC_CardMgr_ResultOk) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_NONE);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Command executed");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             GWEN_DB_GroupName(dbRsp));
        GWEN_DB_GroupRename(dbRsp, "command");
        GWEN_DB_AddGroup(dbClientResponse, dbRsp);
        if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                         LC_Request_GetInRequestId(rq),
                                         dbClientResponse)) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          GWEN_DB_Group_free(dbDriverResponse);
          return -1;
        }
        GWEN_DB_Group_free(dbDriverResponse);
        return 0;
      } /* if ok */
      else if (res==LC_CardMgr_ResultNeedMore) {
        GWEN_TYPE_UINT32 rqid;

        /* TODO */
        GWEN_DB_Group_free(dbRsp);
        rqid=LC_Request_GetInRequestId(rq);
        DBG_ERROR(0, "NeedMore not yet supported");
        if (LC_CardServer_SendErrorResponse
            (cs, rqid,
             LC_ERROR_NOT_SUPPORTED,
             "NeedMore not yet supported")
           ) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          GWEN_DB_Group_free(dbDriverResponse);
          return -1;
        }
        GWEN_DB_Group_free(dbDriverResponse);
        return 0;
      } /* if need more */
      else if (res==LC_CardMgr_ResultError ||
               res==LC_CardMgr_ResultCmdError) {
        int code;
        const char *text;
        GWEN_TYPE_UINT32 rqid;

        rqid=LC_Request_GetInRequestId(rq);
        code=GWEN_DB_GetIntValue(dbRsp, "code", 0, 0);
        text=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
        DBG_NOTICE(0, "Error in command (%d: %s)", code, text);
        if (LC_CardServer_SendErrorResponse(cs, rqid, code, text)) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          GWEN_DB_Group_free(dbRsp);
          GWEN_DB_Group_free(dbDriverResponse);
          return -1;
        }
        GWEN_DB_Group_free(dbRsp);
        GWEN_DB_Group_free(dbDriverResponse);
        return 0;
      } /* if error */
    } /* if driverResponse */
  } /* ExecCommand */
  else if (strcasecmp(name, "ServiceOpen")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceOpen request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      char numbuf[16];
      const char *rspName;
      GWEN_DB_NODE *dbServiceRspBody;
      GWEN_DB_NODE *dbRsp;

      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
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
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceOpen");
      dbClientResponse=GWEN_DB_Group_new("ServiceOpenResponse");

      dbServiceRspBody=GWEN_DB_GetGroup(dbServiceResponse,
                                        GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                        "body");
      assert(dbServiceRspBody);
      rspName=GWEN_DB_GetCharValue(dbServiceResponse,
                                   "command/vars/cmd",0, 0);
      assert(rspName);
    
      /* check for error */
      if (strcasecmp(rspName, "error")==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Error flagged by service.");

        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
      }
      else if (strcasecmp(rspName, "ServiceOpenResponse")==0) {
        GWEN_TYPE_UINT32 contextId;

        /* get context id */
        if (1!=sscanf(GWEN_DB_GetCharValue(dbServiceRspBody,
                                           "contextId", 0, "0"),
                      "%x", &contextId)) {
          DBG_ERROR(0, "No/bad context id from service");
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_INVALID);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "No context id received.");
          /* remove service from client */
          LC_Client_DelService(LC_Request_GetClient(rq),
                               LC_Service_GetServiceId(sv));
          LC_Service_DecActiveClientsCount(sv);
        }
        else {
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_NONE);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "Service open.");
          /* send context id */
          snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
          numbuf[sizeof(numbuf)-1]=0;
          GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "contextId", numbuf);
        }
      }
      else {
        DBG_ERROR(0, "Unknown service response to ServiceOpen: %s",
                  rspName);
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Invalid service response.");
        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");
      dbRsp=GWEN_DB_GetGroup(dbClientResponse,
                             GWEN_DB_FLAGS_DEFAULT,
                             "service");
      GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ServiceCommand response");
        GWEN_DB_Group_free(dbServiceResponse);
        return -1;
      }
      GWEN_DB_Group_free(dbServiceResponse);
      return 0;
    } /* if serviceResponse */
  } /* ServiceOpen */
  else if (strcasecmp(name, "ServiceClose")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceClose request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      char numbuf[16];
      const char *rspName;
      GWEN_DB_NODE *dbServiceRspBody;
      GWEN_DB_NODE *dbRsp;

      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
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
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceClose");
      dbClientResponse=GWEN_DB_Group_new("ServiceCloseResponse");

      dbServiceRspBody=GWEN_DB_GetGroup(dbServiceResponse,
                                        GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                        "body");
      assert(dbServiceRspBody);
      rspName=GWEN_DB_GetCharValue(dbServiceResponse,
                                   "command/vars/cmd",0, 0);
      assert(rspName);
    
      /* check for error */
      if (strcasecmp(rspName, "error")==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Error flagged by service.");
      }
      else if (strcasecmp(rspName, "ServiceCloseResponse")==0) {
        GWEN_TYPE_UINT32 contextId;

        /* get context id */
        if (1!=sscanf(GWEN_DB_GetCharValue(dbServiceRspBody,
                                           "contextId", 0, "0"),
                      "%x", &contextId)) {
          DBG_ERROR(0, "No/bad context id from service");
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_INVALID);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "No context id received.");
        }
        else {
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_NONE);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "Service closed.");
          /* send context id */
          snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
          numbuf[sizeof(numbuf)-1]=0;
          GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "contextId", numbuf);
        }
      }
      else {
        DBG_ERROR(0, "Unknown service response to ServiceClose: %s",
                  rspName);
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Invalid service response.");
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");
      dbRsp=GWEN_DB_GetGroup(dbClientResponse,
                             GWEN_DB_FLAGS_DEFAULT,
                             "service");
      GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ServiceCommand response");
        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
        GWEN_DB_Group_free(dbServiceResponse);
        return -1;
      }

      /* remove service from client */
      LC_Client_DelService(LC_Request_GetClient(rq),
                           LC_Service_GetServiceId(sv));
      LC_Service_DecActiveClientsCount(sv);
      GWEN_DB_Group_free(dbServiceResponse);
      return 0;
    } /* if serviceResponse */
  } /* ServiceClose */
  else if (strcasecmp(name, "ServiceCommand")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceCommand request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbRsp;
      int res;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      GWEN_TYPE_UINT32 contextId;
      char numbuf[16];
  
      DBG_DEBUG(0, "Handling ServiceCommand request");
      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
      }

      /* get context id */
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/contextId", 0, "0"),
                    "%x", &contextId)) {
        DBG_ERROR(0, "Uups, no/bad context id, should not happen here...");
        abort();
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
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceCommand");
      dbClientResponse=GWEN_DB_Group_new("ServiceCommandResponse");

      dbRsp=GWEN_DB_Group_new("command");
      res=LC_CardServer_CheckServiceRsp(cs,
                                        rq,
                                        dbServiceResponse,
                                        dbRsp);
      if (res==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_NONE);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Service command executed");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             GWEN_DB_GroupName(dbRsp));
        GWEN_DB_GroupRename(dbRsp, "command");
        GWEN_DB_AddGroup(dbClientResponse, dbRsp);
      }
      else {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "bad service response");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             "error");
        GWEN_DB_Group_free(dbRsp);
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send context id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "contextId", numbuf);
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ExecCommand response");
        GWEN_DB_Group_free(dbServiceResponse);
        return -1;
      }
      GWEN_DB_Group_free(dbServiceResponse);
      return 0;
    } /* if serviceResponse */
  } /* ServiceCommand */
  else {
    DBG_ERROR(0, "Unknown client request \"%s\"", name);
    return -1;
  }

  return 1;
}


void LC_CardServer_RemoveRequest(LC_CARDSERVER *cs, LC_REQUEST *rq){
  GWEN_TYPE_UINT32 rid;

  DBG_DEBUG(0, "Removing request");
  LC_Request_List_Del(rq);
  rid=LC_Request_GetOutRequestId(rq);
  if (rid) {
    DBG_DEBUG(0, "- outRequest is %08x", rid);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
  }
  rid=LC_Request_GetInRequestId(rq);
  if (rid) {
    DBG_DEBUG(0, "- inRequest is %08x", rid);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  }
  LC_Request_free(rq);
}



int LC_CardServer_CheckRequests(LC_CARDSERVER *cs){
  LC_REQUEST *rq;
  int needIO;

  assert(cs);
  needIO=0;
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    int rv;
    LC_REQUEST *next;

    DBG_INFO(0, "Checking request %s",
             GWEN_DB_GetCharValue(LC_Request_GetInRequestData(rq),
                                  "command/vars/cmd", 0, 0));
    next=LC_Request_List_Next(rq);
    rv=LC_CardServer_CheckRequest(cs, rq);
    if (rv!=1) {
      DBG_DEBUG(0, "Removing request");
      LC_CardServer_RemoveRequest(cs, rq);
      needIO++;
    }
    rq=next;
  } /* while */

  return needIO?0:1;
}



int LC_CardServer_HandleStartWait(LC_CARDSERVER *cs,
				  GWEN_TYPE_UINT32 rid,
				  GWEN_DB_NODE *dbReq){
  LC_READER *r;
  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 mask;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitReqId;
  GWEN_DB_NODE *dbRsp;
  unsigned int readers;

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StartWait [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* allow all cards to be seen */
  LC_Client_DelAllCards(cl);

  flags=LC_CardServer_GetFlags(dbReq, "body/flags");
  mask=LC_CardServer_GetFlags(dbReq, "body/mask");

  readers=0;
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (!((LC_Reader_GetFlags(r) ^ flags) & mask)) {
      /* reader matches flags, does the client already use it ? */
      if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
        /* yes, simply do nothing about it */
        DBG_INFO(0, "Reader \"%s\" is already known to client %08x",
                 LC_Reader_GetReaderName(r), clientId);
      }
      else {
	if (LC_Reader_IsAvailable(r)) {
	  /* add the reader */
	  DBG_INFO(0, "Adding reader \"%s\" for client %08x",
		   LC_Reader_GetReaderName(r), clientId);
          LC_Reader_IncUsageCount(r);
	  LC_Client_AddReader(cl, LC_Reader_GetReaderId(r));
	  readers++;
	  /* is reader down ? */
	  if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown) {
	    DBG_NOTICE(0, "Starting reader \"%s\" on account of client %08x",
		       LC_Reader_GetReaderName(r), clientId);
	    if (LC_CardServer_StartReader(cs, r)) {
	      DBG_ERROR(0, "Could not start reader \"%s\"",
			LC_Reader_GetReaderName(r));
              LC_Reader_DecUsageCount(r);
              LC_Client_DelReader(cl, LC_Reader_GetReaderId(r));
              readers--;
	    }
	  } /* if reader down */
	} /* if reader is available */
      } /* if reader is new to the client */
    } /* if reader matches the given flags */
    r=LC_Reader_List_Next(r);
  } /* while */

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("StartWaitResponse");
  if (readers) {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "Waiting for cards");
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "No matching reader");
  }
  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  if (readers) {
    LC_Client_AddWaitRequestCount(cl);
    LC_Client_AddWaitReaderState(cl, flags, mask);
  }

  /* remove old StartWaitRequest (if any) */
  waitReqId=LC_Client_GetLastWaitRequestId(cl);
  if (waitReqId) {
    /* remove previous wait request, store new one */
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, waitReqId, 0);
    LC_Client_SubWaitRequestCount(cl);
  }
  LC_Client_SetLastWaitRequestId(cl, rid);

  return 0;
}



int LC_CardServer_HandleStopWait(LC_CARDSERVER *cs,
				 GWEN_TYPE_UINT32 rid,
				 GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitReqId;
  GWEN_DB_NODE *dbRsp;

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StopWait [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* remove WaitRequest (if any) */
  waitReqId=LC_Client_GetLastWaitRequestId(cl);
  if (waitReqId) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, waitReqId, 0);
    LC_Client_SetLastWaitRequestId(cl, 0);
  }

  dbRsp=GWEN_DB_Group_new("StopWaitResponse");
  if (LC_Client_GetWaitRequestCount(cl)) {
    LC_Client_SubWaitRequestCount(cl);
    if (LC_Client_GetWaitRequestCount(cl)==0) {
      LC_READER *r;
  
      r=LC_Reader_List_First(cs->readers);
      while(r) {
	LC_READER *next;

	next=LC_Reader_List_Next(r);
	if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
          LC_Client_DelReader(cl,LC_Reader_GetReaderId(r));
          DBG_VERBOUS(0, "Calling LC_Reader_DecUsageCount");
          LC_Reader_DecUsageCount(r);
        }
	r=next;
      } /* while */
    } /* if no wait request left */
  
    /* create response for client */
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "No longer waiting for cards");
  }
  else {
    /* create response for client */
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "Not waiting for cards");
  }
  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");

  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleTakeCard(LC_CARDSERVER *cs,
				 GWEN_TYPE_UINT32 rid,
				 GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;

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

  DBG_NOTICE(0, "Client %08x: TakeCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in free list */
  card=LC_Card_List_First(cs->freeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      break;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    /* search for card in active list */
    card=LC_Card_List_First(cs->activeCards);
    while(card) {
      if (LC_Card_GetCardId(card)==cardId) {
	/* card found */
        break;
      }
      card=LC_Card_List_Next(card);
    } /* while */
  }

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  LC_Card_AddWaitingClient(card, LC_Client_GetClientId(cl));

  rq=LC_Request_new(cl,
		    GWEN_DB_Group_dup(dbReq),
                    0, card);
  LC_Request_SetInRequestId(rq, rid);
  LC_Request_List_Add(rq, cs->requests);
  return 0;
}



int LC_CardServer_HandleReleaseCard(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;

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

  DBG_NOTICE(0, "Client %08x: ReleaseCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in active list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      if (LC_Card_GetClient(card)==cl) {
        GWEN_DB_NODE *dbRsp;
        GWEN_TYPE_UINT32 ridReset;

	LC_Card_SetClient(card, 0);
        LC_Card_SetContext(card, 0);
	LC_Card_List_Del(card);
	LC_Card_List_Add(card, cs->freeCards);

        ridReset=LC_CardServer_SendResetCard(cs, card);
        if (ridReset==0) {
          DBG_ERROR(0, "Could not send card reset request");
        }
        else
          /* we don't expect an answer, delete request */
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridReset, 1);

	/* create response for client */
	dbRsp=GWEN_DB_Group_new("ReleaseCardResponse");
	GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			     "code", "OK");
	GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			     "text", "Card released");

	/* send response */
	if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
	  DBG_ERROR(0, "Could not send response to client");
	  return -1;
	}
	DBG_DEBUG(0, "Response sent.");
	GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
	return 0;
      }
      else {
	DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
		  cardId, LC_Client_GetClientId(cl));
	LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_CARD_NOT_OWNED,
					"Card is not owned by you");
	return -1;
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
  LC_CardServer_SendErrorResponse(cs, rid,
                                  LC_ERROR_INVALID,
				  "Unknown card id");
  return -1;
}



int LC_CardServer_HandleCommandCard(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_READER *r;
  LC_DRIVER *d;
  LC_REQUEST *rq;
  const void *p;
  unsigned int bs;
  const char *target;
  char numbuf[16];
  GWEN_DB_NODE *dbDriverReq;

  DBG_DEBUG(0, "Command card request %08x", rid);
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

  DBG_NOTICE(0, "Client %08x: CommandCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  p=GWEN_DB_GetBinValue(dbReq, "body/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(0, "No data give");
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "body/target", 0, 0);
  if (!target) {
    DBG_ERROR(0, "No target given");
    return -1;
  }

  /* search for card in active list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      if (LC_Card_GetClient(card)==cl) {
        break;
      }
      else {
	DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
		  cardId, LC_Client_GetClientId(cl));
	LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_CARD_NOT_OWNED,
					"Card is not owned by you");
	return -1;
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Card not found");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  /* create outbound command */
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  dbDriverReq=GWEN_DB_Group_new("CardCommand");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LC_Card_GetSlot(card));
  GWEN_DB_SetIntValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "cardnum", LC_Card_GetReadersCardId(card));
  GWEN_DB_SetBinValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", p, bs);
  GWEN_DB_SetCharValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbDriverReq,
                    card);
  LC_Request_SetInRequestId(rq, rid);
  LC_Reader_AddRequest(r, rq);

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleExecCommand(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CARDMGR_RESULT res;
  GWEN_DB_NODE *dbRsp;

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

  DBG_NOTICE(0, "Client %08x: ExecCommand (%s) [%s/%s]",
             clientId,
             GWEN_DB_GetCharValue(dbReq,
                                  "body/command/name", 0, "no command"),
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in active list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      if (LC_Card_GetClient(card)==cl) {
        break;
      }
      else {
	DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
		  cardId, LC_Client_GetClientId(cl));
	LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_CARD_NOT_OWNED,
					"Card is not owned by you");
	return -1;
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Card not found");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  /* create outbound command */
  dbRsp=GWEN_DB_Group_new("ExecCommandResponse");
  res=LC_CardMgr_HandleCommand(cs->cardManager,
                               card,
                               rid,
                               dbReq,
                               dbRsp);
  if (res==LC_CardMgr_ResultOk) {
    DBG_DEBUG(0, "Command enqueued");
    GWEN_DB_Group_free(dbRsp);
  }
  else if (res==LC_CardMgr_ResultImmediateResponse) {
    GWEN_DB_SetIntValue(dbRsp,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_NONE);
    GWEN_DB_SetCharValue(dbRsp,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Command executed");
    if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                     rid,
                                     dbRsp)) {
      DBG_ERROR(0, "Could not send ExecCommand response");
      return -1;
    }
  }
  else {
    if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
      int code;
      const char *text;

      code=GWEN_DB_GetIntValue(dbRsp, "code", 0, LC_ERROR_GENERIC);
      text=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
      DBG_ERROR(0, "Bad result %d (%d: %s)", res, code, text);
      GWEN_DB_Dump(dbRsp, stderr, 2);
      LC_CardServer_SendErrorResponse(cs, rid,
                                      code,
                                      text?text:"Error handling command");
    }
    else {
      DBG_ERROR(0, "Bad result %d (generic error)", res);
      LC_CardServer_SendErrorResponse(cs, rid,
                                      LC_ERROR_GENERIC,
                                      "Error handling command");
    }
    GWEN_DB_Group_free(dbRsp);
    return -1;
  }

  return 0;
}



int LC_CardServer_HandleSelectCard(LC_CARDSERVER *cs,
                                   GWEN_TYPE_UINT32 rid,
                                   GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  const char *cardName;
  GWEN_DB_NODE *dbRsp;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cardName=GWEN_DB_GetCharValue(dbReq, "body/cardName", 0, 0);
  if (!cardName) {
    if (LC_CardServer_SendErrorResponse(cs, rid,
					LC_ERROR_INVALID,
					"Command incomplete")) {
      DBG_ERROR(0, "Could not send response");
    }
    return -1;
  }

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

  DBG_NOTICE(0, "Client %08x: SelectCard (%s) [%s/%s]",
             clientId,
             cardName,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in active list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      if (LC_Card_GetClient(card)==cl)
        break;
      DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
                cardId, LC_Client_GetClientId(cl));
      if (LC_CardServer_SendErrorResponse(cs, rid,
                                          LC_ERROR_CARD_NOT_OWNED,
                                          "Card is not owned by you")){
        DBG_ERROR(0, "Could not send response");
      }
      return -1;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown card id");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  if (LC_CardMgr_SelectCard(cs->cardManager,
                            card,
                            cardName)) {
    if (LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_INVALID,
                                        "Could not select card/app")){
      DBG_ERROR(0, "Could not send response");
    }
    return -1;
  }

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("SelectCardResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Card and application selected");

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleSetNotify(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 flags;
  int i;
  GWEN_BUFFER *ebuf;
  int err;

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: SetNotify [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  flags=0;
  err=0;
  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbReq, "body/flag", i, 0);
    if (!s)
      break;
    else {
      const char *p;

      p=strchr(s, ':');
      if (p) {
        char ntype[32];
        int len;
        GWEN_TYPE_UINT32 lflags;

        len=p-s;
        if (len>=sizeof(ntype)) {
          DBG_ERROR(0, "Notification type too long (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Notification type/code too long (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        strncpy(ntype, s, len);
        ntype[len]=0;
        p++;
        lflags=LC_CardServer_GetNotificationMask(ntype, p);
        if (!lflags) {
          DBG_ERROR(0, "Unknown notification type/code (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Bad Notification type/code (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        if (lflags & ~LC_Client_GetNotifyMask(cl)) {
          DBG_ERROR(0, "Notification type/code not allowed (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Notification type/code not allowed (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        else {
          flags|=lflags;
        }
      } /* if ":" found */
      else {
        DBG_ERROR(0, "Bad notification type/code (%s)", s);
        GWEN_Buffer_AppendString(ebuf,
                                 "Bad Notification type/code (");
        GWEN_Buffer_AppendString(ebuf, s);
        GWEN_Buffer_AppendString(ebuf, ")");
        err++;
        break;
      }
    } /* if type/code pair found */
  } /* for */

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("SetNotifyResponse");
  if (err) {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", GWEN_Buffer_GetStart(ebuf));
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Notification types/codes ok");
  }
  GWEN_Buffer_free(ebuf);

  DBG_NOTICE(0, "Setting notify flags %08x", flags);
  LC_Client_SetNotifyFlags(cl, flags);

  /* send initial notifications to this client */
  if (flags & LC_NOTIFY_FLAGS_CLIENT_MASK) {
    /* TODO */
  }

  if (flags & LC_NOTIFY_FLAGS_SERVICE_MASK) {
    LC_SERVICE *as;

    as=LC_Service_List_First(cs->services);
    while(as) {
      switch(LC_Service_GetStatus(as)) {
      case LC_ServiceStatusStarted:
        if (flags & LC_NOTIFY_FLAGS_SERVICE_START) {
          LC_CardServer_SendServiceNotification(cs, cl,
                                                LC_NOTIFY_CODE_SERVICE_START,
                                                as,
                                                "Service started");
        }
        break;
      case LC_ServiceStatusStopping:
      case LC_ServiceStatusUp:
        if (flags & LC_NOTIFY_FLAGS_SERVICE_UP) {
          LC_CardServer_SendServiceNotification(cs, cl,
                                                LC_NOTIFY_CODE_SERVICE_UP,
                                                as,
                                                "Service is up");
        }
        break;
      case LC_ServiceStatusDown:
        if (flags & LC_NOTIFY_FLAGS_SERVICE_DOWN) {
          LC_CardServer_SendServiceNotification(cs, cl,
                                                LC_NOTIFY_CODE_SERVICE_UP,
                                                as,
                                                "Service is down");
        }
        break;
      case LC_ServiceStatusAborted:
        if (flags & LC_NOTIFY_FLAGS_SERVICE_ERROR) {
          LC_CardServer_SendServiceNotification(cs, cl,
                                                LC_NOTIFY_CODE_SERVICE_UP,
                                                as,
                                                "Service aborted");
        }
        break;
      case LC_ServiceStatusDisabled:
        if (flags & LC_NOTIFY_FLAGS_SERVICE_ERROR) {
          LC_CardServer_SendServiceNotification(cs, cl,
                                                LC_NOTIFY_CODE_SERVICE_UP,
                                                as,
                                                "Service disabled");
        }
        break;
      default:
        break;
      } /* switch */
      as=LC_Service_List_Next(as);
    }
  }

  if (flags & LC_NOTIFY_FLAGS_DRIVER_MASK) {
    LC_DRIVER *d;

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      switch(LC_Driver_GetStatus(d)) {
      case LC_DriverStatusStarted:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_START) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_START,
					       d,
					       "Driver started");
	}
	break;
      case LC_DriverStatusStopping:
      case LC_DriverStatusUp:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_UP) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_UP,
					       d,
					       "Driver is up");
	}
	break;
      case LC_DriverStatusDown:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_DOWN) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_DOWN,
					       d,
					       "Driver is down");
	}
	break;
      case LC_DriverStatusAborted:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_ERROR,
					       d,
					       "Driver aborted");
	}
	break;
      case LC_DriverStatusDisabled:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_ERROR,
					       d,
					       "Driver disabled");
	}
	break;
      default:
        break;
      } /* switch */
      d=LC_Driver_List_Next(d);
    }
  } /* if driver mask */

  if (flags & LC_NOTIFY_FLAGS_READER_MASK) {
    LC_READER *r;

    r=LC_Reader_List_First(cs->readers);
    while(r) {
      switch(LC_Reader_GetStatus(r)) {
      case LC_ReaderStatusWaitForDriver:
      case LC_ReaderStatusWaitForReaderUp:
	if (flags & LC_NOTIFY_FLAGS_READER_START) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_START,
					       r,
					       "Reader started");
	}
	break;
      case LC_ReaderStatusWaitForReaderDown:
      case LC_ReaderStatusUp:
	if (flags & LC_NOTIFY_FLAGS_READER_UP) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_UP,
					       r,
					       "Reader is up");
	}
	break;
      case LC_ReaderStatusDown:
	if (flags & LC_NOTIFY_FLAGS_READER_DOWN) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_DOWN,
					       r,
					       "Reader is down");
	}
	break;
      case LC_ReaderStatusAborted:
	if (flags & LC_NOTIFY_FLAGS_READER_ERROR) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_ERROR,
					       r,
					       "Reader aborted");
	}
	break;
      case LC_ReaderStatusDisabled:
	if (flags & LC_NOTIFY_FLAGS_READER_ERROR) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_ERROR,
					       r,
					       "Reader disabled");
	}
	break;
      default:
        break;
      } /* switch */
      r=LC_Reader_List_Next(r);
    }
  }

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  DBG_NOTICE(0, "Response sent.");

  return 0;
}



int LC_CardServer_HandleCardCheck(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;

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

  DBG_NOTICE(0, "Client %08x: CardCheck [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad client message");
    return -1;
  }

  /* search for card in free list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      break;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    /* search for card in active list */
    card=LC_Card_List_First(cs->freeCards);
    while(card) {
      if (LC_Card_GetCardId(card)==cardId) {
	/* card found */
        break;
      }
      card=LC_Card_List_Next(card);
    } /* while */
  }

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Card not found");
    return -1;
  }

  if (LC_Card_GetClient(card)==cl) {
    DBG_ERROR(0, "Card \"%08x\" not owned by this client", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_NOT_OWNED,
                                    "Card not owned");
    return -1;
  }

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("CardCheckResponse");
  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "REMOVED");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Card has been removed");
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Card is ok");
  }
  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleCardReset(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 ridReset;

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

  DBG_NOTICE(0, "Client %08x: CardReset [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad client message");
    return -1;
  }

  /* search for card in free list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      break;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    /* search for card in active list */
    card=LC_Card_List_First(cs->freeCards);
    while(card) {
      if (LC_Card_GetCardId(card)==cardId) {
	/* card found */
        break;
      }
      card=LC_Card_List_Next(card);
    } /* while */
  }

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Card not found");
    return -1;
  }

  if (LC_Card_GetClient(card)==cl) {
    DBG_ERROR(0, "Card \"%08x\" not owned by this client", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_NOT_OWNED,
                                    "Card not owned");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card \"%08x\" removed", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card removed");
    return -1;
  }

  /* reset card */
  ridReset=LC_CardServer_SendResetCard(cs, card);
  if (ridReset==0) {
    DBG_ERROR(0, "Could not send card reset request");
  }
  else
    /* we don't expect an answer, delete request */
    /* TODO: Enqueue this request and return success only if RESET really
     * succeeded */
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridReset, 1);

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("CardCheckResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Card is ok");

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}





