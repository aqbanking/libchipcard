/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: clientmanager.c 267 2006-09-19 18:36:57Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "slavemanager_p.h"
#include "server_l.h"
#include "sl_request_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>

#include <gwenhywfar/directory.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/net2.h>
#include <gwenhywfar/nl_http.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_socket.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>


LCSL_SLAVEMANAGER *LCSL_SlaveManager_new(LCS_SERVER *server) {
  LCSL_SLAVEMANAGER *slm;

  assert(server);
  GWEN_NEW_OBJECT(LCSL_SLAVEMANAGER, slm);
  slm->server=server;
  slm->ipcManager=LCS_Server_GetIpcManager(server);
  slm->slaveReaders=LCCO_Reader_List2_new();

  return slm;
}



void LCSL_SlaveManager_free(LCSL_SLAVEMANAGER *slm) {
  if (slm) {
    LCCO_Reader_List2_freeAll(slm->slaveReaders);
    GWEN_FREE_OBJECT(slm);
  }
}



int LCSL_SlaveManager_Init(LCSL_SLAVEMANAGER *slm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbSlave;

  DBG_INFO(0, "Initialising slave manager");
  assert(slm);

  slm->commandTimeout=LCSL_SLAVEMANAGER_DEF_COMMAND_TIMEOUT;

  dbSlave=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                           "SlaveManager");
  if (dbSlave) {
    GWEN_DB_NODE *dbT;

    /* read some values */
#define LCSL_SLM_INIT_VAL(s) \
  slm->s=GWEN_DB_GetIntValue(dbSlave, __STRING(s), 0, slm->s);
    LCSL_SLM_INIT_VAL(commandTimeout)
#undef LCSL_SLM_INIT_VAL
    /* connect to master */
    dbT=GWEN_DB_GetGroup(dbSlave, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                         "master");
    if (dbT) {
      int rv;

      rv=LCSL_SlaveManager__PrepareConnection(slm, dbT);
      if (rv) {
        DBG_INFO(0, "here (%d)", rv);
        return rv;
      }
      rv=LCSL_SlaveManager__Connect(slm);
      if (rv==LC_ERROR_IPC)
        return LCS_INITRESULT_RESTART;
      if (rv) {
        DBG_INFO(0, "here (%d)", rv);
        return rv;
      }
    }
    else {
      DBG_ERROR(0, "No master given in config file");
      return -1;
    }
  }
  else {
    DBG_ERROR(0, "No SlaveManager group in configuration");
    return -1;
  }

  return 0;
}




int LCSL_SlaveManager__PrepareConnection(LCSL_SLAVEMANAGER *slm,
                                         GWEN_DB_NODE *gr) {
  const char *typ;
  GWEN_NETLAYER *nl;
  GWEN_NETLAYER *nlBase;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  GWEN_TYPE_UINT32 sid;
  const char *address;
  int port;
  GWEN_DB_NODE *dbHeader;
  GWEN_URL *url;

  assert(slm);
  assert(gr);

  typ=GWEN_DB_GetCharValue(gr, "typ", 0, "local");
  address=GWEN_DB_GetCharValue(gr,
                               "addr", 0, 0);
  if (!address) {
    DBG_ERROR(0, "No address given");
    return -1;
  }
  url=GWEN_Url_fromString(address);
  if (!url) {
    DBG_ERROR(0, "Bad url: %s", address);
    return -1;
  }
  port=GWEN_DB_GetIntValue(gr,
                           "port", 0,
                           LC_DEFAULT_PORT);

  if (strcasecmp(typ, "local")==0) {
    char *taddr, *p;

    /* HTTP over UDS */
    taddr=strdup(address);
    p=strrchr(taddr, '/');
    if (p)
      *p=0;
    if (GWEN_Directory_GetPath(taddr, GWEN_PATH_FLAGS_CHECKROOT)) {
      DBG_ERROR(0, "Could not create path \"%s\"", taddr);
      free(taddr);
      GWEN_Url_free(url);
      return -1;
    }
    free(taddr);
    sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
    GWEN_InetAddr_SetAddress(addr, address);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);
  }
  else if (strcasecmp(typ, "public")==0) {
    /* HTTP over TCP */
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);
  }
  else {
    const char *certDir;
    const char *newCertDir;
    const char *ownCertFile;
    const char *ciphers;
    GWEN_BUFFER *cfbuf=0;
    GWEN_BUFFER *cdbuf=0;
    GWEN_BUFFER *ncdbuf=0;
    GWEN_BUFFER *dhbuf=0;

    /* get cert dir */
    certDir=GWEN_DB_GetCharValue(gr, "certdir", 0, DEF_SERVER_TRUSTEDCERTDIR);
    if (!certDir) {
      GWEN_STRINGLIST *sl;

      sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                   LCS_PATH_SERVER_NEWCERTDIR);
      assert(sl);

      cdbuf=GWEN_Buffer_new(0, 256, 0, 1);
      GWEN_Buffer_AppendString(cdbuf,
                               GWEN_StringList_FirstString(sl));
      certDir=GWEN_Buffer_GetStart(cdbuf);
      GWEN_StringList_free(sl);
    }

    /* get new cert dir */
    newCertDir=GWEN_DB_GetCharValue(gr, "newcertdir", 0,
                                    DEF_SERVER_NEWCERTDIR);
    if (!newCertDir) {
      GWEN_STRINGLIST *sl;

      sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                   LCS_PATH_SERVER_NEWCERTDIR);
      assert(sl);
      ncdbuf=GWEN_Buffer_new(0, 256, 0, 1);
      GWEN_Buffer_AppendString(ncdbuf,
                               GWEN_StringList_FirstString(sl));
      newCertDir=GWEN_Buffer_GetStart(ncdbuf);
      GWEN_StringList_free(sl);
    }

    ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
    ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0, 0);
    if (ownCertFile) {
      if (*ownCertFile!='/' && *ownCertFile!='\\') {
        GWEN_STRINGLIST *sl;
        int rv;

        sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                     LCS_PATH_SERVER_DATADIR);
        assert(sl);

        cfbuf=GWEN_Buffer_new(0, 256, 0, 1);
        rv=GWEN_Directory_FindPathForFile(sl, ownCertFile, cfbuf);
        GWEN_StringList_free(sl);
        if (rv) {
          DBG_ERROR(0, "Cert file \"%s\" not found", ownCertFile);
          GWEN_Buffer_free(cfbuf);
          GWEN_Buffer_free(cdbuf);
          GWEN_Buffer_free(ncdbuf);
          GWEN_Buffer_free(dhbuf);
          GWEN_Url_free(url);
          return rv;
        }
        ownCertFile=GWEN_Buffer_GetStart(cfbuf);
      }
    }
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);

    if (strcasecmp(typ, "private")==0) {
      /* HTTP over SSL */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              0, /* dhFile */
                              0);
      GWEN_NetLayerSsl_SetAskAddCertFn(nl, LCS_Server_AskAddCert,
                                       (void*)slm->server);
      GWEN_NetLayer_free(nlBase);
      nlBase=nl;
    }
    else if (strcasecmp(typ, "secure")==0) {
      /* HTTP over SSL with certificates */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              0, /*dhFile */
                              1);
      GWEN_NetLayerSsl_SetAskAddCertFn(nl, LCS_Server_AskAddCert,
                                       (void*)slm->server);
      GWEN_NetLayer_free(nlBase);
      nlBase=nl;
    }
    else {
      DBG_ERROR(0, "Unknown mode \"%s\"", typ);
      GWEN_InetAddr_free(addr);
      GWEN_Buffer_free(cfbuf);
      GWEN_Buffer_free(cdbuf);
      GWEN_Buffer_free(ncdbuf);
      GWEN_Buffer_free(dhbuf);
      GWEN_Url_free(url);
      return -1;
    }
    GWEN_Buffer_free(cfbuf);
    GWEN_Buffer_free(cdbuf);
    GWEN_Buffer_free(ncdbuf);
    GWEN_Buffer_free(dhbuf);

    if (ciphers)
      GWEN_NetLayerSsl_SetCiphers(nl, ciphers);
  }
  GWEN_InetAddr_free(addr);

  /* create HTTP layer, initialize it */
  nl=GWEN_NetLayerHttp_new(nlBase);
  GWEN_NetLayer_free(nlBase);

  dbHeader=GWEN_NetLayerHttp_GetOutHeader(nl);

  GWEN_NetLayerHttp_SetOutCommand(nl, "POST", url);

  sid=GWEN_IpcManager_AddClient(slm->ipcManager,
                                nl,
                                LCS_MARK_SLAVE);
  if (sid==0) {
    DBG_ERROR(0, "Could not add client");
    GWEN_DB_Dump(gr, stderr, 2);
    GWEN_NetLayer_free(nl);
    GWEN_Url_free(url);
    return -1;
  }

  GWEN_Url_free(url);

  slm->ipcId=sid;
  LCS_Server_UseConnectionFor(slm->server,
                              nl,
                              LCS_Connection_Type_Master,
                              sid);
  return 0;
}



int LCSL_SlaveManager__Work(LCSL_SLAVEMANAGER *slm, int timeout){
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
    DBG_DEBUG(0, "SlaveManager: Working");
    /* activity detected, work with it */
    rv=GWEN_IpcManager_Work(slm->ipcManager);
    if (rv==-1) {
      DBG_ERROR(0, "Error while working with IPC");
      return -1;
    }
    else if (rv==1)
      break;
  }

  return 0;
}



int LCSL_SlaveManager__Connect(LCSL_SLAVEMANAGER *slm) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  time_t startt;
  GWEN_TYPE_UINT32 rid;
  int rv;

  startt=time(0);

  dbReq=GWEN_DB_Group_new("Driver_Ready");

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverType", "slave");

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverName", "slave");

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", 0);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Slave started and ready");

  rid=GWEN_IpcManager_SendRequest(slm->ipcManager,
                                  slm->ipcId,
                                  dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  /* this sends the message and hopefully receives an answer */
  DBG_INFO(0, "Sending Ready Report");
  dbRsp=0;
  while (1) {
    dbRsp=GWEN_IpcManager_GetResponseData(slm->ipcManager, rid);
    if (dbRsp) {
      DBG_DEBUG(0, "Command answered");
      break;
    }
    DBG_VERBOUS(0, "Working...");
    if (LCSL_SlaveManager__Work(slm, 1000)) {
      DBG_ERROR(0, "Error at work");
      GWEN_IpcManager_RemoveRequest(slm->ipcManager, slm->ipcId, 1);
      return -1;
    }

    if (difftime(time(0), startt)>=LCSL_SLAVE_STARTTIMEOUT) {
      DBG_ERROR(0, "Timeout");
      GWEN_IpcManager_RemoveRequest(slm->ipcManager, slm->ipcId, 1);
      return LC_ERROR_IPC;
    }
  } /* while */

  DBG_DEBUG(0, "Answer received");
  rv=LCS_Server_CheckIpcResponse(dbRsp);
  if (rv) {
    DBG_ERROR(0, "Error returned by master, aborting (%d)", rv);
    GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 1);
    return rv;
  }
  GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 1);

  DBG_NOTICE(0, "Connected to master");
  return 0;
}



int LCSL_SlaveManager__Disconnect(LCSL_SLAVEMANAGER *slm) {
  if (GWEN_IpcManager_Disconnect(slm->ipcManager, slm->ipcId)) {
    DBG_ERROR(0, "Error while disconnecting");
  }
  return 0;
}




int LCSL_SlaveManager_Fini(LCSL_SLAVEMANAGER *slm, GWEN_DB_NODE *db) {
  return 0;
}



int LCSL_SlaveManager_HandleRequest(LCSL_SLAVEMANAGER *slm,
                                     GWEN_TYPE_UINT32 rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq) {
  int rv;

  assert(slm);
  assert(name);

  rv=-1;
  if (strcasecmp(name, "Driver_StartReader")==0) {
    rv=LCSL_SlaveManager_HandleStartReader(slm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Driver_StopReader")==0) {
    rv=LCSL_SlaveManager_HandleStopReader(slm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardCommand")==0) {
    rv=LCSL_SlaveManager_HandleCardCommand(slm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Driver_ResetCard")==0) {
    rv=LCSL_SlaveManager_HandleCardReset(slm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Driver_StopDriver")==0) {
    DBG_INFO(0, "Request \"%s\" ignored by slave manager", name);
  }
  else if (strcasecmp(name, "Driver_SuspendCheck")==0) {
    rv=LCSL_SlaveManager_HandleSuspendCheck(slm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Driver_ResumeCheck")==0) {
    rv=LCSL_SlaveManager_HandleResumeCheck(slm, rid, name, dbReq);
  }
  /* Insert more handlers here */
  else {
    DBG_DEBUG(0, "Command \"%s\" not handled by slave manager", name);
    rv=1; /* not handled */
  }

  return rv;
}



void LCSL_SlaveManager_DumpState(const LCSL_SLAVEMANAGER *slm) {
  if (!slm) {
    fprintf(stderr, "No slave manager.\n");
    return;
  }
  else {
    fprintf(stderr, "SlaveManager\n");
    fprintf(stderr, "=====================================\n");
  }
}



void LCSL_SlaveManager_ReaderChg(LCSL_SLAVEMANAGER *slm,
                                 GWEN_TYPE_UINT32 did,
                                 LCCO_READER *r,
                                 LC_READER_STATUS newSt,
                                 const char *reason) {
  assert(slm);
  if (newSt==LC_ReaderStatusHwAdd) {
    /* ignored here */
  }
  else if (newSt==LC_ReaderStatusHwDel) {
    /* also ignored here */
  }
}



void LCSL_SlaveManager_NewReader(LCSL_SLAVEMANAGER *slm,
                                 LCCO_READER *r) {
  assert(slm);
  LCSL_Reader_Extend(r);
  LCCO_Reader_Attach(r);
  LCSL_Reader_SetSlaveReaderId(r, ++(slm->lastReaderId));
  LCCO_Reader_List2_PushBack(slm->slaveReaders, r);
  DBG_NOTICE(0, "Added reader \"%s\" to slave pool",
             LCCO_Reader_GetReaderName(r));
}



int LCSL_SlaveManager_Work(LCSL_SLAVEMANAGER *slm) {
  LCCO_READER_LIST2_ITERATOR *it;
  int done=0;

  if (slm->disconnected) {
    DBG_NOTICE(0, "Slave disconnected, going down");
    return LCS_WORKRESULT_RESTART;
  }

  it=LCCO_Reader_List2_First(slm->slaveReaders);
  if (it) {
    LCCO_READER *sr;

    sr=LCCO_Reader_List2Iterator_Data(it);
    assert(sr);
    while(sr) {
      GWEN_TYPE_UINT32 flags;

      flags=LCSL_Reader_GetFlags(sr);
      if (LCCO_Reader_IsAvailable(sr)) {
        /* reader is available, does the master know about it? */
        if (!(flags & LCSL_READER_FLAGS_REPORTED_UP)) {
          /* no, inform him */
          DBG_INFO(0, "Reader \"%s\" has become available, informing master",
                   LCCO_Reader_GetReaderName(sr));
          if (LCSL_SlaveManager_SendReaderAdd(slm, sr)==0) {
            flags|=LCSL_READER_FLAGS_REPORTED_UP;
            flags&=~LCSL_READER_FLAGS_REPORTED_DOWN;
            flags&=~LCSL_READER_FLAGS_STARTED;
            flags&=~LCSL_READER_FLAGS_STOPPED;
            LCSL_Reader_SetFlags(sr, flags);
            done++;
          }
        }
      }
      else {
        /* reader is not available, does the master know about it?
         * And does the master know the reader at all? */
        if ((flags & LCSL_READER_FLAGS_REPORTED_UP) &&
            !(flags & LCSL_READER_FLAGS_REPORTED_DOWN)) {
          /* The master has previously been informed about this reader, so he
           * surely wants to know when the reader is down. So we inform him. */
          DBG_INFO(0, "Reader \"%s\" has become unavailable, informing master",
                   LCCO_Reader_GetReaderName(sr));
          if (LCSL_SlaveManager_SendReaderDel(slm, sr)==0) {
            flags|=LCSL_READER_FLAGS_REPORTED_DOWN;
            flags&=~LCSL_READER_FLAGS_REPORTED_UP;
            LCSL_Reader_SetFlags(sr, flags);
            done++;
          }
        }
      }
  
      if (LCCO_Reader_IsAvailable(sr) &&
          (flags & LCSL_READER_FLAGS_STARTED)) {
        LCCO_CARD *card;
  
        /* reader has been started by the master, so it is safe now to
         * report any inserted cards */
        while( (card=LCSL_Reader_GetNextInsertedCard(sr)) ) {
          DBG_INFO(0, "Got card \"%08x\" for reader \"%s\" (%d)",
                   LCCO_Card_GetCardId(card),
                   LCCO_Reader_GetReaderName(sr),
                   LCCO_Card_GetStatus(card));
          if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted)
            LCSL_SlaveManager_SendCardInserted(slm, sr, card);
          /* we need to free the card here because when adding it to the
           * reader's list we called LCCO_Card_Attach(). */
          LCCO_Card_free(card);
          done++;
        }
  
        /* ... or to report any removed cards */
        while( (card=LCSL_Reader_GetNextRemovedCard(sr)) ) {
          LCSL_SlaveManager_SendCardRemoved(slm, sr, card);
          /* we need to free the card here because when adding it to the
           * reader's list we called LCCO_Card_Attach(). */
          LCCO_Card_free(card);
          done++;
        }
      }
  
      if (!LCCO_Reader_IsAvailable(sr) &&
          (flags & LCSL_READER_FLAGS_REPORTED_DOWN)) {
        LCDM_DEVICEMANAGER *dm;
  
        dm=LCS_Server_GetDeviceManager(slm->server);
        assert(dm);
  
        /* reader is down and the master knows it, so it is safe to
         * remove the reader now */
        if (flags & LCSL_READER_FLAGS_STARTED) {
          /* driver has been started by master, so
           * LCDM_DeviceManager_BeginUseReader has been called. We need to
           * undo that here */
          LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
          LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
        }
        /* remove from internal list and destroy reader */
        DBG_NOTICE(0, "Removing slave reader \"%s\"",
                   LCCO_Reader_GetReaderName(sr));
        LCCO_Reader_List2_Remove(slm->slaveReaders, sr);
        LCCO_Reader_free(sr);
        sr=0;
        done++;
      }
      sr=LCCO_Reader_List2Iterator_Next(it);
    } /* while */
    LCCO_Reader_List2Iterator_free(it);
  }

  if (done)
    return 1;

  return 0;
}



void LCSL_SlaveManager_NewCard(LCSL_SLAVEMANAGER *slm, LCCO_CARD *card) {
  GWEN_TYPE_UINT32 readerId;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  assert(slm);
  readerId=LCCO_Card_GetReaderId(card);
  assert(readerId);

  it=LCCO_Reader_List2_First(slm->slaveReaders);
  if (it) {
    sr=LCCO_Reader_List2Iterator_Data(it);
    assert(sr);
    while(sr) {
      if (LCCO_Reader_GetReaderId(sr)==readerId)
        break;
      sr=LCCO_Reader_List2Iterator_Next(it);
    }
    LCCO_Reader_List2Iterator_free(it);
  }

  if (!sr) {
    DBG_ERROR(0, "Reader for new card is not in slave pool");
    assert(0);
  }
  else {
    LCSL_Reader_AddInsertedCard(sr, card);
    DBG_INFO(0, "New card for slave reader \"%s\" enqueued",
             LCCO_Reader_GetReaderName(sr));
  }
}



void LCSL_SlaveManager_CardRemoved(LCSL_SLAVEMANAGER *slm,
                                   LCCO_CARD *card) {
  GWEN_TYPE_UINT32 readerId;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  assert(slm);
  readerId=LCCO_Card_GetReaderId(card);
  assert(readerId);

  it=LCCO_Reader_List2_First(slm->slaveReaders);
  if (it) {
    sr=LCCO_Reader_List2Iterator_Data(it);
    assert(sr);
    while(sr) {
      if (LCCO_Reader_GetReaderId(sr)==readerId)
        break;
      sr=LCCO_Reader_List2Iterator_Next(it);
    }
    LCCO_Reader_List2Iterator_free(it);
  }

  if (!sr) {
    DBG_ERROR(0, "Reader for removed card is not in slave pool");
    assert(0);
  }
  else {
    LCSL_Reader_AddRemovedCard(sr, card);
    DBG_DEBUG(0, "Removed card for slave reader \"%s\" enqueued",
              LCCO_Reader_GetReaderName(sr));
  }
}



void LCSL_SlaveManager_ConnectionDown(LCSL_SLAVEMANAGER *slm,
                                      GWEN_NETLAYER *nl) {
  DBG_WARN(0, "Connection to master is down, will restart in a few seconds");
  slm->disconnected=1;
}




int LCSL_SlaveManager_SendReaderAdd(LCSL_SLAVEMANAGER *slm,
                                    LCCO_READER *sr) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbReader;
  char numbuf[16];
  GWEN_TYPE_UINT32 rqid;
  int rv;

  DBG_NOTICE(0, "Announcing plugged-in reader \"%s\" to master",
             LCCO_Reader_GetReaderName(sr));
  dbReq=GWEN_DB_Group_new("Driver_ReaderAdd");
  dbReader=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT, "reader");
  assert(dbReader);

  LCCO_Reader_toDb(sr, dbReader);
  GWEN_DB_DeleteVar(dbReader, "readerId");
  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCSL_Reader_GetSlaveReaderId(sr));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);
  rqid=GWEN_IpcManager_SendRequest(slm->ipcManager,
                                   slm->ipcId,
                                   dbReq);
  if (rqid==0) {
    DBG_ERROR(0, "Could not send ReaderAdd request to master");
    return -1;
  }
  else {
    /* remove request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }

  return 0;
}



int LCSL_SlaveManager_SendReaderDel(LCSL_SLAVEMANAGER *slm,
                                     LCCO_READER *sr) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  GWEN_TYPE_UINT32 rqid;
  int rv;

  DBG_NOTICE(0, "Announcing unplugged reader \"%s\" to master",
             LCCO_Reader_GetReaderName(sr));
  dbReq=GWEN_DB_Group_new("Driver_ReaderDel");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCSL_Reader_GetSlaveReaderId(sr));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  rqid=GWEN_IpcManager_SendRequest(slm->ipcManager,
                                   slm->ipcId,
                                   dbReq);
  if (rqid==0) {
    DBG_ERROR(0, "Could not send ReaderDel request to master");
    return -1;
  }
  else {
    /* remove request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }
  return 0;
}



int LCSL_SlaveManager_SendCardInserted(LCSL_SLAVEMANAGER *slm,
                                       LCCO_READER *sr,
                                       LCCO_CARD *card) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  int slot;
  int cardnum;
  const void *atr;
  unsigned int atrLen;
  GWEN_TYPE_UINT32 rqid;
  const char *cardType;

  assert(slm);
  assert(sr);
  assert(card);

  DBG_NOTICE(0, "Announcing inserted card %08x in reader \"%s\" to master",
             LCCO_Card_GetCardId(card),
             LCCO_Reader_GetReaderName(sr));

  slot=LCCO_Card_GetSlotNum(card);
  cardnum=LCCO_Card_GetCardId(card);

  atr=LCCO_Card_GetAtr(card, &atrLen);
  switch(LCCO_Card_GetCardType(card)) {
  case LC_CardTypeProcessor: cardType="PROCESSOR"; break;
  case LC_CardTypeMemory:    cardType="MEMORY"; break;
  default:                   cardType=0; break;
  }

  dbReq=GWEN_DB_Group_new("Driver_CardInserted");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCSL_Reader_GetMasterReaderId(sr));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCCO_Card_GetReaderId(card));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slot);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardnum);

  if (atr && atrLen)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "atr", atr, atrLen);
  if (cardType)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "cardType", cardType);

  rqid=GWEN_IpcManager_SendRequest(slm->ipcManager,
                                   slm->ipcId,
                                   dbReq);
  if (rqid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }
  else {
    GWEN_IpcManager_RemoveRequest(slm->ipcManager, rqid, 1);
    DBG_DEBUG(0, "Command sent");
  }

  return 0;
}



int LCSL_SlaveManager_SendCardRemoved(LCSL_SLAVEMANAGER *slm,
                                      LCCO_READER *sr,
                                      LCCO_CARD *card) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rqid;

  assert(slm);
  assert(sr);
  assert(card);

  DBG_NOTICE(0, "Announcing removed card %08x in reader \"%s\" to master",
             LCCO_Card_GetCardId(card),
             LCCO_Reader_GetReaderName(sr));

  dbReq=GWEN_DB_Group_new("Driver_CardRemoved");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCSL_Reader_GetMasterReaderId(sr));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCCO_Card_GetReaderId(card));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LCCO_Card_GetSlotNum(card));

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", LCCO_Card_GetCardId(card));

  rqid=GWEN_IpcManager_SendRequest(slm->ipcManager,
                                   slm->ipcId,
                                   dbReq);
  if (rqid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }
  else {
    GWEN_IpcManager_RemoveRequest(slm->ipcManager, rqid, 1);
    DBG_DEBUG(0, "Command sent");
  }

  return 0;
}




#include "slr_command.c"
#include "slr_reset.c"
#include "slr_startreader.c"
#include "slr_stopreader.c"
#include "slr_suspendcheck.c"
#include "slr_resumecheck.c"









