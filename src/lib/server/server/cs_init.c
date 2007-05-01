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
  cs->cardManager=LCCM_CardManager_new(cs);

  cs->clientManager=LCCL_ClientManager_new(cs);
  cs->serviceManager=LCSV_ServiceManager_new(cs);
  cs->slaveManager=LCSL_SlaveManager_new(cs);

  return cs;
}



void LCS_Server_free(LCS_SERVER *cs) {
  if (cs) {
    GWEN_INHERIT_FINI(LCS_SERVER, cs);

    LCSL_SlaveManager_free(cs->slaveManager);
    LCSV_ServiceManager_free(cs->serviceManager);
    LCCL_ClientManager_free(cs->clientManager);
    LCCM_CardManager_free(cs->cardManager);
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
    const char *dhDir;
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

    /* get dh dir */
    dhDir=GWEN_DB_GetCharValue(gr, "dhdir", 0, LCS_DEFAULT_DHDIR);

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
                              dhDir,
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
                              dhDir,
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
  const char *s;

  assert(cs);
  assert(db);

  cs->requestManager=GWEN_IpcRequestManager_new(cs->ipcManager);
  cs->role=LCS_Server_RoleStandAlone;
  s=GWEN_DB_GetCharValue(db, "role", 0, "standAlone");
  if (s) {
    if (strcasecmp(s, "standAlone")==0)
      cs->role=LCS_Server_RoleStandAlone;
    else if (strcasecmp(s, "master")==0)
      cs->role=LCS_Server_RoleMaster;
    else if (strcasecmp(s, "slave")==0)
      cs->role=LCS_Server_RoleSlave;
    else {
      DBG_ERROR(0, "Unknown server role \"%s\", aborting", s);
      return -1;
    }
    DBG_INFO(0, "Server role: %s", s);
  }

  cs->disableAutoconf=
    GWEN_DB_GetIntValue(db, "disableAutoconf", 0, 0);
  if (cs->disableAutoconf) {
    DBG_WARN(0, "Autoconfiguration disabled");
  }
  else {
    DBG_INFO(0, "Autoconfiguration enabled");
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

  rv=LCDM_DeviceManager_Init(cs->deviceManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  rv=LCCM_CardManager_Init(cs->cardManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  if (LCS_Server_GetRole(cs)==LCS_Server_RoleSlave) {
    DBG_INFO(0, "Initialising slave manager");
    rv=LCSL_SlaveManager_Init(cs->slaveManager, db);
    if (rv) {
      DBG_INFO(0, "here (%d)", rv);
      return rv;
    }
  }
  else {
    rv=LCCL_ClientManager_Init(cs->clientManager, db);
    if (rv) {
      DBG_INFO(0, "here (%d)", rv);
      return rv;
    }

    rv=LCSV_ServiceManager_Init(cs->serviceManager, db);
    if (rv) {
      DBG_INFO(0, "here (%d)", rv);
      return rv;
    }
  }

  return 0;
}



int LCS_Server_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  int rv;
  int firstError=0;

  assert(cs);

  if (cs->role==LCS_Server_RoleSlave) {
    rv=LCSL_SlaveManager_Fini(cs->slaveManager, db);
    if (rv && firstError==0)
      firstError=rv;
  }
  else {
    rv=LCSV_ServiceManager_Fini(cs->serviceManager, db);
    if (rv && firstError==0)
      firstError=rv;

    rv=LCCL_ClientManager_Fini(cs->clientManager, db);
    if (rv && firstError==0)
      firstError=rv;
  }

  rv=LCCM_CardManager_Fini(cs->cardManager, db);
  if (rv && firstError==0)
    firstError=rv;

  rv=LCDM_DeviceManager_Fini(cs->deviceManager);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

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








LCS_SERVER_ASKADDCERT_FN
LCS_Server_SetAskAddCertFn(LCS_SERVER *cs,
                           LCS_SERVER_ASKADDCERT_FN fn) {
  LCS_SERVER_ASKADDCERT_FN oldFn;

  assert(cs);
  oldFn=cs->askAddCertFn;
  cs->askAddCertFn=fn;
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



LCCM_CARDMANAGER *LCS_Server_GetCardManager(const LCS_SERVER *cs) {
  assert(cs);
  return cs->cardManager;
}



LCSV_SERVICEMANAGER *LCS_Server_GetServiceManager(const LCS_SERVER *cs){
  assert(cs);
  return cs->serviceManager;
}



LCSL_SLAVEMANAGER *LCS_Server_GetSlaveManager(const LCS_SERVER *cs) {
  assert(cs);
  return cs->slaveManager;
}






int LCS_Server_CheckIpcResponse(GWEN_DB_NODE *db) {
  const char *name;

  if (strcasecmp(GWEN_DB_GroupName(db), "error")==0) {
    int numCode;
    const char *txt;
    int res;

    numCode=GWEN_DB_GetIntValue(db, "code", 0, LC_ERROR_GENERIC);
    txt=GWEN_DB_GetCharValue(db, "text", 0, "<empty>");
    DBG_DEBUG(0, "Error %x: %s", numCode, txt);
    switch(numCode) {
    case GWEN_IPC_ERROR_TIMEOUT:
      res=LC_ERROR_TIMEOUT;
      break;
    case GWEN_IPC_ERROR_CONNERR:
      res=LC_ERROR_IPC;
      break;
    case GWEN_IPC_ERROR_GENERIC:
    default:
      res=LC_ERROR_GENERIC;
      break;
    }
    return res;
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



LCS_SERVER_ROLE LCS_Server_GetRole(const LCS_SERVER *cs) {
  assert(cs);
  return cs->role;
}




