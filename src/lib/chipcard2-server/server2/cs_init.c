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


#include "server_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/directory.h>

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/nl_http.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_socket.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


GWEN_INHERIT_FUNCTIONS(LCS_SERVER)



LCS_SERVER *LCS_Server_new() {
  LCS_SERVER *cs;

  GWEN_NEW_OBJECT(LCS_SERVER, cs);
  GWEN_INHERIT_INIT(LCS_SERVER, cs);

  cs->ipcManager=GWEN_IpcManager_new();
  cs->deviceManager=LCDM_DeviceManager_new(cs);

  /* set default callbacks */
  cs->handleRequestFn=LCS_Server__HandleRequest;
  cs->connectionDownFn=LCS_Server__ConnectionDown;

  return cs;
}



void LCS_Server_free(LCS_SERVER *cs) {
  if (cs) {
    GWEN_INHERIT_FINI(LCS_SERVER, cs);

    LCDM_DeviceManager_free(cs->deviceManager);
    GWEN_IpcRequestManager_free(cs->requestManager);
    GWEN_IpcManager_free(cs->ipcManager);
    GWEN_DB_Group_free(cs->dbConfig);
    GWEN_FREE_OBJECT(cs);
  }
}



int LCS_Server__InitListener(LCS_SERVER *cs, GWEN_DB_NODE *gr) {
  int permissions=0666;
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

  assert(cs);
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
    unlink(address);
    permissions=GWEN_DB_GetIntValue(gr, "rights", 0, 0666);
    sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
    GWEN_InetAddr_SetAddress(addr, address);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetLocalAddr(nlBase, addr);
  }
  else if (strcasecmp(typ, "public")==0) {
    /* HTTP over TCP */
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetLocalAddr(nlBase, addr);
  }
  else {
    const char *certDir;
    const char *newCertDir;
    const char *ownCertFile;
    const char *ciphers;
    const char *dhFile;
    GWEN_BUFFER *cfbuf=0;
    GWEN_BUFFER *cdbuf=0;
    GWEN_BUFFER *ncdbuf=0;
    GWEN_BUFFER *dhbuf=0;

    /* get cert dir */
    certDir=GWEN_DB_GetCharValue(gr, "certdir", 0, 0);
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
    newCertDir=GWEN_DB_GetCharValue(gr, "newcertdir", 0, 0);
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

    /* get dh file */
    dhFile=GWEN_DB_GetCharValue(gr, "dhfile", 0, LCS_DEFAULT_DHFILE);
    if (1) { /* dhFile is always valid here */
      if (*dhFile!='/' && *dhFile!='\\') {
        GWEN_STRINGLIST *sl;
        int rv;

        sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                     LCS_PATH_SERVER_DATADIR);
        assert(sl);
        dhbuf=GWEN_Buffer_new(0, 256, 0, 1);
        rv=GWEN_Directory_FindPathForFile(sl, dhFile, dhbuf);
        GWEN_StringList_free(sl);
        if (rv) {
          DBG_ERROR(0, "DHFILE (%s) not found", dhFile);
          GWEN_Buffer_free(cfbuf);
          GWEN_Buffer_free(cdbuf);
          GWEN_Buffer_free(ncdbuf);
          GWEN_Buffer_free(dhbuf);
          GWEN_Url_free(url);
          return rv;
        }
        dhFile=GWEN_Buffer_GetStart(dhbuf);
      }
    }

    ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
    ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0,
                                     LCS_DEFAULT_CERTFILE);
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
    GWEN_NetLayer_SetLocalAddr(nlBase, addr);

    if (strcasecmp(typ, "private")==0) {
      /* HTTP over SSL */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              dhFile,
                              0);
      GWEN_NetLayer_free(nlBase);
      nlBase=nl;
    }
    else if (strcasecmp(typ, "secure")==0) {
      /* HTTP over SSL with certificates */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              dhFile,
                              1);
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

  sid=GWEN_IpcManager_AddServer(cs->ipcManager,
                                nl,
                                LCS_MARK_SERVER);
  if (sid==0) {
    DBG_ERROR(0, "Could not add server");
    GWEN_DB_Dump(gr, stderr, 2);
    GWEN_NetLayer_free(nl);
    GWEN_Url_free(url);
    return -1;
  }

  /* adjust unix domain socket */
  if (strcasecmp(typ, "local")==0) {
    if (chmod(address, permissions)) {
      DBG_ERROR(0, "Could not change permissions for service");
      GWEN_DB_Dump(gr, stderr, 2);
      GWEN_Url_free(url);
      return -1;
    }
  }

  GWEN_Url_free(url);

  return 0;
}



int LCS_Server__InitListeners(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *gr=0;
  int servers=0;

  assert(cs);
  assert(db);

  gr=GWEN_DB_FindFirstGroup(db, "server");
  while(gr) {
    int rv;

    rv=LCS_Server__InitListener(cs, gr);
    if (rv==0)
      servers++;
    gr=GWEN_DB_FindNextGroup(gr, "server");
  }

  if (servers==0) {
    DBG_ERROR(0, "Could not init servers (maybe no \"server\" section?)");
    return -1;
  }

  return 0;
}



int LCS_Server__InitPaths(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *dbT;
  const char *s;
  GWEN_PLUGIN_MANAGER *pm;
  int rv;

  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "paths");

#define DEF_PATH(p) \
  GWEN_PathManager_DefinePath(LCS_PATH_DESTLIB, LCS_PATH_##p);  \
  if (dbT && (s=GWEN_DB_GetCharValue(dbT, LCS_PATH_##p, 0, 0))) \
    GWEN_PathManager_AddPath(LCS_PATH_DESTLIB,                  \
                             LCS_PATH_DESTLIB,                  \
                             LCS_PATH_##p,                      \
                             s);                                \
  GWEN_PathManager_AddPathFromWinReg(LCS_PATH_DESTLIB,          \
                                     LCS_PATH_DESTLIB,          \
                                     LCS_PATH_##p,              \
                                     LCS_REGKEY_BASE,           \
                                     LCS_PATH_##p);             \
  GWEN_PathManager_AddPath(LCS_PATH_DESTLIB,                    \
                           LCS_PATH_DESTLIB,                    \
                           LCS_PATH_##p,                        \
                           DEF_##p);
  DEF_PATH(DRIVER_INFODIR)
  DEF_PATH(SERVER_DATADIR)
  DEF_PATH(SERVER_TRUSTEDCERTDIR)
  DEF_PATH(SERVER_NEWCERTDIR)
  DEF_PATH(SERVER_LOGDIR)
#undef DEF_PATH

  /* create and register plugin manager for driver plugins */
  pm=GWEN_PluginManager_new(LCS_PLUGIN_DRIVER);
  if (dbT && (s=GWEN_DB_GetCharValue(dbT, LCS_PATH_DRIVER_EXECDIR, 0, 0)))
    GWEN_PluginManager_AddPath(pm, s);
  GWEN_PluginManager_AddPathFromWinReg(pm,
                                       LCS_REGKEY_BASE,
                                       LCS_PATH_DRIVER_EXECDIR);
  GWEN_PluginManager_AddPath(pm, DEF_DRIVER_EXECDIR);
  rv=GWEN_PluginManager_Register(pm);
  if (rv) {
    DBG_ERROR(0, "Unable to register plugin manager for "
              LCS_PLUGIN_DRIVER
              " (%d)", rv);
    GWEN_PluginManager_free(pm);
    return rv;
  }
  cs->driverPluginManager=pm;

  /* create and register plugin manager for service plugins */
  pm=GWEN_PluginManager_new(LCS_PLUGIN_SERVICE);
  if (dbT && (s=GWEN_DB_GetCharValue(dbT, LCS_PATH_SERVICE_EXECDIR, 0, 0)))
    GWEN_PluginManager_AddPath(pm, s);
  GWEN_PluginManager_AddPathFromWinReg(pm,
                                       LCS_REGKEY_BASE,
                                       LCS_PATH_SERVICE_EXECDIR);
  GWEN_PluginManager_AddPath(pm, DEF_SERVICE_EXECDIR);
  rv=GWEN_PluginManager_Register(pm);
  if (rv) {
    DBG_ERROR(0, "Unable to register plugin manager for "
              LCS_PLUGIN_SERVICE
              " (%d)", rv);
    GWEN_PluginManager_free(pm);
    return rv;
  }
  cs->servicePluginManager=pm;

  return 0;
}



int LCS_Server_Init(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  int rv;

  assert(cs);
  assert(db);

  cs->requestManager=GWEN_IpcRequestManager_new(cs->ipcManager);

  cs->disableAutoconf=
    GWEN_DB_GetIntValue(db, "disableAutoconf", 0, 0);
  if (cs->disableAutoconf) {
    DBG_WARN(0, "Autoconfiguration disabled");
  }
  else {
    DBG_NOTICE(0, "Autoconfiguration enabled");
  }

  DBG_INFO(0, "Initialising paths");
  rv=LCS_Server__InitPaths(cs, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initialising IPC manager");
  rv=LCS_Server__InitListeners(cs, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initialising device manager");
  rv=LCDM_DeviceManager_Init(cs->deviceManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  return 0;
}



int LCS_Server_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db) {

  assert(cs);

  GWEN_IpcRequestManager_free(cs->requestManager);
  cs->requestManager=0;

  GWEN_PathManager_RemovePaths(LCS_PATH_DESTLIB);
  if (cs->servicePluginManager) {
    GWEN_PluginManager_Unregister(cs->servicePluginManager);
    GWEN_PluginManager_free(cs->servicePluginManager);
    cs->servicePluginManager=0;
  }
  if (cs->driverPluginManager) {
    GWEN_PluginManager_Unregister(cs->driverPluginManager);
    GWEN_PluginManager_free(cs->driverPluginManager);
    cs->driverPluginManager=0;
  }
  return 0;
}



LCS_SERVER_DRIVER_CHG_FN
LCS_Server_SetDriverChgFn(LCS_SERVER *cs,
                          LCS_SERVER_DRIVER_CHG_FN f){
  LCS_SERVER_DRIVER_CHG_FN oldFn;

  assert(cs);
  oldFn=cs->driverChgFn;
  cs->driverChgFn=f;
  return oldFn;
}



LCS_SERVER_READER_CHG_FN
LCS_Server_SetReaderChgFn(LCS_SERVER *cs,
                          LCS_SERVER_READER_CHG_FN f){
  LCS_SERVER_READER_CHG_FN oldFn;

  assert(cs);
  oldFn=cs->readerChgFn;
  cs->readerChgFn=f;
  return oldFn;
}



LCS_SERVER_NEWCARD_FN
LCS_Server_SetNewCardFn(LCS_SERVER *cs,
                        LCS_SERVER_NEWCARD_FN f) {
  LCS_SERVER_NEWCARD_FN oldFn;

  assert(cs);
  oldFn=cs->newCardFn;
  cs->newCardFn=f;
  return oldFn;
}



LCS_SERVER_CARDREMOVED_FN
LCS_Server_SetCardRemovedFn(LCS_SERVER *cs,
                            LCS_SERVER_CARDREMOVED_FN f) {
  LCS_SERVER_CARDREMOVED_FN oldFn;

  assert(cs);
  oldFn=cs->cardRemovedFn;
  cs->cardRemovedFn=f;
  return oldFn;
}



LCS_SERVER_HANDLEREQUEST_FN
LCS_Server_SetHandleRequestFn(LCS_SERVER *cs,
                              LCS_SERVER_HANDLEREQUEST_FN f) {
  LCS_SERVER_HANDLEREQUEST_FN oldFn;

  assert(cs);
  oldFn=cs->handleRequestFn;
  cs->handleRequestFn=f;
  return oldFn;
}



LCS_SERVER_CONNECTION_DOWN_FN
LCS_Server_SetConnectionDownFn(LCS_SERVER *cs,
                               LCS_SERVER_CONNECTION_DOWN_FN f) {
  LCS_SERVER_CONNECTION_DOWN_FN oldFn;

  assert(cs);
  oldFn=cs->connectionDownFn;
  cs->connectionDownFn=f;
  return oldFn;
}



LCS_SERVER_SERVICE_CHG_FN
LCS_Server_SetServiceChgFn(LCS_SERVER *cs,
                          LCS_SERVER_SERVICE_CHG_FN f){
  LCS_SERVER_SERVICE_CHG_FN oldFn;

  assert(cs);
  oldFn=cs->serviceChgFn;
  cs->serviceChgFn=f;
  return oldFn;
}











GWEN_IPCMANAGER *LCS_Server_GetIpcManager(const LCS_SERVER *cs) {
  assert(cs);
  return cs->ipcManager;
}



GWEN_IPC_REQUEST_MANAGER *LCS_Server_GetRequestManager(const LCS_SERVER *cs){
  assert(cs);
  return cs->requestManager;
}



LCDM_DEVICEMANAGER *LCS_Server_GetDeviceManager(const LCS_SERVER *cs) {
  assert(cs);
  return cs->deviceManager;
}



void LCS_Server_SetDeviceManager(LCS_SERVER *cs, LCDM_DEVICEMANAGER *dm) {
  assert(cs);
  cs->deviceManager=dm;
}











