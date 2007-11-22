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


#include "servicemanager_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/text.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif




LCSV_SERVICEMANAGER *LCSV_ServiceManager_new(LCS_SERVER *server) {
  LCSV_SERVICEMANAGER *svm;

  GWEN_NEW_OBJECT(LCSV_SERVICEMANAGER, svm);
  DBG_MEM_INC("LCSV_SERVICEMANAGER", 0);
  svm->server=server;
  svm->ipcManager=LCS_Server_GetIpcManager(server);
  svm->services=LCSV_Service_List_new();

  return svm;
}



void LCSV_ServiceManager_free(LCSV_SERVICEMANAGER *svm) {
  if (svm) {
    LCSV_Service_List_free(svm->services);
    free(svm->addrAddrForServices);
    free(svm->addrTypeForServices);
    DBG_MEM_DEC("LCSV_SERVICEMANAGER");
    GWEN_FREE_OBJECT(svm);
  }
}



int LCSV_ServiceManager_Init(LCSV_SERVICEMANAGER *svm, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *dbT;
  const char *p;

  DBG_INFO(0, "Initialising service manager");
  assert(svm);

  /* preset with reasonable values */
  svm->allowClientService=0;
  svm->serviceStartDelay=LCSV_SERVICEMANAGER_DEF_SERVICE_START_DELAY;
  svm->serviceStartTimeout=LCSV_SERVICEMANAGER_DEF_SERVICE_START_TIMEOUT;
  svm->serviceStopTimeout=LCSV_SERVICEMANAGER_DEF_SERVICE_STOP_TIMEOUT;
  svm->serviceRestartTime=LCSV_SERVICEMANAGER_DEF_SERVICE_RESTART_TIME;
  svm->serviceIdleTimeout=LCSV_SERVICEMANAGER_DEF_SERVICE_IDLE_TIMEOUT;

  /* read configuration file */
  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "ServiceManager");
  if (dbT==0)
    dbT=db;
  if (dbT) {
    svm->allowClientService=GWEN_DB_GetIntValue(dbT, "allowClient", 0, 0);

    /* read some timeout values */
#define LCSV_SVM_INIT_TIME(s) \
  svm->s=GWEN_DB_GetIntValue(dbT, __STRING(s), 0, svm->s);
    LCSV_SVM_INIT_TIME(serviceStartDelay)
    LCSV_SVM_INIT_TIME(serviceStartTimeout)
    LCSV_SVM_INIT_TIME(serviceStopTimeout)
    LCSV_SVM_INIT_TIME(serviceRestartTime)
    LCSV_SVM_INIT_TIME(serviceIdleTimeout)
#undef LCSV_SVM_INIT_TIME
  }

  /* find config of server to be used for services */
  dbT=GWEN_DB_FindFirstGroup(db, "server");
  while(dbT) {
    if (GWEN_DB_GetIntValue(dbT, "useForServices", 0, 0)!=0)
      break;
    dbT=GWEN_DB_FindNextGroup(dbT, "server");
  }
  if (!dbT)
    dbT=GWEN_DB_FindFirstGroup(db, "server");

  assert(dbT);

  p=GWEN_DB_GetCharValue(dbT, "typ", 0, "local");
  assert(p);
  svm->addrTypeForServices=strdup(p);

  p=GWEN_DB_GetCharValue(dbT, "addr", 0, 0);
  assert(p);
  svm->addrAddrForServices=strdup(p);

  svm->addrPortForServices=GWEN_DB_GetIntValue(dbT, "port", 0, 0);

  /* read services */
  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "ServiceManager");
  if (dbT==0)
    dbT=db;

  dbT=GWEN_DB_FindFirstGroup(dbT, "service");
  while(dbT) {
    LCSV_SERVICE *sv;
    const char *p;

    /* service section found, create a service */
    sv=LCSV_Service_fromDb(dbT);
    assert(sv);

    p=LCSV_Service_GetServiceName(sv);
    if (!p || !*p) {
      GWEN_BUFFER *tbuf;
      char numbuf[16];

      tbuf=GWEN_Buffer_new(0, 256, 0, 1);
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LCSV_Service_GetServiceId(sv));
      GWEN_Buffer_AppendString(tbuf, "service-");
      GWEN_Buffer_AppendString(tbuf, numbuf);
      LCSV_Service_SetServiceName(sv, GWEN_Buffer_GetStart(tbuf));
      GWEN_Buffer_free(tbuf);
    }

    p=LCSV_Service_GetLogFile(sv);
    if (!p || !*p) {
      GWEN_BUFFER *lbuf;
      GWEN_STRINGLIST *sl;
      const char *s;

      sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                   LCS_PATH_SERVER_LOGDIR);
      assert(sl);
      s=GWEN_StringList_FirstString(sl);
      assert(s);
      lbuf=GWEN_Buffer_new(0, 256, 0, 1);

      GWEN_Buffer_AppendString(lbuf, s);
      GWEN_Buffer_AppendString(lbuf, DIRSEP"services"DIRSEP);
      GWEN_Buffer_AppendString(lbuf, LCSV_Service_GetServiceType(sv));
      GWEN_Buffer_AppendString(lbuf, "-");
      GWEN_Buffer_AppendString(lbuf, LCSV_Service_GetServiceName(sv));
      GWEN_Buffer_AppendString(lbuf, ".log");
      LCSV_Service_SetLogFile(sv, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
    }

    DBG_INFO(0, "Adding service \"%s\"", LCSV_Service_GetServiceName(sv));
    LCSV_Service_List_Add(sv, svm->services);

    dbT=GWEN_DB_FindNextGroup(dbT, "service");
  }

  return 0;
}



int LCSV_ServiceManager_Fini(LCSV_SERVICEMANAGER *svm, GWEN_DB_NODE *db) {
  assert(svm);
  LCSV_Service_List_Clear(svm->services);
  return 0;
}



void LCSV_ServiceManager_AbandonService(LCSV_SERVICEMANAGER *svm,
                                        LCSV_SERVICE *sv,
                                        LC_SERVICE_STATUS newSt,
                                        const char *reason) {
  LCSV_Service_SetStatus(sv, newSt);
  if (newSt==LC_ServiceStatusAborted)
    LCSV_Service_SetTimeout(sv, svm->serviceRestartTime);
  else
    LCSV_Service_SetTimeout(sv, 0);

  LCS_Server_ServiceChg(svm->server,
                        LCSV_Service_GetServiceId(sv),
                        LCSV_Service_GetServiceType(sv),
                        LCSV_Service_GetServiceName(sv),
                        newSt, reason);
}



uint32_t LCSV_ServiceManager_SendStopService(LCSV_SERVICEMANAGER *svm,
					     const LCSV_SERVICE *sv){
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  uint32_t rid;

  assert(svm);
  assert(sv);
  dbReq=GWEN_DB_Group_new("StopService");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCSV_Service_GetServiceId(sv));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);

  rv=GWEN_IpcManager_SendRequest(svm->ipcManager,
				 LCSV_Service_GetIpcId(sv),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



int LCSV_ServiceManager_StartService(LCSV_SERVICEMANAGER *svm,
                                     LCSV_SERVICE *sv) {
  GWEN_PROCESS *p=0;
  GWEN_BUFFER *pbuf=0;
  GWEN_BUFFER *abuf=0;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;
  GWEN_PLUGIN_MANAGER *pm=0;

  assert(svm);
  assert(sv);

  DBG_INFO(0, "Starting service \"%s\"",
           LCSV_Service_GetServiceName(sv));

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LCSV_Service_GetDataDir(sv);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LCSV_Service_GetLogFile(sv);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=getenv("LCSV_SERVICE_LOGLEVEL");
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --loglevel ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  if (svm->addrTypeForServices) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, svm->addrTypeForServices);
  }

  if (svm->addrAddrForServices) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, svm->addrAddrForServices);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", svm->addrPortForServices);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCSV_Service_GetServiceId(sv));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LCSV_Service_GetServiceType(sv);
  if (!s) {
    DBG_ERROR(0, "No service type");
    LCSV_ServiceManager_AbandonService(svm, sv,
                                       LC_ServiceStatusAborted,
                                       "No service type");
    GWEN_Buffer_free(abuf);
    return -1;
  }

  /* add configuration folder */
  GWEN_Buffer_AppendString(abuf,
			   " -C "
			   LC_DEFAULT_CONFDIR
			   DIRSEP
			   "services"
			   DIRSEP);
  GWEN_Buffer_AppendString(abuf, s);

  /* get driver path by loading its plugin description */
  pm=GWEN_PluginManager_FindPluginManager(LCS_PLUGIN_SERVICE);
  if (!pm) {
    DBG_ERROR(0, "Plugin manager \"%s\" not found",
              LCS_PLUGIN_SERVICE);
    GWEN_Buffer_free(abuf);
    abort();
  }
  else {
    GWEN_PLUGIN_DESCRIPTION *pd;
    const char *p;

    pd=GWEN_PluginManager_GetPluginDescr(pm, s);
    if (!pd) {
      DBG_ERROR(0, "Plugin description for driver \"%s\" not found", s);
      LCSV_ServiceManager_AbandonService(svm, sv,
                                       LC_ServiceStatusAborted,
                                       "No plugin description for driver");
      GWEN_Buffer_free(abuf);
      return -1;
    }
    p=GWEN_PluginDescription_GetPath(pd);
    assert(p);
    pbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(pbuf, p);
    GWEN_Buffer_AppendString(pbuf, DIRSEP);
    GWEN_Buffer_AppendString(pbuf, s);
    GWEN_PluginDescription_free(pd);
  }

  p=GWEN_Process_new();
  DBG_INFO(0, "Starting process for service \"%s\" (%s)",
           LCSV_Service_GetServiceName(sv), GWEN_Buffer_GetStart(pbuf));
  DBG_INFO(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

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
    LCSV_ServiceManager_AbandonService(svm, sv,
                                       LC_ServiceStatusAborted,
                                       "Unable to execute service");
    return -1;
  }

  /* process started */
  DBG_INFO(0, "Process started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LCSV_Service_SetProcess(sv, p);
  LCSV_Service_SetStatus(sv, LC_ServiceStatusStarted);
  /* notify callback */
  LCS_Server_ServiceChg(svm->server,
                        LCSV_Service_GetServiceId(sv),
                        LCSV_Service_GetServiceType(sv),
                        LCSV_Service_GetServiceName(sv),
                        LC_ServiceStatusStarted,
                        "Service started");
  /* done */
  return 0;
}



int LCSV_ServiceManager_CheckService(LCSV_SERVICEMANAGER *svm,
                                     LCSV_SERVICE *sv) {
  int done=0;
  uint32_t nid;
  LC_SERVICE_STATUS st;
  uint32_t sflags;

  assert(svm);
  assert(sv);

  nid=LCSV_Service_GetIpcId(sv);
  st=LCSV_Service_GetStatus(sv);

  sflags=LCSV_Service_GetFlags(sv);

  DBG_DEBUG(0, "Checking service %s (%d)",
            LCSV_Service_GetServiceName(sv), st);

  if ((sflags & LC_SERVICE_FLAGS_CLIENT) &&
      (st==LC_ServiceStatusAborted ||
       st==LC_ServiceStatusDown ||
       st==LC_ServiceStatusDisabled)
     ) {
    DBG_NOTICE(0, "Service \"%s\" is unused, removing it",
               LCSV_Service_GetServiceName(sv));
    LCSV_Service_List_Del(sv);
    LCSV_Service_free(sv);
    return 1;
  }

  if (st==LC_ServiceStatusAborted) {
    if (LCSV_Service_CheckTimeout(sv)) {
      DBG_NOTICE(0, "Reenabling service \"%s\"",
                 LCSV_Service_GetServiceName(sv));
      st=LC_ServiceStatusDown;
      LCSV_Service_SetStatus(sv, st);
      LCS_Server_ServiceChg(svm->server,
                            LCSV_Service_GetServiceId(sv),
                            LCSV_Service_GetServiceType(sv),
                            LCSV_Service_GetServiceName(sv),
                            st,
                            "Reenabling service");
      done++;
    }
  } /* if aborted */

  if (st==LC_ServiceStatusWaitForStart) {
    if (LCSV_Service_CheckTimeout(sv)) {
      int rv;

      rv=LCSV_ServiceManager_StartService(svm, sv);
      if (rv) {
        DBG_INFO(0, "here (%d)", rv);
        return 1;
      }
      st=LCSV_Service_GetStatus(sv);
      LCSV_Service_SetTimeout(sv, svm->serviceStartTimeout);
      done++;
    }
  }

  if (st==LC_ServiceStatusStopping) {
    GWEN_PROCESS *p;

    p=LCSV_Service_GetProcess(sv);
    if (p) {
      GWEN_PROCESS_STATE pst;

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (LCSV_Service_CheckTimeout(sv)==1) {
          DBG_WARN(0, "Service is still running, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Service still running, "
                                           "killing it");
          done++;
        }
        else {
          /* otherwise give the process a little bit time ... */
          DBG_DEBUG(0, "still waiting for service to go down");
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Service terminated normally");
        LCSV_Service_SetProcess(sv, 0);
        st=LC_ServiceStatusDown;
        LCSV_Service_SetStatus(sv, st);
        LCS_Server_ServiceChg(svm->server,
                              LCSV_Service_GetServiceId(sv),
                              LCSV_Service_GetServiceType(sv),
                              LCSV_Service_GetServiceName(sv),
                              st,
                              "Service terminated normally");
        done++;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Service terminated abnormally");
        LCSV_Service_SetProcess(sv, 0);
        st=LC_ServiceStatusAborted;
        LCSV_ServiceManager_AbandonService(svm, sv, st,
                                         "Service terminated abnormally");
        done++;;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Service has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCSV_Service_SetProcess(sv, 0);
        st=LC_ServiceStatusAborted;
        LCSV_ServiceManager_AbandonService(svm, sv, st,
                                         "Service has been stopped,"
                                         "killing it");
        done++;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCSV_Service_SetProcess(sv, 0);
        st=LC_ServiceStatusAborted;
        LCSV_ServiceManager_AbandonService(svm, sv, st,
                                         "Unknown process status, "
                                         "killing it");
        done++;
      }
    } /* if process */
    else {
      if (!(sflags & LC_SERVICE_FLAGS_CLIENT)) {
        DBG_ERROR(0, "No process for local service:");
        abort();
      }
    }
  } /* if stopping */

  if (st==LC_ServiceStatusStarted) {
    /* service started, check timeout */
    if (LCSV_Service_CheckTimeout(sv)==1) {
      GWEN_PROCESS *p;

      DBG_WARN(0, "Service \"%s\" timed out", LCSV_Service_GetServiceName(sv));
      p=LCSV_Service_GetProcess(sv);
      if (p) {
        GWEN_PROCESS_STATE pst;

        pst=GWEN_Process_CheckState(p);
        if (pst==GWEN_ProcessStateRunning) {
          DBG_WARN(0,
                   "Service is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Service is running but did not "
                                           "signal readyness, "
                                           "killing it");
          done++;
        }
        else if (pst==GWEN_ProcessStateExited) {
          DBG_WARN(0, "Service terminated without signalling readyness");
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Service terminated "
                                           "without signalling readyness");
          done++;
        }
        else if (pst==GWEN_ProcessStateAborted) {
          DBG_WARN(0, "Service terminated abnormally");
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Service terminated abnormally");
          done++;
        }
        else if (pst==GWEN_ProcessStateStopped) {
          DBG_WARN(0, "Service has been stopped, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Service has been stopped, "
                                           "killing it");
          done++;
        }
        else {
          DBG_ERROR(0, "Unknown process status %d, killing", pst);
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCSV_Service_SetProcess(sv, 0);
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                           "Unknown process status, "
                                           "killing");
          done++;
        }
      } /* if process */
      else {
        if (!(sflags & LC_SERVICE_FLAGS_CLIENT)) {
          DBG_ERROR(0, "No process for local service:");
          abort();
        }
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for service start");
    }
  }

  if (st==LC_ServiceStatusUp) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;

    /* check whether the service really is still up and running */
    p=LCSV_Service_GetProcess(sv);
    if (p) {
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Service is not running anymore");
        GWEN_Process_Terminate(p);
        LCSV_Service_SetProcess(sv, 0);
        st=LC_ServiceStatusAborted;
        LCSV_ServiceManager_AbandonService(svm, sv, st,
                                         "Service is not running anymore");
        done++;
      }
    } /* if process */
    else {
      if (!(sflags & LC_SERVICE_FLAGS_CLIENT)) {
        DBG_ERROR(0, "No process for local service:");
        abort();
      }
    }

    DBG_DEBUG(0, "Service still running");
    if (svm->serviceIdleTimeout &&
        !(sflags & LC_SERVICE_FLAGS_AUTOLOAD) &&
        !(sflags & LC_SERVICE_FLAGS_CLIENT) &&
        LCSV_Service_GetInterestedClients(sv)==0) {
      time_t t;

      /* check for idle timeout */
      t=LCSV_Service_GetIdleSince(sv);
      assert(t);

      if (difftime(time(0), t)>svm->serviceIdleTimeout) {
        uint32_t rid;

        DBG_NOTICE(0, "Service \"%s\" is too long idle, stopping it",
                   LCSV_Service_GetServiceName(sv));

        assert(sv);
        rid=LCSV_ServiceManager_SendStopService(svm, sv);
        if (!rid) {
          DBG_ERROR(0, "Could not send StopService command for service \"%s\"",
                    LCSV_Service_GetServiceName(sv));
          st=LC_ServiceStatusAborted;
          LCSV_ServiceManager_AbandonService(svm, sv, st,
                                             "Could not send StopService "
                                             "command");
          done++;
        }
        else {
          DBG_DEBUG(0, "Sent StopService request for service \"%s\"",
                    LCSV_Service_GetServiceName(sv));
          st=LC_ServiceStatusStopping;
          LCSV_Service_SetStatus(sv, st);
          LCS_Server_ServiceChg(svm->server,
                                LCSV_Service_GetServiceId(sv),
                                LCSV_Service_GetServiceType(sv),
                                LCSV_Service_GetServiceName(sv),
                                st,
                                "Stopping service");
          LCSV_Service_SetTimeout(sv, svm->serviceStopTimeout);
          done++;
        }
      } /* if timeout */
      /* otherwise service is not idle */
    }
  }

  if (st==LC_ServiceStatusDown) {
    if (LCSV_Service_GetInterestedClients(sv) ||
        (sflags & LC_SERVICE_FLAGS_AUTOLOAD)) {
      LCSV_Service_SetTimeout(sv, svm->serviceStartDelay);
      st=LC_ServiceStatusWaitForStart;
      LCSV_Service_SetStatus(sv, st);
      LCS_Server_ServiceChg(svm->server,
                            LCSV_Service_GetServiceId(sv),
                            LCSV_Service_GetServiceType(sv),
                            LCSV_Service_GetServiceName(sv),
                            st,
                            "Initiating service start");
      done++;
    }
  }

  return (done!=0);
}



int LCSV_ServiceManager_CheckServices(LCSV_SERVICEMANAGER *svm) {
  int done=0;
  LCSV_SERVICE *sv;

  assert(svm);
  sv=LCSV_Service_List_First(svm->services);
  while(sv) {
    int rv;

    rv=LCSV_ServiceManager_CheckService(svm, sv);
    if (rv!=0)
      done++;
    sv=LCSV_Service_List_Next(sv);
  }

  if (done)
    return 1;
  return 0;
}



int LCSV_ServiceManager_Work(LCSV_SERVICEMANAGER *svm) {
  int rv;
  int done=0;

  for (;;) {
    rv=LCSV_ServiceManager_CheckServices(svm);
    if (rv!=0)
      done++;
    else
      break;
  }
  if (done)
    return 1;
  return 0;
}



int LCSV_ServiceManager_ListServices(LCSV_SERVICEMANAGER *svm) {
  LCSV_SERVICE *sv;

  assert(svm);
  sv=LCSV_Service_List_First(svm->services);
  while(sv) {
    LCS_Server_ServiceChg(svm->server,
                          LCSV_Service_GetServiceId(sv),
                          LCSV_Service_GetServiceType(sv),
                          LCSV_Service_GetServiceName(sv),
                          LCSV_Service_GetStatus(sv),
                          "Service listing");
    sv=LCSV_Service_List_Next(sv);
  }

  return 0;
}



void LCSV_ServiceManager_ConnectionDown(LCSV_SERVICEMANAGER *svm,
                                        uint32_t ipcId) {
  LCSV_SERVICE *sv;

  assert(svm);
  sv=LCSV_Service_List_First(svm->services);
  while(sv) {

    if (LCSV_Service_GetIpcId(sv)==ipcId) {
      DBG_INFO(0, "Connection for service \"%08x\" (%s/%s) lost",
               LCSV_Service_GetServiceId(sv),
               LCSV_Service_GetServiceType(sv),
               LCSV_Service_GetServiceName(sv));
      LCSV_ServiceManager_AbandonService(svm, sv,
                                         LC_ServiceStatusAborted,
                                         "Connection broken");
    }

    sv=LCSV_Service_List_Next(sv);
  }
}



int LCSV_ServiceManager_HandleRequest(LCSV_SERVICEMANAGER *svm,
                                      uint32_t rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq) {
  int rv;

  assert(svm);
  assert(name);

  if (strcasecmp(name, "ServiceReady")==0) {
    rv=LCSV_ServiceManager_HandleServiceReady(svm, rid, dbReq);
  }
  /* Insert more handlers here */
  else {
    DBG_DEBUG(0, "Command \"%s\" not handled by service manager", name);
    rv=1; /* not handled */
  }

  return rv;
}



int LCSV_ServiceManager_HandleServiceReady(LCSV_SERVICEMANAGER *svm,
                                           uint32_t rid,
                                           GWEN_DB_NODE *dbReq) {
  GWEN_DB_NODE *dbRsp;
  uint32_t serviceId;
  uint32_t nodeId;
  LCSV_SERVICE *sv;
  const char *text;
  const char *p;
  int i;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Service %08x: ServiceReady", nodeId);

  p=GWEN_DB_GetCharValue(dbReq, "data/service/serviceId", 0, 0);
  if (!p || !*p)
    p=GWEN_DB_GetCharValue(dbReq, "data/serviceId", 0, "0");

  if (1!=sscanf(p, "%x", &i)) {
    DBG_ERROR(0, "Invalid service id (%s)", p);
    LCS_Server_SendErrorResponse(svm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid or missing service id");
    return -1;
  }
  serviceId=i;
  if (serviceId==0 && svm->allowClientService==0) {
    DBG_ERROR(0, "Invalid service id, client services not allowed");
    LCS_Server_SendErrorResponse(svm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid service id, "
                                 "client services not allowed");
    if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* service ready */
  /* find service */
  if (serviceId) {
    sv=LCSV_Service_List_First(svm->services);
    while(sv) {
      if (LCSV_Service_GetServiceId(sv)==serviceId)
        break;
      sv=LCSV_Service_List_Next(sv);
    } /* while */

    if (!sv) {
      DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
      LCS_Server_SendErrorResponse(svm->server, rid,
                                   LC_ERROR_INVALID,
                                   "Service not found");
      if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }
  }
  else {
    const char *stype;
    const char *sname;
    GWEN_DB_NODE *dbService;

    /* service with id=0, must be a remote service */
    dbService=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                               "data/service");
    if (!dbService) {
      DBG_ERROR(0, "No service group given in request");
      LCS_Server_SendErrorResponse(svm->server, rid,
                                   LC_ERROR_INVALID,
                                   "No service description");
      if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    sname=GWEN_DB_GetCharValue(dbReq, "data/service/serviceName", 0, 0);
    stype=GWEN_DB_GetCharValue(dbReq, "data/service/serviceType", 0, 0);
    if (!stype) {
      DBG_ERROR(0, "No service type given in remote service");
      LCS_Server_SendErrorResponse(svm->server, rid,
                                   LC_ERROR_INVALID,
                                   "No service type");
      if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    /* create service from DB */
    sv=LCSV_Service_fromDb(dbService);
    assert(sv);
    serviceId=LCSV_Service_GetServiceId(sv);
    LCSV_Service_SubFlags(sv, LC_SERVICE_FLAGS_RUNTIME_MASK);
    LCSV_Service_AddFlags(sv, LC_SERVICE_FLAGS_CLIENT);

    if (sname==0 || *sname==0) {
      GWEN_BUFFER *tbuf;
      char numbuf[16];

      tbuf=GWEN_Buffer_new(0, 256, 0, 1);
      snprintf(numbuf, sizeof(numbuf)-1, "%08x", serviceId);
      GWEN_Buffer_AppendString(tbuf, "autoservice-");
      GWEN_Buffer_AppendString(tbuf, numbuf);
      LCSV_Service_SetServiceName(sv, GWEN_Buffer_GetStart(tbuf));
      GWEN_Buffer_free(tbuf);
    }

    /* add service to list */
    DBG_NOTICE(0, "Adding client service \"%s\" (%s)",
               LCSV_Service_GetServiceName(sv), stype);
    LCSV_Service_List_Add(sv, svm->services);
  } /* if service does not exist */

  LCSV_Service_SetIpcId(sv, nodeId);

  /* check code */
  text=GWEN_DB_GetCharValue(dbReq, "data/text", 0, "Service up");

  DBG_NOTICE(0, "Service \"%08x\" is up (%s)", serviceId, text);
  LCSV_Service_SetStatus(sv, LC_ServiceStatusUp);
  LCS_Server_ServiceChg(svm->server,
                        LCSV_Service_GetServiceId(sv),
                        LCSV_Service_GetServiceType(sv),
                        LCSV_Service_GetServiceName(sv),
                        LC_ServiceStatusUp,
                        text);

  dbRsp=GWEN_DB_Group_new("ServiceReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Service registered");
  if (GWEN_IpcManager_SendResponse(svm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (GWEN_IpcManager_RemoveRequest(svm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCSV_ServiceManager_GetMatchingServices(LCSV_SERVICEMANAGER *svm,
                                            const char *serviceType,
                                            const char *serviceName,
                                            GWEN_DB_NODE *dbData) {
  LCSV_SERVICE *sv;
  int count=0;

  assert(svm);
  assert(serviceType);
  assert(serviceName);

  sv=LCSV_Service_List_First(svm->services);
  while(sv) {
    const char *sname;
    const char *stype;

    sname=LCSV_Service_GetServiceName(sv);
    assert(sname);
    stype=LCSV_Service_GetServiceType(sv);
    assert(stype);

    if (-1!=GWEN_Text_ComparePattern(stype, serviceType, 0) &&
        -1!=GWEN_Text_ComparePattern(sname, serviceName, 0)) {
      GWEN_DB_NODE *dbService;

      dbService=GWEN_DB_GetGroup(dbData, GWEN_PATH_FLAGS_CREATE_GROUP,
                                 "service");
      assert(dbService);
      LCSV_Service_toDb(sv, dbService);
      count++;
    }

    sv=LCSV_Service_List_Next(sv);
  }

  return count;
}



uint32_t LCSV_ServiceManager_SendCommand(LCSV_SERVICEMANAGER *svm,
					 uint32_t serviceId,
					 GWEN_DB_NODE *dbCmd) {
  LCSV_SERVICE *sv;
  char numbuf[16];
  int rv;
  uint32_t rid;

  assert(svm);

  sv=LCSV_Service_List_First(svm->services);
  while(sv) {
    if (LCSV_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LCSV_Service_List_Next(sv);
  }

  if (!sv) {
    DBG_ERROR(0, "Service not found");
    return 0;
  }

  if (LCSV_Service_GetStatus(sv)!=LC_ServiceStatusUp) {
    DBG_ERROR(0, "Bad service status (%d)", LCSV_Service_GetStatus(sv));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LCSV_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);

  rv=GWEN_IpcManager_SendRequest(svm->ipcManager,
				 LCSV_Service_GetIpcId(sv),
				 dbCmd,
				 &rid);
  if (rv<0) {
    DBG_ERROR(0, "Could not send request (%d)", rv);
    return 0;
  }

  return rid;
}











