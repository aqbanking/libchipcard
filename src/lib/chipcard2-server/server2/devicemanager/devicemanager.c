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


#include "devicemanager_p.h"
#include "server_l.h"
#include "dm_card_l.h"
#include <chipcard2-server/common/driverinfo.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/plugin.h>
#include <gwenhywfar/ipc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif


GWEN_INHERIT_FUNCTIONS(LCDM_DEVICEMANAGER)


LCDM_DEVICEMANAGER *LCDM_DeviceManager_new(LCS_SERVER *server) {
  LCDM_DEVICEMANAGER *dm;

  GWEN_NEW_OBJECT(LCDM_DEVICEMANAGER, dm);
  GWEN_INHERIT_INIT(LCDM_DEVICEMANAGER, dm);
  dm->server=server;
  dm->ipcManager=LCS_Server_GetIpcManager(server);

  dm->drivers=LCDM_Driver_List_new();
  dm->readers=LCDM_Reader_List_new();
  dm->dbDrivers=GWEN_DB_Group_new("drivers");
  dm->dbConfigDrivers=GWEN_DB_Group_new("configDrivers");


  return dm;
}



void LCDM_DeviceManager_free(LCDM_DEVICEMANAGER *dm) {
  if (dm) {
    GWEN_INHERIT_FINI(LCDM_DEVICEMANAGER, dm);
    free(dm->addrTypeForDrivers);
    free(dm->addrAddrForDrivers);
    GWEN_DB_Group_free(dm->dbDrivers);
    LCDM_Reader_List_free(dm->readers);
    LCDM_Driver_List_free(dm->drivers);

    GWEN_FREE_OBJECT(dm);
  }
}



int LCDM_DeviceManager_Init(LCDM_DEVICEMANAGER *dm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbT;
  const char *p;

  DBG_INFO(0, "Initializing device manager");
  assert(dm);
  GWEN_DB_ClearGroup(dm->dbDrivers, 0);

  /* preset with reasonable values */
  dm->allowRemote=0;
  dm->driverStartDelay=LCDM_DEVICEMANAGER_DEF_DRIVER_START_DELAY;
  dm->driverStartTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_START_TIMEOUT;
  dm->driverStopTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_STOP_TIMEOUT;
  dm->driverRestartTime=LCDM_DEVICEMANAGER_DEF_DRIVER_RESTART_TIME;
  dm->driverIdleTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_IDLE_TIMEOUT;

  dm->readerStartDelay=LCDM_DEVICEMANAGER_DEF_READER_START_DELAY;
  dm->readerStartTimeout=LCDM_DEVICEMANAGER_DEF_READER_START_TIMEOUT;
  dm->readerRestartTime=LCDM_DEVICEMANAGER_DEF_READER_RESTART_TIME;
  dm->readerIdleTimeout=LCDM_DEVICEMANAGER_DEF_READER_IDLE_TIMEOUT;
  dm->readerCommandTimeout=LCDM_DEVICEMANAGER_DEF_READER_COMMAND_TIMEOUT;

  /* read configuration file */
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "DeviceManager");
  if (dbT==0) {
    DBG_WARN(0,
             "Your configuration does not have a \"DeviceManager\" group. "
             "Please update the file.");
    dbT=dbConfig;
  }
  if (dbT) {
    dm->allowRemote=GWEN_DB_GetIntValue(dbT, "allowRemote", 0, 0);

    /* read some timeout values */
#define LCDM_DM_INIT_TIME(s) \
  dm->s=GWEN_DB_GetIntValue(dbT, __STRING(s), 0, dm->s);
    LCDM_DM_INIT_TIME(driverStartDelay)
    LCDM_DM_INIT_TIME(driverStartTimeout)
    LCDM_DM_INIT_TIME(driverStopTimeout)
    LCDM_DM_INIT_TIME(driverRestartTime)
    LCDM_DM_INIT_TIME(driverIdleTimeout)
    LCDM_DM_INIT_TIME(readerStartDelay)
    LCDM_DM_INIT_TIME(readerStartTimeout)
    LCDM_DM_INIT_TIME(readerRestartTime)
    LCDM_DM_INIT_TIME(readerIdleTimeout)
    LCDM_DM_INIT_TIME(readerCommandTimeout)
#undef LCDM_DM_INIT_TIME
  }

  /* find config of server to be used for drivers */
  dbT=GWEN_DB_FindFirstGroup(dbConfig, "server");
  while(dbT) {
    if (GWEN_DB_GetIntValue(dbT, "useForDrivers", 0, 0)!=0)
      break;
    dbT=GWEN_DB_FindNextGroup(dbT, "server");
  }
  if (!dbT)
    dbT=GWEN_DB_FindFirstGroup(dbConfig, "server");

  assert(dbT);

  p=GWEN_DB_GetCharValue(dbT, "typ", 0, "local");
  assert(p);
  dm->addrTypeForDrivers=strdup(p);

  p=GWEN_DB_GetCharValue(dbT, "addr", 0, 0);
  assert(p);
  dm->addrAddrForDrivers=strdup(p);

  dm->addrPortForDrivers=GWEN_DB_GetIntValue(dbT, "port", 0, 0);

  /* read drivers */
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "DeviceManager");
  if (dbT==0)
    dbT=dbConfig;

  dbT=GWEN_DB_FindFirstGroup(dbT, "driver");
  while(dbT) {
    LCDM_DRIVER *d;
    GWEN_DB_NODE *dbR;

    /* driver section found, add it to global list and create a driver */
    GWEN_DB_InsertGroup(dm->dbDrivers,
                        GWEN_DB_Group_dup(dbT));
    GWEN_DB_AddGroup(dm->dbConfigDrivers,
                     GWEN_DB_Group_dup(dbT));

    d=LCDM_Driver_fromDb(dbT);
    assert(d);

    if (!LCDM_Driver_GetLogFile(d)) {
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
      LCS_Server_ReplaceVar(DIRSEP"drivers"DIRSEP"@driver@"
                            DIRSEP"@reader@"
                            ".log",
                            "driver",
                            LCDM_Driver_GetDriverName(d),
                            lbuf);
      LCDM_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
    }

    DBG_INFO(0, "Adding driver \"%s\"", LCDM_Driver_GetDriverName(d));
    LCDM_Driver_List_Add(d, dm->drivers);

    dbR=GWEN_DB_GetFirstGroup(dbT);
    while(dbR) {
      if (strcasecmp(GWEN_DB_GroupName(dbR), "reader")==0) {
        LCDM_READER *r;

        /* reader section found */
        r=LCDM_Reader_fromDb(d, dbR);
        assert(r);
        LCDM_Driver_IncAssignedReadersCount(d);
        DBG_INFO(0, "Adding reader \"%s\"", LCDM_Reader_GetReaderName(r));
        /* readers from config file are always assumed available */
        LCDM_Reader_SetIsAvailable(r, 1);
        LCDM_Reader_List_Add(r, dm->readers);
      } /* if reader */
      dbR=GWEN_DB_GetNextGroup(dbR);
    } /* while */

    dbT=GWEN_DB_FindNextGroup(dbT, "driver");
  }

  return 0;
}



int LCDM_DeviceManager_Fini(LCDM_DEVICEMANAGER *dm) {

  DBG_INFO(0, "Deinitializing device manager");

  GWEN_DB_ClearGroup(dm->dbDrivers, 0);
  LCDM_Reader_List_Clear(dm->readers);
  LCDM_Driver_List_Clear(dm->drivers);

  return 0;
}



void LCDM_DeviceManager_BeginUseReaders(LCDM_DEVICEMANAGER *dm) {
  LCDM_READER *r;

  assert(dm);
  dm->readerUsage++;
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    LCDM_Reader_IncUsageCount(r, 1);
    r=LCDM_Reader_List_Next(r);
  }
}



void LCDM_DeviceManager_EndUseReaders(LCDM_DEVICEMANAGER *dm, int count) {
  LCDM_READER *r;

  assert(dm);
  assert(dm->readerUsage>=count);
  dm->readerUsage-=count;
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    LCDM_Reader_DecUsageCount(r, count);
    r=LCDM_Reader_List_Next(r);
  }
}






void LCDM_DeviceManager_BeginUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCDM_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_Reader_IncUsageCount(r, 1);
}



void LCDM_DeviceManager_EndUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCDM_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_Reader_DecUsageCount(r, 1);
}



void LCDM_DeviceManager_AbandonReader(LCDM_DEVICEMANAGER *dm,
                                      LCDM_READER *r,
                                      LC_READER_STATUS newSt,
                                      const char *reason) {
  LCDM_DRIVER *d;
  LC_READER_STATUS oldSt;

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  oldSt=LCDM_Reader_GetStatus(r);
  if (oldSt!=LC_ReaderStatusDown &&
      oldSt!=LC_ReaderStatusDisabled &&
      oldSt!=LC_ReaderStatusAborted) {
    DBG_ERROR(0, "Decrementing active reader count");
    LCDM_Driver_DecActiveReadersCount(d, 1);
  }

  LCDM_Reader_SetStatus(r, newSt);
  if (newSt==LC_ReaderStatusAborted)
    LCDM_Reader_SetTimeout(r, dm->readerRestartTime);
  else
    LCDM_Reader_SetTimeout(r, 0);

  LCS_Server_ReaderChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Reader_GetReaderId(r),
                       LCDM_Reader_GetReaderType(r),
                       LCDM_Reader_GetReaderName(r),
                       newSt, reason);
}



void LCDM_DeviceManager_AbandonDriver(LCDM_DEVICEMANAGER *dm,
                                      LCDM_DRIVER *d,
                                      LC_DRIVER_STATUS newSt,
                                      const char *reason) {
  LCDM_READER *r;

  if (LCDM_Driver_GetStatus(d)!= newSt) {
    LCDM_Driver_SetStatus(d, newSt);
    LCS_Server_DriverChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         LCDM_Driver_GetDriverType(d),
                         LCDM_Driver_GetDriverName(d),
                         LCDM_Driver_GetLibraryFile(d),
                         newSt, reason);
    if (newSt==LC_DriverStatusAborted)
      LCDM_Driver_SetTimeout(d, dm->driverRestartTime);
    else
      LCDM_Driver_SetTimeout(d, 0);

    r=LCDM_Reader_List_First(dm->readers);
    while(r) {
      LC_READER_STATUS rst;

      rst=LCDM_Reader_GetStatus(r);
      if (rst!=LC_ReaderStatusDown &&
          rst!=LC_ReaderStatusDisabled &&
          rst!=LC_ReaderStatusAborted) {
        DBG_INFO(0, "Abandoning reader");
        LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted, reason);
      }
      r=LCDM_Reader_List_Next(r);
    }
  }
}



int LCDM_DeviceManager_ReloadDrivers(LCDM_DEVICEMANAGER *dm) {
  GWEN_STRINGLIST *sl;
  GWEN_STRINGLISTENTRY *se;
  GWEN_DB_NODE *dbDrivers;

  DBG_INFO(0, "Reloading driver info files");

  sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                               LCS_PATH_DRIVER_INFODIR);
  assert(sl);

  dbDrivers=GWEN_DB_Group_dup(dm->dbConfigDrivers);

  se=GWEN_StringList_FirstEntry(sl);
  while(se) {
    const char *s;
    int rv;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    /* read all available drivers from folder */
    rv=LC_DriverInfo_ReadDrivers(s, dbDrivers, 1);
    if (rv) {
      DBG_INFO(0, "No driver info in folder \"%s\"", s);
    }
    se=GWEN_StringListEntry_Next(se);
  }

  GWEN_DB_Group_free(dm->dbDrivers);
  dm->dbDrivers=dbDrivers;
  GWEN_StringList_free(sl);
  return 0;
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStartReader(LCDM_DEVICEMANAGER *dm,
						    const LCDM_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;
  int port;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("StartReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCDM_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCDM_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "driversReaderId", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "name", LCDM_Reader_GetReaderName(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "slots", LCDM_Reader_GetSlots(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "flags", LCDM_Reader_GetFlags(r));
  port=LCDM_Reader_GetPort(r);
  if (port==-1) {
    port=LCDM_Driver_GetFirstNewPort(d)+(++(dm->nextNewPort));
    DBG_INFO(0, "Assigning port %d to reader \"%s\"",
	     port, LCDM_Reader_GetReaderName(r));
  }
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "port", port);

  return GWEN_IPCManager_SendRequest(dm->ipcManager,
				     LCDM_Driver_GetIpcId(d),
				     dbReq);
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStopReader(LCDM_DEVICEMANAGER *dm,
						   const LCDM_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  const char *p;
  LCDM_DRIVER *d;

  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("StopReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCDM_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCDM_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  p=LCDM_Reader_GetReaderName(r);
  if (p)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", p);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "port",  LCDM_Reader_GetPort(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slots", LCDM_Reader_GetSlots(r));

  return GWEN_IPCManager_SendRequest(dm->ipcManager,
				     LCDM_Driver_GetIpcId(d),
				     dbReq);
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStopDriver(LCDM_DEVICEMANAGER *dm,
                                                   const LCDM_DRIVER *d) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;

  assert(d);
  dbReq=GWEN_DB_Group_new("StopDriver");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCDM_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);

  return GWEN_IPCManager_SendRequest(dm->ipcManager,
                                     LCDM_Driver_GetIpcId(d),
                                     dbReq);
}



int LCDM_DeviceManager_StartDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d) {
  GWEN_PROCESS *p=0;
  GWEN_BUFFER *pbuf=0;
  GWEN_BUFFER *abuf=0;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;
  GWEN_PLUGIN_MANAGER *pm=0;

  assert(dm);
  assert(d);

  DBG_INFO(0, "Starting driver \"%s\"", LCDM_Driver_GetDriverName(d));

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LCDM_Driver_GetDriverDataDir(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LCDM_Driver_GetLibraryFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -l ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LCDM_Driver_GetLogFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=getenv("LCDM_DRIVER_LOGLEVEL");
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --loglevel ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  if (dm->addrTypeForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, dm->addrTypeForDrivers);
  }

  if (dm->addrAddrForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, dm->addrAddrForDrivers);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", dm->addrPortForDrivers);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCDM_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LCDM_Driver_GetDriverType(d);
  if (!s) {
    DBG_ERROR(0, "No driver type");
    LCDM_DeviceManager_AbandonDriver(dm, d,
                                     LC_DriverStatusAborted,
                                     "No driver type");
    GWEN_Buffer_free(abuf);
    return -1;
  }

  /* get driver path by loading its plugin description */
  pm=GWEN_PluginManager_FindPluginManager(LCS_PLUGIN_DRIVER);
  if (!pm) {
    DBG_ERROR(0, "Plugin manager \"%s\" not found",
              LCS_PLUGIN_DRIVER);
    GWEN_Buffer_free(abuf);
    abort();
  }
  else {
    GWEN_PLUGIN_DESCRIPTION *pd;
    const char *p;

    pd=GWEN_PluginManager_GetPluginDescr(pm, s);
    if (!pd) {
      DBG_ERROR(0, "Plugin description for driver \"%s\" not found", s);
      LCDM_DeviceManager_AbandonDriver(dm, d,
                                       LC_DriverStatusAborted,
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
  DBG_INFO(0, "Starting driver process for driver \"%s\" (%s)",
           LCDM_Driver_GetDriverName(d), GWEN_Buffer_GetStart(pbuf));
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
    LCDM_DeviceManager_AbandonDriver(dm, d,
                                     LC_DriverStatusAborted,
                                     "Unable to execute driver");
    return -1;
  }

  /* process started */
  DBG_INFO(0, "Process started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LCDM_Driver_SetProcess(d, p);
  LCDM_Driver_SetStatus(d, LC_DriverStatusStarted);
  /* notify callback */
  LCS_Server_DriverChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Driver_GetDriverType(d),
                       LCDM_Driver_GetDriverName(d),
                       LCDM_Driver_GetLibraryFile(d),
                       LC_DriverStatusStarted,
                       "Driver started");
  /* done */
  return 0;
}



int LCDM_DeviceManager_CheckDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d) {
  int done=0;
  GWEN_TYPE_UINT32 nid;
  LC_DRIVER_STATUS dst;
  GWEN_TYPE_UINT32 dflags;

  assert(dm);
  assert(d);

  nid=LCDM_Driver_GetIpcId(d);
  dst=LCDM_Driver_GetStatus(d);

  DBG_ERROR(0, "Checking driver %s (%d)",
            LCDM_Driver_GetDriverName(d), dst);

  dflags=LCDM_Driver_GetDriverFlags(d);
  if (!(dflags & LCDM_DRIVER_FLAGS_REMOTE) &&
      (dflags & LCDM_DRIVER_FLAGS_AUTO) &&
      (LCDM_Driver_GetAssignedReadersCount(d)==0)){
    DBG_NOTICE(0, "Driver \"%s\" is unused, removing it",
               LCDM_Driver_GetDriverName(d));
    LCDM_Driver_List_Del(d);
    LCDM_Driver_free(d);
    return 1;
  } /* if local idle driver timed out*/

  if (dst==LC_DriverStatusAborted) {
    if (!(LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE) &&
        LCDM_Driver_CheckTimeout(d)) {
      DBG_NOTICE(0, "Reenabling driver \"%s\"",
                 LCDM_Driver_GetDriverName(d));
      dst=LC_DriverStatusDown;
      LCDM_Driver_SetStatus(d, dst);
      done++;
    }
  } /* if aborted */

  if (dst==LC_DriverStatusWaitForStart) {
    if (LCDM_Driver_CheckTimeout(d)) {
      int rv;

      rv=LCDM_DeviceManager_StartDriver(dm, d);
      if (rv) {
        DBG_INFO(0, "here (%d)", rv);
        return 1;
      }
      dst=LCDM_Driver_GetStatus(d);
      LCDM_Driver_SetTimeout(d, dm->driverStartTimeout);
      done++;
    }
  }

  if (dst==LC_DriverStatusStopping) {
    GWEN_PROCESS *p;

    p=LCDM_Driver_GetProcess(d);
    if (p) {
      GWEN_PROCESS_STATE pst;

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (LCDM_Driver_CheckTimeout(d)==1) {
          DBG_WARN(0, "Driver is still running, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver still running, "
                                           "killing it");
          done++;
        }
        else {
          /* otherwise give the process a little bit time ... */
          DBG_DEBUG(0, "still waiting for driver to go down");
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Driver terminated normally");
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusDown;
        LCDM_Driver_SetStatus(d, dst);
        LCS_Server_DriverChg(dm->server,
                                     LCDM_Driver_GetDriverId(d),
                                     LCDM_Driver_GetDriverType(d),
                                     LCDM_Driver_GetDriverName(d),
                                     LCDM_Driver_GetLibraryFile(d),
                                     dst,
                                     "Driver terminated normally");
        done++;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Driver terminated abnormally");
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver terminated abnormally");
        done++;;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Driver has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver has been stopped,"
                                         "killing it");
        done++;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Unknown process status, "
                                         "killing it");
        done++;
      }
    } /* if process */
    else {
      if (!(LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LCDM_Driver_Dump(d, stderr, 2);
        abort();
      }
    }
  } /* if stopping */

  if (dst==LC_DriverStatusStarted) {
    /* driver started, check timeout */
    if (LCDM_Driver_CheckTimeout(d)==1) {
      GWEN_PROCESS *p;

      DBG_WARN(0, "Driver \"%s\" timed out", LCDM_Driver_GetDriverName(d));
      p=LCDM_Driver_GetProcess(d);
      if (p) {
        GWEN_PROCESS_STATE pst;

        pst=GWEN_Process_CheckState(p);
        if (pst==GWEN_ProcessStateRunning) {
          DBG_WARN(0,
                   "Driver is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver is running but did not "
                                           "signal readyness, "
                                           "killing it");
          done++;
        }
        else if (pst==GWEN_ProcessStateExited) {
          DBG_WARN(0, "Driver terminated without signalling readyness");
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver terminated "
                                           "without signalling readyness");
          done++;
        }
        else if (pst==GWEN_ProcessStateAborted) {
          DBG_WARN(0, "Driver terminated abnormally");
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver terminated abnormally");
          done++;
        }
        else if (pst==GWEN_ProcessStateStopped) {
          DBG_WARN(0, "Driver has been stopped, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver has been stopped, "
                                           "killing it");
          done++;
        }
        else {
          DBG_ERROR(0, "Unknown process status %d, killing", pst);
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Unknown process status, "
                                           "killing");
          done++;
        }
      } /* if process */
      else {
        if (!(LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE)) {
          DBG_ERROR(0, "No process for local driver:");
          LCDM_Driver_Dump(d, stderr, 2);
          abort();
        }
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for driver start");
    }
  }

  if (dst==LC_DriverStatusUp) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;
    GWEN_NETCONNECTION *conn;

    /* check whether the driver really is still up and running */
    p=LCDM_Driver_GetProcess(d);
    if (p) {
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Driver is not running anymore");
        LCDM_Driver_Dump(d, stderr, 2);
        GWEN_Process_Terminate(p);
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver is not running anymore");
        done++;
      }
    } /* if process */
    else {
      if (!(LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LCDM_Driver_Dump(d, stderr, 2);
        abort();
      }
    }

    /* check connection */
    conn=GWEN_IPCManager_GetConnection(dm->ipcManager,
                                       LCDM_Driver_GetIpcId(d));
    assert(conn);
    if (GWEN_NetConnection_GetStatus(conn)!=
        GWEN_NetTransportStatusLConnected) {
      DBG_ERROR(0, "Driver connection is down");
      p=LCDM_Driver_GetProcess(d);
      if (p) {
        GWEN_Process_Terminate(p);
      }
      LCDM_Driver_SetProcess(d, 0);
      dst=LC_DriverStatusAborted;
      LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                       "Driver connection broken");
      done++;
    }

    DBG_DEBUG(0, "Driver still running");
    if (LCDM_Driver_GetActiveReadersCount(d)==0 &&
        dm->driverIdleTimeout) {
      time_t t;

      /* check for idle timeout */
      t=LCDM_Driver_GetIdleSince(d);
      assert(t);

      if (!(LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE) &&
          dm->driverIdleTimeout &&
          (difftime(time(0), t)>dm->driverIdleTimeout)) {
        GWEN_TYPE_UINT32 rid;

        DBG_NOTICE(0, "Driver \"%s\" is too long idle, stopping it",
                   LCDM_Driver_GetDriverName(d));

        assert(d);
        rid=LCDM_DeviceManager_SendStopDriver(dm, d);
        if (!rid) {
          DBG_ERROR(0, "Could not send StopDriver command for driver \"%s\"",
                    LCDM_Driver_GetDriverName(d));
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Could not send StopDriver "
                                           "command");
          done++;
        }
        else {
          DBG_DEBUG(0, "Sent StopDriver request for driver \"%s\"",
                    LCDM_Driver_GetDriverName(d));
          dst=LC_DriverStatusStopping;
          LCDM_Driver_SetStatus(d, dst);
          LCS_Server_DriverChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               LCDM_Driver_GetDriverType(d),
                               LCDM_Driver_GetDriverName(d),
                               LCDM_Driver_GetLibraryFile(d),
                               dst,
                               "Stopping driver");
          LCDM_Driver_SetTimeout(d, dm->driverStopTimeout);
          done++;
        }
      } /* if timeout */
      /* otherwise reader is not idle */
    }
  }

  if (dst==LC_DriverStatusDown) {
    if (LCDM_Driver_GetActiveReadersCount(d)) {
      LCDM_Driver_SetTimeout(d, dm->driverStartDelay);
      dst=LC_DriverStatusWaitForStart;
      LCDM_Driver_SetStatus(d, dst);
      LCS_Server_DriverChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           LCDM_Driver_GetDriverType(d),
                           LCDM_Driver_GetDriverName(d),
                           LCDM_Driver_GetLibraryFile(d),
                           dst,
                           "Initiating driver start");
      done++;
    }
  }

  return (done!=0);
}



int LCDM_DeviceManager_CheckReader(LCDM_DEVICEMANAGER *dm, LCDM_READER *r) {
  LC_READER_STATUS rst;
  int didSomething=0;
  LCDM_DRIVER *d;

  rst=LCDM_Reader_GetStatus(r);
  d=LCDM_Reader_GetDriver(r);

  DBG_ERROR(0, "Checking reader %s (%d)", LCDM_Reader_GetReaderName(r), rst);

  if (rst==LC_ReaderStatusWaitForStart) {
    if (LCDM_Reader_CheckTimeout(r)) {
      rst=LC_ReaderStatusWaitForDriver;
      LCDM_Reader_SetStatus(r, rst);
      LCDM_Reader_SetTimeout(r, dm->readerStartTimeout);
      didSomething++;
    }
  }

  if (rst==LC_ReaderStatusWaitForDriver) {
    if (LCDM_Reader_CheckTimeout(r)) {
      DBG_ERROR(0, "Reader %s time out", LCDM_Reader_GetReaderName(r));
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
				       "Reader timed out");
      return 1;
    }
    else {
      LC_DRIVER_STATUS dst;

      dst=LCDM_Driver_GetStatus(d);
      if (dst==LC_DriverStatusUp) {
	GWEN_TYPE_UINT32 rid;

	rid=LCDM_DeviceManager_SendStartReader(dm, r);
	if (rid==0) {
	  DBG_ERROR(0, "Reader %s: Could not send start request",
		    LCDM_Reader_GetReaderName(r));
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   "IPC error");
	  return 1;
	}
	rst=LC_ReaderStatusWaitForReaderUp;
	LCDM_Reader_SetStatus(r, rst);
	LCDM_Reader_SetCurrentRequestId(r, rid);
	didSomething++;
      }
      else if (dst!=LC_DriverStatusStarted &&
               dst!=LC_DriverStatusWaitForStart) {
        DBG_ERROR(0, "Bad status of driver for reader %s (%d)",
                  LCDM_Reader_GetReaderName(r), dst);
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					 "Bad driver status");
	return 1;
      }
    }
  }

  if (rst==LC_ReaderStatusWaitForReaderUp) {
    GWEN_TYPE_UINT32 rid;
    GWEN_DB_NODE *dbRsp;

    rid=LCDM_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbRsp=GWEN_IPCManager_GetResponseData(dm->ipcManager, rid);
    if (dbRsp) {
      if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
	GWEN_BUFFER *ebuf;
	const char *e;

	e=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
	DBG_ERROR(0,
                  "Driver reported error on reader startup: %d (%s)",
		  GWEN_DB_GetIntValue(dbRsp, "code", 0, 0),
		  e?e:"<none>");
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
	}
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         GWEN_Buffer_GetStart(ebuf));

	GWEN_Buffer_free(ebuf);
	GWEN_DB_Group_free(dbRsp);
        if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      }
      else {
        const char *s;
        const char *e;

        s=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, 0);
        assert(s);
        e=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, 0);
        if (strcasecmp(s, "error")==0) {
          GWEN_BUFFER *ebuf;

	  DBG_ERROR(0,
                    "Driver reported error on startup of reader \"%s\": %s",
                    LCDM_Reader_GetReaderName(r),
                    e?e:"(none)");
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
            GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
          }
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }

          LCDM_Reader_SetCurrentRequestId(r, 0);
          return -1;
        }
        else {
          const char *readerInfo;

          readerInfo=GWEN_DB_GetCharValue(dbRsp, "body/info", 0, 0);
          if (readerInfo) {
            DBG_NOTICE(0, "Reader \"%s\" is up (%s), info: %s",
		       LCDM_Reader_GetReaderName(r),
		       e?e:"no result text", readerInfo);
	    LCDM_Reader_SetReaderInfo(r, readerInfo);
	  }
	  else {
	    DBG_NOTICE(0, "Reader \"%s\" is up (%s), no info",
		       LCDM_Reader_GetReaderName(r),
		       e?e:"no result text");
	  }
	  rst=LC_ReaderStatusUp;
	  LCDM_Reader_SetStatus(r, rst);

	  LCS_Server_ReaderChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               LCDM_Reader_GetReaderId(r),
                               LCDM_Reader_GetReaderType(r),
                               LCDM_Reader_GetReaderName(r),
                               LC_ReaderStatusUp,
                               e?e:"Reader is up");
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
          LCDM_Reader_SetCurrentRequestId(r, 0);
        }
	didSomething++;
      }
    }
    else {
      if (LCDM_Reader_CheckTimeout(r)) {
	/* reader timed out */
	DBG_WARN(0, "Reader \"%s\" timed out", LCDM_Reader_GetReaderName(r));
        if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         "Reader timed out (command)");
	return -1;
      } /* if timeout */
      DBG_DEBUG(0, "Still some time left");
      return 0;
    }
  }

  if (rst==LC_ReaderStatusUp) {
    if (dm->readerIdleTimeout) {
      time_t t0;

      t0=LCDM_Reader_GetIdleSince(r);
      if (t0) {
	time_t t1;

	t1=time(0);
	if (difftime(t1, t0)>=dm->readerIdleTimeout) {
	  LC_DRIVER_STATUS dst;

          /* stop reader */
          DBG_NOTICE(0, "Reader \"%s\" too long idle, stopping",
                     LCDM_Reader_GetReaderName(r));
          dst=LCDM_Driver_GetStatus(d);
	  if (dst==LC_DriverStatusUp) {
	    GWEN_TYPE_UINT32 rid;

	    rid=LCDM_DeviceManager_SendStopReader(dm, r);
	    if (rid==0) {
	      DBG_ERROR(0, "Reader %s: Could not send stop request",
			LCDM_Reader_GetReaderName(r));
	      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					       "IPC error");
	      return 1;
	    }
	    rst=LC_ReaderStatusWaitForReaderDown;
	    LCDM_Reader_SetStatus(r, rst);
	    LCDM_Reader_SetCurrentRequestId(r, rid);
	    LCS_Server_ReaderChg(dm->server,
                                 LCDM_Driver_GetDriverId(d),
                                 LCDM_Reader_GetReaderId(r),
                                 LCDM_Reader_GetReaderType(r),
                                 LCDM_Reader_GetReaderName(r),
                                 rst,
                                 "Stopping idle reader");
            didSomething++;
          }
	  else {
	    DBG_ERROR(0, "Bad status of driver for reader %s",
		      LCDM_Reader_GetReaderName(r));
	    LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					     "Bad driver status");
	    return 1;
	  }
	}
      } /* if idle */
    }
  } /* if readerStatusUp */

  if (rst==LC_ReaderStatusWaitForReaderDown) {
    GWEN_TYPE_UINT32 rid;
    GWEN_DB_NODE *dbRsp;

    rid=LCDM_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbRsp=GWEN_IPCManager_GetResponseData(dm->ipcManager, rid);
    if (dbRsp) {
      if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
	GWEN_BUFFER *ebuf;
	const char *e;

	e=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
	DBG_ERROR(0,
		  "Driver reported error on reader down: %d (%s)",
		  GWEN_DB_GetIntValue(dbRsp, "code", 0, 0),
		  e?e:"<none>");
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (down)");
	}
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         GWEN_Buffer_GetStart(ebuf));
	GWEN_Buffer_free(ebuf);
	GWEN_DB_Group_free(dbRsp);
        if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      } /* if error message */
      else {
        const char *s;
        const char *e;

        s=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, 0);
        assert(s);
        e=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, 0);
        if (strcasecmp(s, "error")==0) {
          GWEN_BUFFER *ebuf;

	  DBG_ERROR(0,
                    "Driver reported error on stopping of reader \"%s\": %s",
                    LCDM_Reader_GetReaderName(r),
                    e?e:"(none)");
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
	    GWEN_Buffer_AppendString(ebuf, "Reader error (stopping)");
          }
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
          LCDM_Reader_SetCurrentRequestId(r, 0);
	  return -1;
	} /* if error code returned */
        else {
	  DBG_NOTICE(0, "Reader \"%s\" is down as expected",
		     LCDM_Reader_GetReaderName(r));
	  rst=LC_ReaderStatusDown;
	  LCDM_Reader_SetStatus(r, rst);
	  LCS_Server_ReaderChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               LCDM_Reader_GetReaderId(r),
                               LCDM_Reader_GetReaderType(r),
                               LCDM_Reader_GetReaderName(r),
                               LC_ReaderStatusDown,
                               "Reader is down as expected");
	  GWEN_DB_Group_free(dbRsp);
          if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
	  LCDM_Reader_SetCurrentRequestId(r, 0);
          LCDM_Reader_SetTimeout(r, 0);
          DBG_ERROR(0, "Decrementing active reader count");
          LCDM_Driver_DecActiveReadersCount(d, 1);
        }
	didSomething++;
      } /* if not error message */
    }
  } /* if readerStatusWaitForReaderDown */

  if (rst==LC_ReaderStatusAborted) {
    if (LCDM_Reader_CheckTimeout(r)) {
      rst=LC_ReaderStatusDown;
      LCDM_Reader_SetStatus(r, rst);

      LCS_Server_ReaderChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           LCDM_Reader_GetReaderId(r),
                           LCDM_Reader_GetReaderType(r),
                           LCDM_Reader_GetReaderName(r),
                           LC_ReaderStatusDown,
                           "Reenabling reader");
      didSomething++;
    }
  }

  if (rst==LC_ReaderStatusDown) {
    if (LCDM_Reader_GetUsageCount(r)) {
      LCDM_Reader_SetTimeout(r, dm->readerStartDelay);
      LCDM_Reader_SetStatus(r, LC_ReaderStatusWaitForStart);
      LCDM_Driver_IncActiveReadersCount(d, 1);
      LCS_Server_ReaderChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           LCDM_Reader_GetReaderId(r),
                           LCDM_Reader_GetReaderType(r),
                           LCDM_Reader_GetReaderName(r),
                           LC_ReaderStatusWaitForStart,
                           "Starting reader");
      didSomething++;
    }
  } /* if readerStatusDown */

  if (didSomething)
    return 1;
  return 0;
}



int LCDM_DeviceManager_CheckDrivers(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  LCDM_DRIVER *d;

  assert(dm);
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    int rv;

    rv=LCDM_DeviceManager_CheckDriver(dm, d);
    if (rv!=0)
      done++;
    d=LCDM_Driver_List_Next(d);
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager_CheckReaders(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    int rv;

    rv=LCDM_DeviceManager_CheckReader(dm, r);
    if (rv!=0)
      done++;
    r=LCDM_Reader_List_Next(r);
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager__Work(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  int rv;

  DBG_NOTICE(0, "Checking drivers.");
  rv=LCDM_DeviceManager_CheckDrivers(dm);
  if (rv!=0) {
    DBG_NOTICE(0, "Could work on drivers");
    done++;
  }
  DBG_NOTICE(0, "Checking readers.");
  rv=LCDM_DeviceManager_CheckReaders(dm);
  if (rv!=0) {
    DBG_NOTICE(0, "Could work on readers");
    done++;
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager_Work(LCDM_DEVICEMANAGER *dm) {
  int rv;
  int done=0;

  for (;;) {
    rv=LCDM_DeviceManager__Work(dm);
    if (rv!=0)
      done++;
    else
      break;
  }
  if (done)
    return 1;
  return 0;
}



void LCDM_DeviceManager_DriverIpcDown(LCDM_DEVICEMANAGER *dm,
                                      GWEN_TYPE_UINT32 ipcId) {
  // TODO
}




int LCDM_DeviceManager_HandleRequest(LCDM_DEVICEMANAGER *dm,
                                     GWEN_TYPE_UINT32 rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq) {
  int rv;

  assert(dm);
  assert(name);

  if (strcasecmp(name, "DriverReady")==0) {
    rv=LCDM_DeviceManager_HandleDriverReady(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "CardInserted")==0) {
    rv=LCDM_DeviceManager_HandleCardInserted(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "CardRemoved")==0) {
    rv=LCDM_DeviceManager_HandleCardRemoved(dm, rid, dbReq);
  }
  /* Insert more handlers here */
  else {
    DBG_INFO(0, "Command \"%s\" not handled by device manager",
             name);
    rv=1; /* not handled */
  }

  return rv;
}



int LCDM_DeviceManager_HandleDriverReady(LCDM_DEVICEMANAGER *dm,
                                         GWEN_TYPE_UINT32 rid,
                                         GWEN_DB_NODE *dbReq) {
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 driverId;
  GWEN_TYPE_UINT32 nodeId;
  LCDM_DRIVER *d;
  const char *code;
  const char *text;
  int i;
  GWEN_DB_NODE *dbReader;
  int driverCreated=0;
  GWEN_NETCONNECTION *conn;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: DriverReady", nodeId);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/driverId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid driver id (%s)",
              GWEN_DB_GetCharValue(dbReq, "body/driverId", 0, "0"));
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid driver id");
    return -1;
  }
  driverId=i;
  if (driverId==0 && dm->allowRemote==0) {
    DBG_ERROR(0, "Invalid driver id, remote drivers not allowed");
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid driver id, "
                                 "remote drivers not allowed");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* driver ready */
  /* find driver */
  if (driverId) {
    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      if (LCDM_Driver_GetDriverId(d)==driverId)
        break;
      d=LCDM_Driver_List_Next(d);
    } /* while */

    if (!d) {
      DBG_ERROR(0, "Driver \"%08x\" not found", driverId);
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "Driver not found");
      if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }
  }
  else {
    const char *dtype;
    GWEN_DB_NODE *dbDriver;

    /* driver with id=0, must be a remote driver */
    dtype=GWEN_DB_GetCharValue(dbReq, "body/driverType", 0, 0);
    if (!dtype) {
      DBG_ERROR(0, "No driver type given in remote driver");
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "No driver type");
      if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    /* find driver by type name */
    dbDriver=GWEN_DB_FindFirstGroup(dm->dbDrivers, "driver");
    while(dbDriver) {
      const char *dname;

      dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
      if (dname) {
        if (strcasecmp(dname, dtype)==0)
          break;
      }
      dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
    } /* while */

    if (!dbDriver) {
      DBG_ERROR(0, "Unknown driver type \"%s\"", dtype);
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "Unknown driver type");
      if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    /* create driver from DB */
    d=LCDM_Driver_fromDb(dbDriver);
    assert(d);
    driverId=LCDM_Driver_GetDriverId(d);
    LCDM_Driver_AddDriverFlags(d, LCDM_DRIVER_FLAGS_REMOTE);
    LCDM_Driver_AddDriverFlags(d, LCDM_DRIVER_FLAGS_AUTO);

    /* add driver to list */
    DBG_NOTICE(0, "Adding remote driver \"%s\"", dtype);
    LCDM_Driver_List_Add(d, dm->drivers);
    driverCreated=1;
  } /* if driver does not exist */

  /* create all readers enumerated by the driver */
  dbReader=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                            "body/readers");
  if (dbReader)
    dbReader=GWEN_DB_FindFirstGroup(dbReader, "reader");

  if (dbReader==0 && driverCreated) {
    DBG_ERROR(0, "No readers in request");
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No readers in request");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    LCDM_Driver_SetStatus(d, LC_DriverStatusAborted);
    if (driverCreated) {
      LCDM_Driver_List_Del(d);
      LCDM_Driver_free(d);
    }
    return -1;
  }

  /* now really create readers */
  while(dbReader) {
    LCDM_READER *r;

    r=LCDM_Reader_fromDb(d, dbReader);
    assert(r);
    DBG_NOTICE(0, "Adding reader \"%s\" (enumerated by the driver)",
               LCDM_Reader_GetReaderName(r));
    if (LCDM_Driver_GetDriverFlags(d) & LCDM_DRIVER_FLAGS_REMOTE)
      /* if the driver is remote, so is the reader */
      LCDM_Reader_AddFlags(r, LC_READER_FLAGS_REMOTE);

    /* reader has been created automatically */
    LCDM_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);
    /* reader is available */
    LCDM_Reader_SetIsAvailable(r, 1);

    /* add reader to list */
    LCDM_Reader_List_Add(r, dm->readers);

    /* let the CheckReader function automatically start this reader
     * if necessary */
    if (dm->readerUsage)
      LCDM_Reader_IncUsageCount(r, dm->readerUsage);

    LCDM_Driver_IncAssignedReadersCount(d);

    dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
  } /* while */

  /* store node id */
  LCDM_Driver_SetIpcId(d, nodeId);

  /* check code */
  code=GWEN_DB_GetCharValue(dbReq, "body/code", 0, "<none>");
  text=GWEN_DB_GetCharValue(dbReq, "body/text", 0, "<none>");
  if (strcasecmp(code, "OK")!=0) {
    GWEN_BUFFER *ebuf;

    DBG_ERROR(0, "Error in driver \"%08x\": %s",
              driverId, text);
    ebuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(ebuf, "Driver error (");
    GWEN_Buffer_AppendString(ebuf, text);
    GWEN_Buffer_AppendString(ebuf, ")");
    LCDM_DeviceManager_AbandonDriver(dm, d,
                                     LC_DriverStatusAborted,
                                     GWEN_Buffer_GetStart(ebuf));
    GWEN_Buffer_free(ebuf);
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  conn=GWEN_IPCManager_GetConnection(dm->ipcManager, nodeId);
  assert(conn);
  LCS_Server_UseConnectionFor(dm->server, conn,
                              LCS_Connection_TypeDriver,
                              nodeId);

  DBG_NOTICE(0, "Driver \"%08x\" is up (%s)", driverId, text);
  LCDM_Driver_SetStatus(d, LC_DriverStatusUp);
  LCS_Server_DriverChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Driver_GetDriverType(d),
                       LCDM_Driver_GetDriverName(d),
                       LCDM_Driver_GetLibraryFile(d),
                       LC_DriverStatusUp,
                       "Driver up");

  dbRsp=GWEN_DB_Group_new("DriverReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "text", "Driver registered");
  if (GWEN_IPCManager_SendResponse(dm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleCardInserted(LCDM_DEVICEMANAGER *dm,
                                          GWEN_TYPE_UINT32 rid,
                                          GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LCDM_READER *r;
  LCDM_DRIVER *d;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;
  LCCO_CARD *card;
  GWEN_BUFFER *atr;
  const void *p;
  unsigned int bs;
  const char *cardType;
  LC_CARD_TYPE ct;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: Card inserted", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
	     &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),
  cardType=GWEN_DB_GetCharValue(dbReq, "body/cardType", 0, "unknown");
  if (strcasecmp(cardType, "processor")==0)
    ct=LC_CardTypeProcessor;
  else if (strcasecmp(cardType, "memory")==0)
    ct=LC_CardTypeMemory;
  else {
    DBG_WARN(0, "Unknown card type \"%s\"", cardType);
    ct=LC_CardTypeUnknown;
  }
  atr=0;
  p=GWEN_DB_GetBinValue(dbReq, "body/atr", 0, 0, 0, &bs);
  if (p && bs) {
    atr=GWEN_Buffer_new(0, bs, 0, 1);
    GWEN_Buffer_AppendBytes(atr, p, bs);
  }

  /* find reader */
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==readerId)
      break;
    r=LCDM_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    GWEN_Buffer_free(atr);
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  card=LCCO_Card_new();
  LCDM_Card_extend(card, r);
  LCCO_Card_SetReaderId(card, LCDM_Reader_GetReaderId(r));
  LCCO_Card_SetSlotNum(card, slotNum);
  LCCO_Card_SetReadersCardId(card, cardNum);
  LCCO_Card_SetCardType(card, ct);
  LCCO_Card_SetDriverTypeName(card, LCDM_Driver_GetDriverName(d));
  LCCO_Card_SetReaderTypeName(card, LCDM_Reader_GetReaderType(r));
  if (atr)
    LCCO_Card_SetAtr(card,
                     GWEN_Buffer_GetStart(atr),
                     GWEN_Buffer_GetUsedBytes(atr));
  LCCO_Card_SetStatus(card, LC_CardStatusInserted);
  DBG_NOTICE(0, "Free card found with num \"%08x\" in reader \"%s\"(%08x)",
             LCCO_Card_GetReadersCardId(card),
             LCDM_Reader_GetReaderName(r),
             readerId);

  /* make new card known to server */
  LCS_Server_NewCard(dm->server, card);
  LCCO_Card_free(card);

  if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleCardRemoved(LCDM_DEVICEMANAGER *dm,
                                         GWEN_TYPE_UINT32 rid,
                                         GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LCDM_READER *r;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Card removed", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),

  /* find reader */
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==readerId)
      break;
    r=LCDM_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  LCS_Server_CardRemoved(dm->server, readerId, slotNum, cardNum);

  if (GWEN_IPCManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_ListDrivers(LCDM_DEVICEMANAGER *dm) {
  LCDM_DRIVER *d;

  assert(dm);
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    LCS_Server_DriverChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         LCDM_Driver_GetDriverType(d),
                         LCDM_Driver_GetDriverName(d),
                         LCDM_Driver_GetLibraryFile(d),
                         LCDM_Driver_GetStatus(d),
                         "Driver listing");
    d=LCDM_Driver_List_Next(d);
  }

  return 0;
}



int LCDM_DeviceManager_ListReaders(LCDM_DEVICEMANAGER *dm) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    LCDM_DRIVER *d;

    d=LCDM_Reader_GetDriver(r);
    assert(d);

    LCS_Server_ReaderChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         LCDM_Reader_GetReaderId(r),
                         LCDM_Reader_GetReaderType(r),
                         LCDM_Reader_GetReaderName(r),
                         LCDM_Reader_GetStatus(r),
                         "Reader listing");
    r=LCDM_Reader_List_Next(r);
  }

  return 0;
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendCardCommand(LCDM_DEVICEMANAGER *dm,
                                                    LCCO_CARD *card,
                                                    GWEN_DB_NODE *dbCmd) {
  LCDM_READER *r;
  LCDM_DRIVER *d;
  char numbuf[16];
  GWEN_TYPE_UINT32 rid;

  assert(dm);
  assert(card);

  r=LCDM_Card_GetReader(card);
  assert(r);

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  if (LCDM_Driver_GetStatus(d)!=LC_DriverStatusUp) {
    DBG_ERROR(0, "Bad driver status (%d)", LCDM_Driver_GetStatus(d));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  if (LCDM_Reader_GetStatus(r)!=LC_ReaderStatusUp) {
    DBG_ERROR(0, "Bad reader status (%d)", LCDM_Reader_GetStatus(r));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCDM_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LCDM_Reader_GetDriversReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LCCO_Card_GetSlotNum(card));
  GWEN_DB_SetIntValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", LCCO_Card_GetReadersCardId(card));

  rid=GWEN_IPCManager_SendRequest(dm->ipcManager,
                                  LCDM_Driver_GetIpcId(d),
                                  dbCmd);
  if (rid==0) {
    DBG_ERROR(0, "Could not send request");
    return 0;
  }

  return rid;
}












