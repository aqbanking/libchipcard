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
#include <gwenhywfar/netconnectionhttp.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/common/usbmonitor.h>
#include <chipcard2-server/common/usbttymonitor.h>
#include <chipcard2-server/common/driverinfo.h>

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



LC_CARDSERVER *LC_CardServer_new(const char *dataDir){
  LC_CARDSERVER *cs;
  GWEN_STRINGLIST *sl;

  GWEN_NEW_OBJECT(LC_CARDSERVER, cs);
  cs->ipcManager=GWEN_IPCManager_new();
  cs->drivers=LC_Driver_List_new();
  cs->readers=LC_Reader_List_new();
  cs->clients=LC_Client_List_new();
  cs->activeCards=LC_Card_List_new();
  cs->freeCards=LC_Card_List_new();
  cs->requests=LC_Request_List_new();
  cs->services=LC_Service_List_new();

  cs->readerStartTimeout=LC_CARDSERVER_DEFAULT_READERSTARTTIMEOUT;
  cs->driverStartTimeout=LC_CARDSERVER_DEFAULT_DRIVERSTARTTIMEOUT;
  cs->readerIdleTimeout=LC_CARDSERVER_DEFAULT_READERIDLETIMEOUT;
  cs->driverIdleTimeout=LC_CARDSERVER_DEFAULT_DRIVERIDLETIMEOUT;

  if (dataDir)
    cs->dataDir=strdup(dataDir);
  else {
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR, tbuf, 1);
    cs->dataDir=strdup(GWEN_Buffer_GetStart(tbuf));
    GWEN_Buffer_free(tbuf);
  }

  cs->dbDrivers=GWEN_DB_Group_new("drivers");
  cs->dbConfigDrivers=GWEN_DB_Group_new("configDrivers");

  cs->usbTtyMonitor=LC_USBTTYMonitor_new();
  cs->usbMonitor=LC_USBMonitor_new();

  /* create card manager */
  sl=GWEN_StringList_new();
  LC_CardServer_SampleDirs(cs, sl);
  cs->cardManager=LC_CardMgr_new(sl);

  /* sample drivers and readers from XML files */
  LC_CardServer_SampleDrivers(cs, sl, cs->dbDrivers, 1);
  GWEN_StringList_free(sl);

  return cs;
}



void LC_CardServer_free(LC_CARDSERVER *cs){
  if (cs) {
    LC_DRIVER *d;
    LC_CLIENT *cl;
    int rv;

    /* remove IPC nodes of all clients */
    cl=LC_Client_List_First(cs->clients);
    while(cl) {
      GWEN_TYPE_UINT32 nid;
    
      /* remove the IPC node of this client */
      nid=LC_Client_GetClientId(cl);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Client_GetClientId(cl));
        }
      }
      cl=LC_Client_List_Next(cl);
    }

    /* remove IPC nodes of all drivers */
    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      GWEN_TYPE_UINT32 nid;
    
      /* remove the IPC node of this driver */
      nid=LC_Driver_GetIpcId(d);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
      }
      d=LC_Driver_List_Next(d);
    }

    LC_USBTTYMonitor_free(cs->usbTtyMonitor);
    LC_USBMonitor_free(cs->usbMonitor);
    LC_CardMgr_free(cs->cardManager);
    free(cs->dataDir);
    free(cs->typeForDrivers);
    free(cs->addrForDrivers);
    GWEN_DB_Group_free(cs->dbConfigDrivers);
    GWEN_DB_Group_free(cs->dbDrivers);
    LC_Request_List_free(cs->requests);
    LC_Reader_List_free(cs->readers);
    LC_Driver_List_free(cs->drivers);
    LC_Client_List_free(cs->clients);
    LC_Card_List_free(cs->activeCards);
    LC_Card_List_free(cs->freeCards);
    LC_Service_List_free(cs->services);
    GWEN_IPCManager_free(cs->ipcManager);
    GWEN_FREE_OBJECT(cs);
  }
}



int LC_CardServer_CheckClient(LC_CARDSERVER *cs, LC_CLIENT *cl) {
  GWEN_NETCONNECTION *conn;
  int isDown;

  conn=GWEN_IPCManager_GetConnection(cs->ipcManager,
                                     LC_Client_GetClientId(cl));
  if (conn==0) isDown=1;
  else
    isDown=(GWEN_NetConnection_GetStatus(conn)!=
            GWEN_NetTransportStatusLConnected);
  return isDown!=0;
}



int LC_CardServer_CheckClients(LC_CARDSERVER *cs) {
  LC_CLIENT *cl;
  int done;

  done=0;
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    LC_CLIENT *next;

    next=LC_Client_List_Next(cl);
    if (LC_CardServer_CheckClient(cs, cl)) {
      DBG_NOTICE(0, "Client \"%08x\" down",
                 LC_Client_GetClientId(cl));
      LC_CardServer_ClientDown(cs, cl);
      done++;
    }
    cl=next;
  } /* while */

  return done?0:1;
}



void LC_CardServer_ClientDown(LC_CARDSERVER *cs, LC_CLIENT *cl) {
  LC_CARD *card;
  LC_READER *r;
  LC_SERVICE *as;
  LC_REQUEST *rq;
  int rv;

  /* remove all requests of this client */
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    LC_REQUEST *next;

    next=LC_Request_List_Next(rq);
    if (LC_Request_GetClient(rq)==cl) {
      LC_Request_List_Del(rq);
      LC_Request_free(rq);
    }
    rq=next;
  } /* while */

  /* release all cards used by this client */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetClient(card)==cl) {
      LC_DRIVER *d;
      GWEN_TYPE_UINT32 rid;

      /* card in use by client, reset it */
      DBG_NOTICE(0,
                 "Card \"%08x\" still used by client \"%08x\", "
                 "releasing it",
                 LC_Card_GetCardId(card), LC_Client_GetClientId(cl));
      r=LC_Card_GetReader(card);
      assert(r);
      d=LC_Reader_GetDriver(r);
      assert(d);

      /* Reset card */
      rid=LC_CardServer_SendResetCard(cs, card);
      if (rid==0) {
        DBG_ERROR(0, "Could not send card reset request");
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
                                             r,
					     "Could not send card reset "
					     "request");
      }
      else
        /* we don't expect an answer, delete request */
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);

      /* release card from clients handle */
      LC_Card_SetClient(card, 0);
      LC_Card_SetContext(card, 0);

      /* move card from activeCards to freeCards */
      LC_Card_List_Del(card);
      LC_Card_List_Add(card, cs->freeCards);
    }
    card=LC_Card_List_Next(card);
  } /* while card */

  /* release all readers currently in use by the client */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    /* remove all requests of the given client */
    LC_Reader_DelClientRequests(r, cl);
    if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
      DBG_NOTICE(0, "Reader \"%s\" in use by client \"%08x\"",
                 LC_Reader_GetReaderName(r), LC_Client_GetClientId(cl));
      DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
      LC_Reader_DecUsageCount(r);
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  /* release all services currently in use by the client */
  as=LC_Service_List_First(cs->services);
  while(as) {
    if (LC_Client_HasService(cl, LC_Service_GetServiceId(as))) {
      DBG_NOTICE(0, "Service \"%s\" in use by client \"%08x\"",
                 LC_Service_GetServiceName(as), LC_Client_GetClientId(cl));
      DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
      LC_Service_DecActiveClientsCount(as);
    }
    as=LC_Service_List_Next(as);
  } /* while */

  /* remove the IPC node of this client */
  rv=GWEN_IPCManager_RemoveClient(cs->ipcManager,
                                  LC_Client_GetClientId(cl));
  if (rv) {
    DBG_WARN(0, "Error removing IPC node of client \"%08x\"",
	     LC_Client_GetClientId(cl));
  }

  LC_Client_List_Del(cl);
  LC_Client_free(cl);
}



int LC_CardServer_ReplaceVar(const char *path,
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



LC_CLIENT *LC_CardServer_FindClient(const LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 id){
  LC_CLIENT *cl;

  assert(cs);
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==id)
      return cl;
    cl=LC_Client_List_Next(cl);
  } /* while */
  return 0;
}



void LC_CardServer__Up(GWEN_NETCONNECTION *conn){
}



void LC_CardServer__Down(GWEN_NETCONNECTION *conn){
  DBG_ERROR(0, "Connection down"); // DEBUG
}



int LC_CardServer_ReadConfig(LC_CARDSERVER *cs, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *gr;
  int drivers;
  int servers;
  int permissions;

  assert(cs);
  assert(db);

  permissions=0666;
  drivers=0;
  servers=0;
  cs->disableAutoconf=
      GWEN_DB_GetIntValue(db, "disableAutoconf", 0, 0);
  if (cs->disableAutoconf) {
    DBG_WARN(0, "USB Autoconfiguration disabled");
  }
  else {
    DBG_NOTICE(0, "USB Autoconfiguration enabled");
  }

  cs->allowRemote=GWEN_DB_GetIntValue(db, "allowRemote", 0, 0);

  cs->driverStartTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverStartTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERSTARTTIMEOUT);
  cs->driverStopTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverStopTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERSTOPTIMEOUT);
  cs->readerStartTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerStartTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERSTARTTIMEOUT);

  cs->driverIdleTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverIdleTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERIDLETIMEOUT);

  cs->driverRestartTime=
    GWEN_DB_GetIntValue(db,
                        "driverRestartTime", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERRSTTIME);

  cs->readerIdleTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerIdleTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERIDLETIMEOUT);

  cs->readerCommandTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerCommandTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERCMDTIMEOUT);

  cs->readerRestartTime=
    GWEN_DB_GetIntValue(db,
                        "readerRestartTime", 0,
                        LC_CARDSERVER_DEFAULT_READERRSTTIME);

  cs->usbScanInterval=
    GWEN_DB_GetIntValue(db,
			"usbScanInterval", 0,
                        LC_CARDSERVER_DEFAULT_USB_SCAN_INTERVAL);
  if (cs->usbScanInterval==0) {
    DBG_WARN(0, "Will only scan once for raw USB devices");
  }
  cs->usbTtyScanInterval=
    GWEN_DB_GetIntValue(db,
			"usbTtyScanInterval", 0,
			LC_CARDSERVER_DEFAULT_USBTTY_SCAN_INTERVAL);
  if (cs->usbTtyScanInterval==0) {
    DBG_WARN(0, "Will only scan once for ttyUSB devices");
  }

  gr=GWEN_DB_GetFirstGroup(db);
  while(gr) {
    if (strcasecmp(GWEN_DB_GroupName(gr), "driver")==0) {
      LC_DRIVER *d;
      GWEN_DB_NODE *dbR;

      /* driver section found, add it to global list and create a driver */
      GWEN_DB_InsertGroup(cs->dbDrivers,
			  GWEN_DB_Group_dup(gr));
      GWEN_DB_InsertGroup(cs->dbConfigDrivers,
			  GWEN_DB_Group_dup(gr));

      d=LC_Driver_FromDb(gr);
      assert(d);

      if (!LC_Driver_GetLogFile(d)) {
        GWEN_BUFFER *lbuf;

        lbuf=GWEN_Buffer_new(0, 256, 0, 1);
        GWEN_Directory_OsifyPath(LC_DEFAULT_LOGDIR, lbuf, 1);
        LC_CardServer_ReplaceVar("/drivers/@driver@"
                                 "/@reader@"
                                 ".log",
                                 "driver",
                                 LC_Driver_GetDriverName(d),
                                 lbuf);
        LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
        GWEN_Buffer_free(lbuf);
      }

      DBG_INFO(0, "Adding driver \"%s\"", LC_Driver_GetDriverName(d));
      LC_Driver_List_Add(d, cs->drivers);
      drivers++;

      dbR=GWEN_DB_GetFirstGroup(gr);
      while(dbR) {
        if (strcasecmp(GWEN_DB_GroupName(dbR), "reader")==0) {
          LC_READER *r;

          /* reader section found */
          r=LC_Reader_FromDb(d, dbR);
          assert(r);
          LC_Driver_IncAssignedReadersCount(d);
          DBG_INFO(0, "Adding reader \"%s\"", LC_Reader_GetReaderName(r));
          /* readers from config file are always assumed available */
          LC_Reader_SetIsAvailable(r, 1);
          LC_Reader_List_Add(r, cs->readers);
        } /* if reader */
        dbR=GWEN_DB_GetNextGroup(dbR);
      } /* while */
    } /* if driver */

    else if (strcasecmp(GWEN_DB_GroupName(gr), "service")==0) {
      LC_SERVICE *as;

      /* service section found */
      as=LC_Service_FromDb(gr);
      assert(as);
      DBG_INFO(0, "Adding service \"%s\"", LC_Service_GetServiceName(as));

      /* preset logfile if none given */
      if (!LC_Service_GetLogFile(as)) {
        GWEN_BUFFER *lbuf;

        lbuf=GWEN_Buffer_new(0, 256, 0, 1);
        GWEN_Directory_OsifyPath(LC_DEFAULT_LOGDIR, lbuf, 1);
        LC_CardServer_ReplaceVar("/services/@service@.log",
                                 "service",
                                 LC_Service_GetServiceName(as),
                                 lbuf);
        LC_Service_SetLogFile(as, GWEN_Buffer_GetStart(lbuf));
        GWEN_Buffer_free(lbuf);
      }

      LC_Service_List_Add(as, cs->services);
    }
    else if (strcasecmp(GWEN_DB_GroupName(gr), "server")==0) {
      const char *typ;
      GWEN_NETTRANSPORT *tr;
      GWEN_SOCKET *sk;
      GWEN_INETADDRESS *addr;
      GWEN_TYPE_UINT32 sid;
      const char *address;
      GWEN_NETCONNECTION *conn;

      typ=GWEN_DB_GetCharValue(gr, "typ", 0, "local");
      address=GWEN_DB_GetCharValue(gr,
                                   "addr", 0, 0);
      if (!address) {
        DBG_ERROR(0, "No address given");
        return -1;
      }

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
          return -1;
        }
        if (chmod(taddr, 0777)) {
          DBG_ERROR(0, "Could not set permissions for path \"%s\"", taddr);
          free(taddr);
          return -1;
        }
        free(taddr);
        unlink(address);
        permissions=GWEN_DB_GetIntValue(gr, "rights", 0, 0666);
        sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
        addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
        GWEN_InetAddr_SetAddress(addr, address);
	tr=GWEN_NetTransportSocket_new(sk, 1);
      }
      else if (strcasecmp(typ, "public")==0) {
	/* HTTP over TCP */
	sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
	GWEN_InetAddr_SetAddress(addr,
				 GWEN_DB_GetCharValue
				 (gr,
				  "addr", 0,
				  "0.0.0.0"
				 )
				);
	GWEN_InetAddr_SetPort(addr,
			      GWEN_DB_GetIntValue(gr,
						  "port", 0,
						  LC_DEFAULT_PORT));
	tr=GWEN_NetTransportSocket_new(sk, 1);
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
          cdbuf=GWEN_Buffer_new(0, 256, 0, 1);
          GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR"/"
                                   "certificates/valid",
                                   cdbuf, 1);
          certDir=GWEN_Buffer_GetStart(cdbuf);
        }

        /* get new cert dir */
        newCertDir=GWEN_DB_GetCharValue(gr, "newcertdir", 0, 0);
        if (!newCertDir) {
          ncdbuf=GWEN_Buffer_new(0, 256, 0, 1);
          GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR"/"
                                   "certificates/new",
                                   ncdbuf, 1);
          newCertDir=GWEN_Buffer_GetStart(ncdbuf);
        }

        /* get dh file */
        dhFile=GWEN_DB_GetCharValue(gr, "dhfile", 0, 0);
        if (!dhFile) {
          dhbuf=GWEN_Buffer_new(0, 256, 0, 1);
          GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR
                                   DIRSEP
                                   LC_DEFAULT_DHFILE,
                                   dhbuf, 1);
          dhFile=GWEN_Buffer_GetStart(dhbuf);
        }

        ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
        ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0, 0);
        if (ownCertFile) {
          if (*ownCertFile!=DIRSEPC) {
            FILE *f;

            /* check whether the file exists */
            f=fopen(ownCertFile, "r");
            if (f)
              fclose(f);
            else {
              cfbuf=GWEN_Buffer_new(0, 256, 0, 1);
              GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR, cfbuf, 1);
              GWEN_Buffer_AppendByte(cfbuf, DIRSEPC);
              GWEN_Buffer_AppendString(cfbuf, ownCertFile);
              ownCertFile=GWEN_Buffer_GetStart(cfbuf);
            }
          }
        }
	addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
	GWEN_InetAddr_SetAddress(addr,
				 GWEN_DB_GetCharValue(gr,
						      "addr", 0,
						      "0.0.0.0"));
	GWEN_InetAddr_SetPort(addr,
			      GWEN_DB_GetIntValue(gr,
						  "port", 0,
						  LC_DEFAULT_PORT));
	if (strcasecmp(typ, "private")==0) {
	  /* HTTP over SSL */
	  sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	  tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
                                      ownCertFile,
                                      dhFile,
				      0,
				      1);
	}
	else if (strcasecmp(typ, "secure")==0) {
	  /* HTTP over SSL with certificates */
	  sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	  tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
				      ownCertFile,
                                      dhFile,
				      1,
				      1);
	}
	else {
	  DBG_ERROR(0, "Unknown mode \"%s\"", typ);
          GWEN_InetAddr_free(addr);
          GWEN_Buffer_free(cfbuf);
          GWEN_Buffer_free(cdbuf);
          GWEN_Buffer_free(ncdbuf);
          GWEN_Buffer_free(dhbuf);
	  return -1;
	}
        GWEN_Buffer_free(cfbuf);
        GWEN_Buffer_free(cdbuf);
        GWEN_Buffer_free(ncdbuf);
        GWEN_Buffer_free(dhbuf);

        if (ciphers)
          GWEN_NetTransportSSL_SetCipherList(tr, ciphers);
      }

      GWEN_NetTransport_SetLocalAddr(tr, addr);
      GWEN_InetAddr_free(addr);
      sid=GWEN_IPCManager_AddServer(cs->ipcManager,
				    tr,
				    LC_CARDSERVER_MARK_SERVER);
      if (sid==0) {
	DBG_ERROR(0, "Could not add service");
	GWEN_DB_Dump(gr, stderr, 2);
        return -1;
      }

      conn=GWEN_IPCManager_GetConnection(cs->ipcManager, sid);
      assert(conn);
      GWEN_NetConnectionHTTP_SetDefaultURL(conn, "/libchipcard2/server");


      if (strcasecmp(typ, "local")==0) {
        if (chmod(GWEN_DB_GetCharValue(gr,
                                       "addr", 0,
                                       LC_DEFAULT_UDS_SOCK),
                  permissions)) {
          DBG_ERROR(0, "Could not change permissions for service");
          GWEN_DB_Dump(gr, stderr, 2);
          return -1;
        }
      }
      GWEN_IPCManager_SetUpFn(cs->ipcManager, sid, LC_CardServer__Up);
      GWEN_IPCManager_SetDownFn(cs->ipcManager, sid,
				LC_CardServer__Down);

      DBG_INFO(0, "Service added");
      servers++;
      if (cs->typeForDrivers==0) {
        cs->typeForDrivers=strdup(GWEN_DB_GetCharValue(gr,
                                                       "typ", 0,
                                                       "local"));
        free(cs->addrForDrivers);
        cs->addrForDrivers=strdup(GWEN_DB_GetCharValue(gr,
                                                       "addr", 0,
                                                       "0.0.0.0"));
        cs->portForDrivers=GWEN_DB_GetIntValue(gr,
                                               "port", 0,
                                               LC_DEFAULT_PORT);
      }
    }

    gr=GWEN_DB_GetNextGroup(gr);
  } /* while */

  /*
  if (!drivers) {
    DBG_ERROR(0, "No drivers in this configuration");
    return -1;
  }
  */
  if (!servers) {
    DBG_ERROR(0, "No servers in this configuration");
    return -1;
  }
  return 0;
}



void LC_CardServer_CollectCommands(LC_CARDSERVER *cs) {
  //TODO
}



int LC_CardServer_SendErrorResponse(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    int code,
				    const char *text) {
  GWEN_DB_NODE *gr;

  gr=GWEN_DB_Group_new("Error");

  GWEN_DB_SetIntValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", code);
  if (text)
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", text);
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, gr)) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_GetFlags(GWEN_DB_NODE *db,
                                        const char *vname){
  unsigned int i;
  GWEN_TYPE_UINT32 f;

  f=0;
  for (i=0; ; i++) {
    const char *p;

    p=GWEN_DB_GetCharValue(db, vname, i, 0);
    if (!p)
      break;
    if (strcasecmp(p, "KEYPAD")==0)
      f|=LC_READER_FLAGS_KEYPAD;
    else if (strcasecmp(p, "DISPLAY")==0)
      f|=LC_READER_FLAGS_DISPLAY;
    else if (strcasecmp(p, "NOINFO")==0)
      f|=LC_READER_FLAGS_NOINFO;
    else if (strcasecmp(p, "REMOTE")==0)
      f|=LC_READER_FLAGS_REMOTE;
    else if (strcasecmp(p, "AUTO")==0)
      f|=LC_READER_FLAGS_AUTO;
  } /* for */

  return f;
}



int LC_CardServer_HandleClientReady(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_NETCONNECTION *conn;
  GWEN_NETTRANSPORT *tr;
  const char *p;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (cl) {
    DBG_ERROR(0, "Client already started");
    /* TODO: Send SegResult */
    return -1;
  }

  cl=LC_Client_new(clientId);
  LC_Client_List_Add(cl, cs->clients);

  conn=GWEN_IPCManager_GetConnection(cs->ipcManager, clientId);
  assert(conn);
  tr=GWEN_NetConnection_GetTransportLayer(conn);
  assert(tr);
  LC_ServerConn_TakeOver(conn);
  LC_ServerConn_SetCardServer(conn, cs);
  LC_ServerConn_SetType(conn, LC_ServerConn_TypeClient);

  LC_Client_SetUserName(cl, "nobody");
  if (GWEN_NetTransportSSL_IsOfType(tr)){
    DBG_INFO(0, "Got an SSL connection, checking...");
    if (GWEN_NetTransportSSL_IsSecure(tr)) {
      GWEN_DB_NODE *dbCert;
      const char *p;

      DBG_INFO(0, "Got a secure SSL connection");
      dbCert=GWEN_NetTransportSSL_GetPeerCertificate(tr);
      assert(dbCert);

      p=GWEN_DB_GetCharValue(dbCert, "commonName", 0, "nobody");
      DBG_NOTICE(0, "Verified peer is \"%s\"", p);
      LC_Client_SetUserName(cl, p);
    }
  }

  DBG_NOTICE(0, "Client %08x: ClientReady [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  p=GWEN_DB_GetCharValue(dbReq, "body/application", 0, "");
  DBG_NOTICE(0, "Client \"%08x\" started (%s, Gwen %s, ChipCard %s)",
             clientId,
             p,
	     GWEN_DB_GetCharValue(dbReq, "body/GwenVersion", 0, ""),
	     GWEN_DB_GetCharValue(dbReq, "body/ChipcardVersion", 0, ""));

  LC_Client_SetApplicationName(cl, p);

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("ClientReadyResponse");
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "GwenVersion", GWENHYWFAR_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ChipcardVersion", CHIPCARD_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "System", ""); /* TODO: Get system string */

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to ClientReady");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* remove request */
  if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove ClientReady request");
  }

  DBG_DEBUG(0, "Response sent.");
  return 0;
}



int LC_CardServer_HandleNextCommand(LC_CARDSERVER *cs) {
  const char *name;
  int rv;
  GWEN_TYPE_UINT32 ridNext;
  GWEN_DB_NODE *dbReq;

  assert(cs);

  ridNext=GWEN_IPCManager_GetNextInRequest(cs->ipcManager, 0);
  if (!ridNext) {
    DBG_VERBOUS(0, "No incoming request");
    return 1;
  }
  dbReq=GWEN_IPCManager_GetInRequestData(cs->ipcManager, ridNext);
  assert(dbReq);
  name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC command (no command name), discarding");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0);
    rv=-1;
  }
  else {
    DBG_INFO(0, "Command \"%s\"", name);
    if (strcasecmp(name, "DriverReady")==0) {
      rv=LC_CardServer_HandleDriverReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceReady")==0) {
      rv=LC_CardServer_HandleServiceReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceNotification")==0) {
      rv=LC_CardServer_HandleServiceNotification(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ClientReady")==0) {
      rv=LC_CardServer_HandleClientReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardInserted")==0) {
      rv=LC_CardServer_HandleCardInserted(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ReaderError")==0) {
      rv=LC_CardServer_HandleReaderError(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardRemoved")==0) {
      rv=LC_CardServer_HandleCardRemoved(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "StartWait")==0) {
      rv=LC_CardServer_HandleStartWait(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "StopWait")==0) {
      rv=LC_CardServer_HandleStopWait(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "TakeCard")==0) {
      rv=LC_CardServer_HandleTakeCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ReleaseCard")==0) {
      rv=LC_CardServer_HandleReleaseCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CommandCard")==0) {
      rv=LC_CardServer_HandleCommandCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ExecCommand")==0) {
      rv=LC_CardServer_HandleExecCommand(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "SelectCard")==0) {
      rv=LC_CardServer_HandleSelectCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "SetNotify")==0) {
      rv=LC_CardServer_HandleSetNotify(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "GetDriverVar")==0) {
      rv=LC_CardServer_HandleGetDriverVar(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardCheck")==0) {
      rv=LC_CardServer_HandleCardCheck(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardReset")==0) {
      rv=LC_CardServer_HandleCardReset(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceOpen")==0) {
      rv=LC_CardServer_HandleServiceOpen(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceClose")==0) {
      rv=LC_CardServer_HandleServiceClose(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceCommand")==0) {
      rv=LC_CardServer_HandleServiceCommand(cs, ridNext, dbReq);
    }

    /* Insert more handlers here */
    else {
      DBG_ERROR(0, "Unknown command \"%s\", discarding", name);
      rv=-1;
    }
  }

  if (rv<0) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0);
  }

  return rv;
}



int LC_CardServer_Work(LC_CARDSERVER *cs) {
  LC_DRIVER *d;
  LC_SERVICE *as;
  LC_READER *r;
  int rv;
  int somethingDone;

  somethingDone=0;

  rv=GWEN_IPCManager_Work(cs->ipcManager, 10);
  if (rv==-1) {
    DBG_ERROR(0, "Error in IPC manager");
    return -1;
  }
  else if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Handling commands");
  rv=LC_CardServer_HandleNextCommand(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking clients");
  rv=LC_CardServer_CheckClients(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking drivers");
  d=LC_Driver_List_First(cs->drivers);
  while(d) {
    LC_DRIVER *next;

    next=LC_Driver_List_Next(d);
    rv=LC_CardServer_CheckDriver(cs, d);
    if (rv==0) {
      somethingDone++;
    }
    d=next;
  } /* while */

  DBG_DEBUG(0, "Checking services");
  as=LC_Service_List_First(cs->services);
  while(as) {
    rv=LC_CardServer_CheckService(cs, as);
    if (rv==0) {
      somethingDone++;
    }
    as=LC_Service_List_Next(as);
  } /* while */

  if (!cs->disableAutoconf) {
    DBG_DEBUG(0, "Scanning USB");
    if (LC_CardServer_ScanUSB(cs)) {
      DBG_INFO(0, "here");
    }
  }

  DBG_DEBUG(0, "Checking readers");
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    LC_READER *rnext;

    rnext=LC_Reader_List_Next(r);
    rv=LC_CardServer_CheckReader(cs, r);
    if (rv==0) {
      somethingDone++;
    }
    r=rnext;
  } /* while */

  DBG_DEBUG(0, "Checking cards");
  rv=LC_CardServer_CheckCards(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking requests");
  rv=LC_CardServer_CheckRequests(cs);
  if (rv==0) {
    somethingDone++;
  }

  rv=GWEN_IPCManager_Work(cs->ipcManager, 10);
  if (rv==-1) {
    DBG_ERROR(0, "Error in IPC manager");
    return -1;
  }
  else if (rv==0) {
    somethingDone++;
  }

  return somethingDone?0:1;
}



GWEN_TYPE_UINT32 LC_CardServer_GetIpcId(const LC_CARDSERVER *cs){
  assert(cs);
  return cs->ipcId;
}



void LC_CardServer_SetIpcId(LC_CARDSERVER *cs, GWEN_TYPE_UINT32 id){
  assert(cs);
  cs->ipcId=id;
}









void LC_CardServer_SampleDirs(LC_CARDSERVER *cs,
                              GWEN_STRINGLIST *sl) {
  return LC_DriverInfo_SampleDirs(cs->dataDir, sl);
}



GWEN_TYPE_UINT32 LC_CardServer_GetNotificationMask(const char *ntype,
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



int LC_CardServer__SendNotification(LC_CARDSERVER *cs,
                                    const LC_CLIENT *cl,
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
           LC_Client_GetClientId(cl));

  dbReq=GWEN_DB_Group_new("Notification");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Client_GetClientId(cl));
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
  rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                  LC_Client_GetClientId(cl),
                                  dbReq);
  if (rid==0) {
    DBG_INFO(0, "here");
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);

  /* done */
  return 0;
}



int LC_CardServer_SendNotification(LC_CARDSERVER *cs,
                                   LC_CLIENT *cl,
                                   const char *ntype,
                                   const char *ncode,
                                   GWEN_DB_NODE *dbData){
  int rv;
  GWEN_TYPE_UINT32 mask;
  int err;
  int clients;

  assert(ntype);
  assert(ncode);

  DBG_INFO(0, "Sending notification %s:%s to all clients",
           ntype, ncode);

  mask=LC_CardServer_GetNotificationMask(ntype, ncode);
  if (!mask) {
    DBG_ERROR(0, "here");
    return -1;
  }

  DBG_DEBUG(0, "Mask for %s:%s is %08x", ntype, ncode, mask);

  err=0;
  clients=0;
  if (cl!=0) {
    if (LC_Client_GetNotifyFlags(cl) & mask) {
      clients++;
      rv=LC_CardServer__SendNotification(cs, cl, ntype, ncode, dbData);
      if (rv) {
	DBG_ERROR(0, "Error sending request to client \"%08x\"",
		  LC_Client_GetClientId(cl));
	err++;
      }
    }
    else {
      DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s",
		LC_Client_GetClientId(cl), ntype, ncode);
    }
  }
  else {
    cl=LC_Client_List_First(cs->clients);
    while(cl) {
      if (LC_Client_GetNotifyFlags(cl) & mask) {
	clients++;
	rv=LC_CardServer__SendNotification(cs, cl, ntype, ncode, dbData);
	if (rv) {
	  DBG_ERROR(0, "Error sending request to client \"%08x\"",
		    LC_Client_GetClientId(cl));
	  err++;
	}
      }
      else {
	DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s",
		  LC_Client_GetClientId(cl), ntype, ncode);
      }
      cl=LC_Client_List_Next(cl);
    }
  }

  if (clients && (clients==err)) {
    DBG_ERROR(0, "Could not send notification to any client");
    return -1;
  }

  /* done */
  return 0;
}






void LC_CardServer_DumpState(LC_CARDSERVER *cs) {
  LC_DRIVER *d;
  LC_READER *r;
  LC_CARD *cd;

  fprintf(stderr, "Card-Server Status:\n");
  fprintf(stderr, "  Drivers:\n");
  d=LC_Driver_List_First(cs->drivers);
  while(d) {
    LC_Driver_Dump(d, stderr, 4);
    d=LC_Driver_List_Next(d);
  }

  fprintf(stderr, "  Readers:\n");
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    LC_Reader_Dump(r, stderr, 4);
    r=LC_Reader_List_Next(r);
  }

  fprintf(stderr, " Active Cards:\n");
  cd=LC_Card_List_First(cs->activeCards);
  while(cd) {
    LC_Card_Dump(cd, stderr, 4);
    cd=LC_Card_List_Next(cd);
  }

  fprintf(stderr, " Free Cards:\n");
  cd=LC_Card_List_First(cs->freeCards);
  while(cd) {
    LC_Card_Dump(cd, stderr, 4);
    cd=LC_Card_List_Next(cd);
  }

  fprintf(stderr, "  IPC:\n");
  GWEN_IPCManager_Dump(cs->ipcManager, stderr, 2);
}



int LC_CardServer_GetClientCount(const LC_CARDSERVER *cs){
  assert(cs);
  return LC_Client_List_GetCount(cs->clients);
}




