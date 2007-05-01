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


#include "clientmanager_p.h"
#include "connection_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>



GWEN_TYPE_UINT32 LCCL_ClientManager_GetNotificationMask(const char *ntype,
                                                        const char *ncode) {
  GWEN_TYPE_UINT32 res;

  assert(ntype);
  assert(ncode);

  res=0;

  if (strcasecmp(ntype, LC_NOTIFY_TYPE_DRIVER)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_START)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_UP)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_DOWN)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_ERROR)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_ERROR;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_ADD)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_ADD;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_DEL)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_DEL;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_READER)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_START)==0)
      res=LC_NOTIFY_FLAGS_READER_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_UP)==0)
      res=LC_NOTIFY_FLAGS_READER_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DOWN)==0)
      res=LC_NOTIFY_FLAGS_READER_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ERROR)==0)
      res=LC_NOTIFY_FLAGS_READER_ERROR;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ADD)==0)
      res=LC_NOTIFY_FLAGS_READER_ADD;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DEL)==0)
      res=LC_NOTIFY_FLAGS_READER_DEL;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_SERVICE)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_START)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_UP)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_DOWN)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_ERROR)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_ERROR;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_CARD)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_INSERTED)==0)
      res=LC_NOTIFY_FLAGS_CARD_INSERTED;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_REMOVED)==0)
      res=LC_NOTIFY_FLAGS_CARD_REMOVED;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_CLIENT)==0) {
  }

  if (res==0) {
    DBG_ERROR(0, "Could not translate \"%s:%s\" into a mask",
              ntype, ncode);
    return 0;
  }
  return res;
}



int LCCL_ClientManager__SendNotification(LCCL_CLIENTMANAGER *clm,
                                         const LCCL_CLIENT *cl,
                                         const char *ntype,
                                         const char *ncode,
                                         GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rid;

  assert(ntype);
  assert(ncode);
  DBG_INFO(0, "Sending notification to client \"%08x\"",
           LCCL_Client_GetClientId(cl));

  dbReq=GWEN_DB_Group_new("Notification");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCCL_Client_GetClientId(cl));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ntype", ntype);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ncode", ncode);
  if (dbData) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT, "data");
    assert(dbT);
    GWEN_DB_AddGroupChildren(dbT, dbData);
  }

  /* send request (fire and forget) */
  rid=GWEN_IpcManager_SendRequest(clm->ipcManager,
                                  LCCL_Client_GetClientId(cl),
                                  dbReq);
  if (rid==0) {
    DBG_INFO(0, "here");
    return -1;
  }
  GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 1);

  /* done */
  return 0;
}



int LCCL_ClientManager_SendNotification(LCCL_CLIENTMANAGER *clm,
                                        const LCCL_CLIENT *cl,
                                        const char *ntype,
                                        const char *ncode,
                                        GWEN_DB_NODE *dbData){
  int rv;
  GWEN_TYPE_UINT32 mask;
  int err;
  int clients;

  assert(ntype);
  assert(ncode);

  mask=LCCL_ClientManager_GetNotificationMask(ntype, ncode);
  if (!mask) {
    DBG_ERROR(0, "here");
    return -1;
  }

  DBG_DEBUG(0, "Mask for %s:%s is %08x", ntype, ncode, mask);

  err=0;
  clients=0;

  if (cl!=0) {
    /* send notification to a specific client */
    DBG_INFO(0, "Sending notification %s:%s to client \"%08x\"",
             ntype, ncode, LCCL_Client_GetClientId(cl));
    if (LCCL_Client_GetNotifyFlags(cl) & mask) {
      clients++;
      rv=LCCL_ClientManager__SendNotification(clm, cl,
                                              ntype, ncode,
                                              dbData);
      if (rv) {
        DBG_ERROR(0, "Error sending request to client \"%08x\"",
                  LCCL_Client_GetClientId(cl));
        err++;
      }
    }
    else {
      DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s",
                LCCL_Client_GetClientId(cl), ntype, ncode);
    }
  }
  else {
    /* send notification to all clients */
    DBG_INFO(0, "Sending notification %s:%s to all clients",
             ntype, ncode);
    cl=LCCL_Client_List_First(clm->clients);
    while(cl) {
      if (LCCL_Client_GetNotifyFlags(cl) & mask) {
        clients++;
        rv=LCCL_ClientManager__SendNotification(clm, cl,
                                                ntype, ncode,
                                                dbData);
        if (rv) {
          DBG_ERROR(0, "Error sending request to client \"%08x\"",
                    LCCL_Client_GetClientId(cl));
          err++;
        }
      }
      else {
        DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s [%08x]",
                  LCCL_Client_GetClientId(cl), ntype, ncode,
                  LCCL_Client_GetNotifyFlags(cl) );
      }
      cl=LCCL_Client_List_Next(cl);
    }
  }

  if (clients && (clients==err)) {
    DBG_ERROR(0, "Could not send notification to any client");
    return -1;
  }

  /* done */
  return 0;
}



int LCCL_ClientManager_SendDriverNotification(LCCL_CLIENTMANAGER *clm,
                                              const LCCL_CLIENT *cl,
                                              GWEN_TYPE_UINT32 did,
                                              const char *driverType,
                                              const char *driverName,
                                              const char *libraryFile,
                                              LC_DRIVER_STATUS dst,
                                              const char *reason){
  const char *s;

  switch(dst) {
  case LC_DriverStatusDown:
    s=LC_NOTIFY_CODE_DRIVER_DOWN;
    break;

  case LC_DriverStatusStarted:
    s=LC_NOTIFY_CODE_DRIVER_START;
    break;

  case LC_DriverStatusUp:
    s=LC_NOTIFY_CODE_DRIVER_UP;
    break;

  case LC_DriverStatusAborted:
    s=LC_NOTIFY_CODE_DRIVER_ERROR;
    break;

  case LC_DriverStatusDisabled:
    s=LC_NOTIFY_CODE_DRIVER_ERROR;
    break;

  case LC_DriverStatusStopping:
  case LC_DriverStatusWaitForStart:
  default:
    s=0;
  }

  if (s) {
    GWEN_DB_NODE *dbData;
    char numbuf[16];
    int rv;

    dbData=GWEN_DB_Group_new("driverData");

    rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", did);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverId", numbuf);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverType", driverType);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverName", driverName);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "libraryFile", libraryFile);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", reason);

    rv=LCCL_ClientManager_SendNotification(clm, cl,
                                           LC_NOTIFY_TYPE_DRIVER,
                                           s,
                                           dbData);
    GWEN_DB_Group_free(dbData);
    return rv;
  }

  return 0;
}



int LCCL_ClientManager_SendReaderNotification(LCCL_CLIENTMANAGER *clm,
                                              const LCCL_CLIENT *cl,
                                              GWEN_TYPE_UINT32 did,
                                              LCCO_READER *r,
                                              LC_READER_STATUS rst,
                                              const char *reason) {
  const char *s;

  switch(rst) {
  case LC_ReaderStatusDown:
    s=LC_NOTIFY_CODE_READER_DOWN;
    break;

  case LC_ReaderStatusUp:
    s=LC_NOTIFY_CODE_READER_UP;
    break;

  case LC_ReaderStatusWaitForDriver:
  case LC_ReaderStatusWaitForReaderUp:
    s=LC_NOTIFY_CODE_READER_START;
    break;

  case LC_ReaderStatusAborted:
  case LC_ReaderStatusDisabled:
    s=LC_NOTIFY_CODE_READER_ERROR;
    break;

  case LC_ReaderStatusHwAdd:
    s=LC_NOTIFY_CODE_READER_ADD;
    break;
  case LC_ReaderStatusHwDel:
    s=LC_NOTIFY_CODE_READER_DEL;
    break;

  default:
    s=0;
  }

  if (s) {
    GWEN_DB_NODE *dbData;
    char numbuf[16];
    int rv;

    dbData=GWEN_DB_Group_new("readerData");

    rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", did);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverId", numbuf);

    LCCO_Reader_toDb(r, dbData);
    rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
                LCCO_Reader_GetReaderId(r));
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerId", numbuf);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", reason);

    rv=LCCL_ClientManager_SendNotification(clm, cl,
                                           LC_NOTIFY_TYPE_READER,
                                           s,
                                           dbData);
    GWEN_DB_Group_free(dbData);
    return rv;
  }

  return 0;
}



int LCCL_ClientManager_SendServiceNotification(LCCL_CLIENTMANAGER *clm,
                                               const LCCL_CLIENT *cl,
                                               GWEN_TYPE_UINT32 did,
                                               const char *serviceType,
                                               const char *serviceName,
                                               LC_SERVICE_STATUS st,
                                               const char *reason){
  const char *s;

  switch(st) {
  case LC_ServiceStatusDown:
    s=LC_NOTIFY_CODE_SERVICE_DOWN;
    break;

  case LC_ServiceStatusStarted:
    s=LC_NOTIFY_CODE_SERVICE_START;
    break;

  case LC_ServiceStatusUp:
    s=LC_NOTIFY_CODE_SERVICE_UP;
    break;

  case LC_ServiceStatusAborted:
    s=LC_NOTIFY_CODE_SERVICE_ERROR;
    break;

  case LC_ServiceStatusDisabled:
    s=LC_NOTIFY_CODE_SERVICE_ERROR;
    break;

  case LC_ServiceStatusStopping:
  case LC_ServiceStatusWaitForStart:
  default:
    s=0;
  }

  if (s) {
    GWEN_DB_NODE *dbData;
    char numbuf[16];
    int rv;

    dbData=GWEN_DB_Group_new("serviceData");

    rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", did);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceId", numbuf);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceType", serviceType);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceName", serviceName);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", reason);

    rv=LCCL_ClientManager_SendNotification(clm, cl,
                                           LC_NOTIFY_TYPE_SERVICE,
                                           s,
                                           dbData);
    GWEN_DB_Group_free(dbData);
    return rv;
  }

  return 0;
}



int LCCL_ClientManager_SendCardNotification(LCCL_CLIENTMANAGER *clm,
                                            const LCCL_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            int slotNum,
                                            GWEN_TYPE_UINT32 cardNum,
                                            LC_CARD_STATUS status,
                                            const char *reason){
  const char *s;

  switch(status) {
  case LC_CardStatusInserted:
    s=LC_NOTIFY_CODE_CARD_INSERTED;
    break;
  case LC_CardStatusRemoved:
  default:
    s=LC_NOTIFY_CODE_CARD_REMOVED;
    break;
  }

  if (s) {
    GWEN_DB_NODE *dbData;
    char numbuf[16];
    int rv;

    dbData=GWEN_DB_Group_new("cardData");

    rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", rid);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerId", numbuf);
    rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", slotNum);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "slotNum", numbuf);
    rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", cardNum);
    assert(rv>0 && rv<sizeof(numbuf)-1);
    numbuf[sizeof(numbuf)-1]=0;
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "cardNum", numbuf);
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", reason);

    rv=LCCL_ClientManager_SendNotification(clm, cl,
                                           LC_NOTIFY_TYPE_CARD,
                                           s,
                                           dbData);
    GWEN_DB_Group_free(dbData);
    return rv;
  }

  return 0;
}





int LCCL_ClientManager_HandleSetNotify(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 oldFlags;
  GWEN_TYPE_UINT32 flags=0;
  LCDM_DEVICEMANAGER *dm;
  int err=0;
  int i;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  if (clientId==0) {
    DBG_ERROR(0, "No client id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clientId)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Unknown client id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: SetNotify [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));


  /* old commands */
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbReq, "data/flag", i, 0);
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
          LCS_Server_SendErrorResponse(clm->server, rid,
                                       LC_ERROR_INVALID,
                                       "Notification type/code too long");
          err++;
          break;
        }
        strncpy(ntype, s, len);
        ntype[len]=0;
        p++;
        lflags=LCCL_ClientManager_GetNotificationMask(ntype, p);
        DBG_DEBUG(0, "Mask for %s:%s is %08x", ntype, p, lflags);
        if (!lflags) {
          DBG_ERROR(0, "Unknown notification type/code (%s)", s);
          LCS_Server_SendErrorResponse(clm->server, rid,
                                       LC_ERROR_INVALID,
                                       "Unknown notification type/code");
          err++;
          break;
        }
        if (lflags & ~LCCL_Client_GetNotifyMask(cl)) {
          DBG_ERROR(0, "Notification type/code not allowed (%08x/%08x, %s)",
                    lflags, LCCL_Client_GetNotifyMask(cl), s);
          LCS_Server_SendErrorResponse(clm->server, rid,
                                       LC_ERROR_INVALID,
                                       "Notification type/code not allowed");
          err++;
          break;
        }
        else {
          flags|=lflags;
        }
      } /* if ":" found */
      else {
        DBG_ERROR(0, "Bad notification type/code (%s)", s);
        LCS_Server_SendErrorResponse(clm->server, rid,
                                     LC_ERROR_INVALID,
                                     "Bad notification type/code");
        err++;
        break;
      }
    } /* if type/code pair found */
  } /* for */

  if (err) {
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return 0; /* handled */
  }

  oldFlags=LCCL_Client_GetNotifyFlags(cl);
  DBG_DEBUG(0, "Setting new flags: %08x", flags);
  LCCL_Client_SetNotifyFlags(cl, flags);

  /* let the device manager list all drivers and readers.
   * Internally this is done by the device manager calling the appropriate
   * ReaderChg and DriverChg callbacks which in turn send notifications
   * to the clients according to their specific notifications flags/mask.
   * Since we set listingClient the following notifications will only go to
   * this particular client.
   */
  clm->listingClient=cl;
  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);
  LCDM_DeviceManager_ListDrivers(dm);
  LCDM_DeviceManager_ListReaders(dm);
  clm->listingClient=0;

  if (flags & LC_NOTIFY_FLAGS_SINGLESHOT)
    LCCL_Client_SetNotifyFlags(cl, oldFlags);

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("Client_SetNotifyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Notification types/codes ok");

  /* send response */
  if (GWEN_IpcManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to Client_SetNotify");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* remove request since we handled it */
  if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0; /* handled */
}









