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
#include "pciscanner_l.h"
#include "pcmciascanner_l.h"
#include "usbrawscanner_l.h"
#include "usbttyscanner_l.h"

#include "common/driverinfo.h"

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
  dm->driverBlackList=GWEN_StringList_new();

  return dm;
}



void LCDM_DeviceManager_free(LCDM_DEVICEMANAGER *dm) {
  if (dm) {
    GWEN_INHERIT_FINI(LCDM_DEVICEMANAGER, dm);
    LC_DevMonitor_free(dm->deviceMonitor);
    GWEN_StringList_free(dm->driverBlackList);
    free(dm->addrTypeForDrivers);
    free(dm->addrAddrForDrivers);
    GWEN_DB_Group_free(dm->dbDrivers);
    GWEN_DB_Group_free(dm->dbConfigDrivers);
    LCDM_Reader_List_free(dm->readers);
    LCDM_Driver_List_free(dm->drivers);

    GWEN_FREE_OBJECT(dm);
  }
}



int LCDM_DeviceManager_Init(LCDM_DEVICEMANAGER *dm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbT;
  const char *p;
  int i;

  DBG_INFO(0, "Initialising device manager");
  assert(dm);
  GWEN_DB_ClearGroup(dm->dbDrivers, 0);

  GWEN_StringList_Clear(dm->driverBlackList);

  /* preset with reasonable values */
  dm->allowRemote=0;
  dm->disableAutoConf=0;
  dm->disablePciScan=0;
  dm->disablePcmciaScan=0;
  dm->disableUsbRawScan=0;
  dm->disableUsbTtyScan=0;
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

  dm->hardwareScanInterval=LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_INTERVAL;

  /* read configuration file */
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "DeviceManager");
  if (dbT==0) {
    DBG_WARN(0,
             "Your configuration does not have a \"DeviceManager\" group. "
             "Please update the file.");
    dbT=dbConfig;
  }

  /* read driver black list */
  for (i=0; ; i++) {
    p=GWEN_DB_GetCharValue(dbT, "driverBlackList", i, 0);
    if (!p)
      break;
    GWEN_StringList_AppendString(dm->driverBlackList, p, 0, 1);
  }

  if (dbT) {
    int defval;

    dm->allowRemote=GWEN_DB_GetIntValue(dbT, "allowRemote", 0,
                                        dm->allowRemote);
    dm->disableAutoConf=GWEN_DB_GetIntValue(dbT, "disableAutoConf", 0,
                                            dm->disableAutoConf);
    defval=dm->disableAutoConf;
    dm->disablePciScan=GWEN_DB_GetIntValue(dbT, "disablePciScan", 0, defval);
    dm->disablePcmciaScan=GWEN_DB_GetIntValue(dbT, "disablePcmciaScan", 0,
                                              defval);
    dm->disableUsbRawScan=GWEN_DB_GetIntValue(dbT, "disableUsbRawScan", 0,
                                              defval);
    dm->disableUsbTtyScan=GWEN_DB_GetIntValue(dbT, "disableUsbTtyScan", 0,
                                              defval);

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

    LCDM_DM_INIT_TIME(hardwareScanInterval)
#undef LCDM_DM_INIT_TIME
  }

  /* ensure some minimum values */
  if (dm->hardwareScanInterval<LCDM_DEVICEMANAGER_MIN_HARDWARE_SCAN_INTERVAL)
    dm->hardwareScanInterval=LCDM_DEVICEMANAGER_MIN_HARDWARE_SCAN_INTERVAL;

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
    GWEN_DB_AddGroup(dm->dbDrivers,
                     GWEN_DB_Group_dup(dbT));
    GWEN_DB_AddGroup(dm->dbConfigDrivers,
                     GWEN_DB_Group_dup(dbT));

    d=LCDM_Driver_fromDb(dbT);
    assert(d);

    if (!LCDM_Driver_GetLogFile(d)) {
      LCDM_DeviceManager_SetDriverLogFile(dm, d);
    }
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_CONFIG);

    DBG_INFO(0, "Adding driver \"%s\"", LCDM_Driver_GetDriverName(d));
    LCDM_Driver_List_Add(d, dm->drivers);

    dbR=GWEN_DB_GetFirstGroup(dbT);
    while(dbR) {
      if (strcasecmp(GWEN_DB_GroupName(dbR), "reader")==0) {
        LCDM_READER *r;

        /* reader section found */
        r=LCDM_Reader_fromDb(d, dbR);
	assert(r);
	/* readers from config are always expected to be active */
	LCDM_Reader_SetIsAvailable(r, 1);
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

  if (dm->disableAutoConf==0) {
    LC_DEVSCANNER *scanner;
    int scanners=0;

    DBG_INFO(0, "Autoconfiguration enabled");
    dm->deviceMonitor=LC_DevMonitor_new();
    if (dm->disablePciScan==0) {
      DBG_INFO(0, "Adding PCI bus scanner");
      scanner=LC_PciScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disablePcmciaScan==0) {
      DBG_INFO(0, "Adding PCMCIA bus scanner");
      scanner=LC_PcmciaScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disableUsbRawScan==0) {
      DBG_INFO(0, "Adding USB bus scanner");
      scanner=LC_UsbRawScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disableUsbTtyScan==0) {
      DBG_INFO(0, "Adding USB TTY bus scanner");
      scanner=LC_UsbTtyScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    dm->lastHardwareScan=0;
    if (scanners==0) {
      DBG_WARN(0,
               "All hardware scanner modules disabled, "
               "disabling autoconfig");
      LC_DevMonitor_free(dm->deviceMonitor);
      dm->deviceMonitor=0;
      dm->disableAutoConf=1;
    }
  }

  LCDM_DeviceManager_ReloadDrivers(dm);

  return 0;
}



int LCDM_DeviceManager_Fini(LCDM_DEVICEMANAGER *dm) {

  DBG_INFO(0, "Deinitializing device manager");

  LC_DevMonitor_free(dm->deviceMonitor);
  dm->deviceMonitor=0;

  GWEN_StringList_Clear(dm->driverBlackList);
  LCDM_Reader_List_Clear(dm->readers);
  LCDM_Driver_List_Clear(dm->drivers);
  free(dm->addrTypeForDrivers);
  free(dm->addrAddrForDrivers);
  GWEN_DB_ClearGroup(dm->dbConfigDrivers, 0);
  GWEN_DB_ClearGroup(dm->dbDrivers, 0);

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






void LCDM_DeviceManager_BeginUseReader(LCDM_DEVICEMANAGER *dm,
                                       GWEN_TYPE_UINT32 rid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  LCDM_Reader_IncUsageCount(r, 1);
}



void LCDM_DeviceManager_EndUseReader(LCDM_DEVICEMANAGER *dm,
                                     GWEN_TYPE_UINT32 rid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  LCDM_Reader_DecUsageCount(r, 1);
}



void LCDM_DeviceManager_BeginUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCDM_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_DeviceManager_BeginUseReader(dm, LCDM_Reader_GetReaderId(r));
}



void LCDM_DeviceManager_EndUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCDM_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_DeviceManager_EndUseReader(dm, LCDM_Reader_GetReaderId(r));
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
    DBG_INFO(0, "Decrementing active reader count %s (%08x)",
             LCDM_Reader_GetReaderName(r),
             LCDM_Reader_GetReaderId(r));
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
                       LCDM_Reader_GetReaderInfo(r),
                       newSt, reason);
}



void LCDM_DeviceManager_AbandonDriver(LCDM_DEVICEMANAGER *dm,
                                      LCDM_DRIVER *d,
                                      LC_DRIVER_STATUS newSt,
                                      const char *reason) {
  LCDM_READER *r;
  GWEN_TYPE_UINT32 ipcId;

  ipcId=LCDM_Driver_GetIpcId(d);
  if (ipcId!=0) {
    /* remove IPC node */
    LCDM_Driver_SetIpcId(d, 0);
    DBG_ERROR(0, "Removing driver client %0x8", ipcId);
    GWEN_IpcManager_RemoveClient(dm->ipcManager, ipcId);
    DBG_ERROR(0, "Removing driver client %0x8: done", ipcId);
  }

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
      if (LCDM_Reader_GetDriver(r)==d) {
        LC_READER_STATUS rst;

        rst=LCDM_Reader_GetStatus(r);
        if (rst!=LC_ReaderStatusDown &&
            rst!=LC_ReaderStatusDisabled &&
            rst!=LC_ReaderStatusAborted) {
          DBG_INFO(0, "Abandoning reader");
          LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted, reason);
        }
      }
      r=LCDM_Reader_List_Next(r);
    }
  }
}



int LCDM_DeviceManager_ReloadDrivers(LCDM_DEVICEMANAGER *dm) {
  GWEN_STRINGLIST *sl;
  GWEN_STRINGLISTENTRY *se;
  GWEN_DB_NODE *dbDrivers;
  GWEN_DB_NODE *dbT;

  DBG_INFO(0, "Reloading driver info files");

  sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                               LCS_PATH_DRIVER_INFODIR);
  assert(sl);

  dbDrivers=GWEN_DB_Group_new("drivers");

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
  GWEN_StringList_free(sl);

  GWEN_DB_Group_free(dm->dbDrivers);
  dm->dbDrivers=GWEN_DB_Group_dup(dm->dbConfigDrivers);

  /* remove all groups of drivers which are blacklisted, add all other */
  while( (dbT=GWEN_DB_GetFirstGroup(dbDrivers)) ) {
    const char *n;

    GWEN_DB_UnlinkGroup(dbT);
    n=GWEN_DB_GetCharValue(dbT, "driverName", 0, 0);
    assert(n);
    if (GWEN_StringList_HasString(dm->driverBlackList, n)) {
      DBG_ERROR(0, "Removing driver \"%s\" (blacklisted)", n);
      GWEN_DB_Group_free(dbT);
    }
    else
      GWEN_DB_AddGroup(dm->dbDrivers, dbT);
  }

  GWEN_DB_Group_free(dbDrivers);

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
  dbReq=GWEN_DB_Group_new("Driver_StartReader");

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
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "ctn", LCDM_Reader_GetCtn(r));
  port=LCDM_Reader_GetPort(r);
  if (port==-1) {
    port=0;
    DBG_INFO(0, "Assigning default port 0 to reader \"%s\"",
             LCDM_Reader_GetReaderName(r));
  }
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "port", port);

  return GWEN_IpcManager_SendRequest(dm->ipcManager,
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
  dbReq=GWEN_DB_Group_new("Driver_StopReader");

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

  return GWEN_IpcManager_SendRequest(dm->ipcManager,
				     LCDM_Driver_GetIpcId(d),
				     dbReq);
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStopDriver(LCDM_DEVICEMANAGER *dm,
                                                   const LCDM_DRIVER *d) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;

  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_StopDriver");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCDM_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);

  return GWEN_IpcManager_SendRequest(dm->ipcManager,
                                     LCDM_Driver_GetIpcId(d),
                                     dbReq);
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendSuspendCheck(LCDM_DEVICEMANAGER *dm,
                                                     const LCDM_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_SuspendCheck");

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

  return GWEN_IpcManager_SendRequest(dm->ipcManager,
				     LCDM_Driver_GetIpcId(d),
				     dbReq);
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_SendResumeCheck(LCDM_DEVICEMANAGER *dm,
                                                    const LCDM_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_ResumeCheck");

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

  return GWEN_IpcManager_SendRequest(dm->ipcManager,
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

  DBG_VERBOUS(0, "Checking driver %s (%d)",
              LCDM_Driver_GetDriverName(d), dst);

  dflags=LCDM_Driver_GetDriverFlags(d);
  if (!(dflags & LC_DRIVER_FLAGS_REMOTE) &&
      (dflags & LC_DRIVER_FLAGS_AUTO) &&
      (LCDM_Driver_GetAssignedReadersCount(d)==0)){
    GWEN_TYPE_UINT32 ipcId;

    DBG_NOTICE(0, "Driver \"%s\" is unused, removing it",
               LCDM_Driver_GetDriverName(d));
    ipcId=LCDM_Driver_GetIpcId(d);
    if (ipcId!=0)
      /* remove IPC node */
      GWEN_IpcManager_RemoveClient(dm->ipcManager, ipcId);

    LCDM_Driver_List_Del(d);
    LCDM_Driver_free(d);
    return 1;
  } /* if local idle driver timed out*/

  if (dst==LC_DriverStatusAborted) {
    if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) &&
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
        if (LCDM_Driver_GetIpcId(d)) {
          /* remove IPC node */
          GWEN_IpcManager_RemoveClient(dm->ipcManager,
                                       LCDM_Driver_GetIpcId(d));
          LCDM_Driver_SetIpcId(d, 0);
        }
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
      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
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
        if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
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
    GWEN_NETLAYER *conn;

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
        return 1;
      }
    } /* if process */
    else {
      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LCDM_Driver_Dump(d, stderr, 2);
        abort();
      }
    }

    /* check connection */
    conn=GWEN_IpcManager_GetNetLayer(dm->ipcManager,
                                     LCDM_Driver_GetIpcId(d));
    assert(conn);
    if (GWEN_NetLayer_GetStatus(conn)!=
        GWEN_NetLayerStatus_Connected) {
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

    DBG_VERBOUS(0, "Driver still running");
    if (LCDM_Driver_GetActiveReadersCount(d)==0 &&
        dm->driverIdleTimeout) {
      time_t t;

      /* check for idle timeout */
      t=LCDM_Driver_GetIdleSince(d);
      assert(t);

      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) &&
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

  DBG_VERBOUS(0, "Checking reader %s (%d)",
              LCDM_Reader_GetReaderName(r), rst);

  if (rst==LC_ReaderStatusWaitForStart) {
    if (LCDM_Reader_CheckTimeout(r)) {
      rst=LC_ReaderStatusWaitForDriver;
      LCDM_Reader_SetStatus(r, rst);
      LCDM_Reader_SetTimeout(r, dm->readerStartTimeout);
      didSomething++;
    }
  }

  if (!LCDM_Reader_IsAvailable(r)) {
    if (LCDM_Reader_GetStatus(r)!=LC_ReaderStatusDisabled) {
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusDisabled,
				       "Reader no longer connected");
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
    dbRsp=GWEN_IpcManager_GetResponseData(dm->ipcManager, rid);
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
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      }
      else {
        const char *s;
        const char *e;

        s=GWEN_DB_GetCharValue(dbRsp, "data/code", 0, 0);
        assert(s);
        e=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, 0);
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
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }

          LCDM_Reader_SetCurrentRequestId(r, 0);
          return -1;
        }
        else {
          const char *readerInfo;

          readerInfo=GWEN_DB_GetCharValue(dbRsp, "data/info", 0, 0);
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
                               LCDM_Reader_GetReaderInfo(r),
                               LC_ReaderStatusUp,
                               e?e:"Reader is up");
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
          LCDM_Reader_SetCurrentRequestId(r, 0);
          if (LCDM_Reader_GetFlags(r) & LC_READER_FLAGS_SUSPENDED_CHECKS) {
            GWEN_TYPE_UINT32 rqid;

            /* checks have been suspended, tell this to the reader */
            rqid=LCDM_DeviceManager_SendSuspendCheck(dm, r);
            if (rqid==0) {
              DBG_INFO(0, "here");
              LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                               "Could not send ResumeCheck "
                                               "command");
              return -1;
            }
        
            /* immediately remove this request, we don't expect an answer */
            if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
              DBG_ERROR(0, "Could not remove request");
              abort();
            }
          }
        }
	didSomething++;
      }
    }
    else {
      if (LCDM_Reader_CheckTimeout(r)) {
	/* reader timed out */
	DBG_WARN(0, "Reader \"%s\" timed out", LCDM_Reader_GetReaderName(r));
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
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
                                 LCDM_Reader_GetReaderInfo(r),
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
    dbRsp=GWEN_IpcManager_GetResponseData(dm->ipcManager, rid);
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
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      } /* if error message */
      else {
        const char *s;
        const char *e;

        s=GWEN_DB_GetCharValue(dbRsp, "data/code", 0, 0);
        assert(s);
        e=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, 0);
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
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
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
                               LCDM_Reader_GetReaderInfo(r),
                               LC_ReaderStatusDown,
                               "Reader is down as expected");
	  GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
	  LCDM_Reader_SetCurrentRequestId(r, 0);
          LCDM_Reader_SetTimeout(r, 0);
          DBG_INFO(0, "Decrementing active reader count");
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
                           LCDM_Reader_GetReaderInfo(r),
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
                           LCDM_Reader_GetReaderInfo(r),
                           LC_ReaderStatusWaitForStart,
                           "Starting reader");
      didSomething++;
    }
  } /* if readerStatusDown */

  if (rst==LC_ReaderStatusDisabled) {
    if ((LCDM_Reader_GetFlags(r) & LC_READER_FLAGS_AUTO) &&
	LCDM_Reader_GetUsageCount(r)==0) {
      DBG_NOTICE(0, "Reader \"%s\" is no longer active, removing it",
                 LCDM_Reader_GetReaderName(r));
      LCDM_Driver_DecAssignedReadersCount(LCDM_Reader_GetDriver(r));

      LCDM_Reader_List_Del(r);
      LCDM_Reader_free(r);
      return 1; /* we did something */
    }
  }

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
    LCDM_DRIVER *dNext;

    dNext=LCDM_Driver_List_Next(d);
    rv=LCDM_DeviceManager_CheckDriver(dm, d);
    if (rv!=0)
      done++;
    d=dNext;
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
    LCDM_READER *rnext;
    int rv;

    rnext=LCDM_Reader_List_Next(r);
    rv=LCDM_DeviceManager_CheckReader(dm, r);
    if (rv!=0)
      done++;
    r=rnext;
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager__Work(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  int rv;

  DBG_VERBOUS(0, "Maybe scanning for hardware changes");
  rv=LCDM_DeviceManager_HardwareScan(dm);
  if (rv!=0)
    done++;

  DBG_VERBOUS(0, "Checking drivers.");
  rv=LCDM_DeviceManager_CheckDrivers(dm);
  if (rv!=0) {
    DBG_VERBOUS(0, "Could work on drivers");
    done++;
  }
  DBG_VERBOUS(0, "Checking readers.");
  rv=LCDM_DeviceManager_CheckReaders(dm);
  if (rv!=0) {
    DBG_VERBOUS(0, "Could work on readers");
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
  LCDM_DRIVER *d;

  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==ipcId)
      break;
    d=LCDM_Driver_List_Next(d);
  } /* while */

  if (d) {
    DBG_NOTICE(0, "Connection of driver \"%s\" (%08x) just went down",
               LCDM_Driver_GetDriverName(d),
               LCDM_Driver_GetDriverId(d));
    LCDM_Driver_SetIpcId(d, 0);
    LCDM_DeviceManager_AbandonDriver(dm, d, LC_DriverStatusDown,
				     "Driver connection went down");
  }
}




int LCDM_DeviceManager_HandleRequest(LCDM_DEVICEMANAGER *dm,
                                     GWEN_TYPE_UINT32 rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq) {
  int rv;

  assert(dm);
  assert(name);

  if (strcasecmp(name, "Driver_Ready")==0) {
    rv=LCDM_DeviceManager_HandleDriverReady(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardInserted")==0) {
    rv=LCDM_DeviceManager_HandleCardInserted(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardRemoved")==0) {
    rv=LCDM_DeviceManager_HandleCardRemoved(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_ReaderError")==0) {
    rv=LCDM_DeviceManager_HandleReaderError(dm, rid, dbReq);
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
  GWEN_TYPE_UINT32 driverFlagsValue;
  GWEN_TYPE_UINT32 driverFlagsMask;
  LCDM_DRIVER *d;
  const char *code;
  const char *text;
  int i;
  GWEN_DB_NODE *dbReader;
  int driverCreated=0;
  GWEN_NETLAYER *conn;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: DriverReady", nodeId);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/driverId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid driver id (%s)",
              GWEN_DB_GetCharValue(dbReq, "data/driverId", 0, "0"));
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
    dtype=GWEN_DB_GetCharValue(dbReq, "data/driverType", 0, 0);
    if (!dtype) {
      DBG_ERROR(0, "No driver type given in remote driver");
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "No driver type");
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    /* create driver from DB */
    d=LCDM_Driver_fromDb(dbDriver);
    assert(d);
    LCDM_Driver_SetIpcId(d, nodeId);
    driverId=LCDM_Driver_GetDriverId(d);
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_REMOTE);
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);

    /* add driver to list */
    DBG_NOTICE(0, "Adding remote driver \"%s\"", dtype);
    LCDM_Driver_List_Add(d, dm->drivers);
    driverCreated=1;
  } /* if driver does not exist */

  driverFlagsValue=LC_DriverFlags_fromDb(dbReq, "data/driverFlagsValue");
  driverFlagsMask=LC_DriverFlags_fromDb(dbReq, "data/driverFlagsMask");
  if (driverFlagsMask) {
    GWEN_TYPE_UINT32 x;

    /* don't change runtime flags */
    x=(((LCDM_Driver_GetDriverFlags(d) ^ driverFlagsValue) &
        (driverFlagsMask & ~LC_DRIVER_FLAGS_RUNTIME_MASK))) ^
      LCDM_Driver_GetDriverFlags(d);
    LCDM_Driver_SetDriverFlags(d, x);
  }

  /* create all readers enumerated by the driver */
  dbReader=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                            "data/readers");
  if (dbReader)
    dbReader=GWEN_DB_FindFirstGroup(dbReader, "reader");

  if (dbReader==0 && driverCreated) {
    DBG_ERROR(0, "No readers in request");
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No readers in request");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    LCDM_DeviceManager_AbandonDriver(dm, d, LC_DriverStatusAborted,
                                     "No readers in driver message");

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
    LCDM_Driver_IncAssignedReadersCount(d);
    DBG_NOTICE(0, "Adding reader \"%s\" (enumerated by the driver)",
               LCDM_Reader_GetReaderName(r));
    if (LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)
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

    dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
  } /* while */

  /* store node id */
  LCDM_Driver_SetIpcId(d, nodeId);

  /* check code */
  code=GWEN_DB_GetCharValue(dbReq, "data/code", 0, "<none>");
  text=GWEN_DB_GetCharValue(dbReq, "data/text", 0, "<none>");
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  conn=GWEN_IpcManager_GetNetLayer(dm->ipcManager, nodeId);
  assert(conn);
  LCS_Server_UseConnectionFor(dm->server, conn,
                              LCS_Connection_Type_Driver,
                              nodeId);

  DBG_NOTICE(0, "Driver \"%08x\" (%s) is up (%s)",
             driverId, LCDM_Driver_GetDriverName(d), text);
  LCDM_Driver_SetStatus(d, LC_DriverStatusUp);
  LCS_Server_DriverChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Driver_GetDriverType(d),
                       LCDM_Driver_GetDriverName(d),
                       LCDM_Driver_GetLibraryFile(d),
                       LC_DriverStatusUp,
                       "Driver up");

  dbRsp=GWEN_DB_Group_new("Driver_ReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "text", "Driver registered");
  if (GWEN_IpcManager_SendResponse(dm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: Card inserted", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
             "%x",
	     &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0),
  cardType=GWEN_DB_GetCharValue(dbReq, "data/cardType", 0, "unknown");
  if (strcasecmp(cardType, "processor")==0)
    ct=LC_CardTypeProcessor;
  else if (strcasecmp(cardType, "memory")==0)
    ct=LC_CardTypeMemory;
  else {
    DBG_WARN(0, "Unknown card type \"%s\"", cardType);
    ct=LC_CardTypeUnknown;
  }
  atr=0;
  p=GWEN_DB_GetBinValue(dbReq, "data/atr", 0, 0, 0, &bs);
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
  LCCO_Card_SetReaderFlags(card, LCDM_Reader_GetFlags(r));
  if (LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_HAS_VERIFY_FN) {
    LCCO_Card_AddReaderFlags(card, LC_READER_FLAGS_DRIVER_HAS_VERIFY);
  }
  if (atr) {
    LCCO_Card_SetAtr(card,
                     GWEN_Buffer_GetStart(atr),
                     GWEN_Buffer_GetUsedBytes(atr));
    GWEN_Buffer_free(atr);
    atr=0;
  }
  LCCO_Card_SetStatus(card, LC_CardStatusInserted);
  DBG_NOTICE(0, "Free card found with num \"%08x\" in reader \"%s\"(%08x)",
             LCCO_Card_GetReadersCardId(card),
             LCDM_Reader_GetReaderName(r),
             readerId);

  /* make new card known to server */
  LCS_Server_NewCard(dm->server, card);
  LCCO_Card_free(card);

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Card removed", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0),

  /* find reader */
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==readerId)
      break;
    r=LCDM_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  LCS_Server_CardRemoved(dm->server, readerId, slotNum, cardNum);

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleReaderError(LCDM_DEVICEMANAGER *dm,
                                         GWEN_TYPE_UINT32 rid,
                                         GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 driverId;
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LCDM_DRIVER *d;
  LCDM_READER *r;
  const char *text;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/driverId", 0, "0"),
             "%x",
             &driverId)!=1) {
    DBG_ERROR(0, "Bad driver id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* find driver */
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetDriverId(d)==driverId)
      break;
    d=LCDM_Driver_List_Next(d);
  } /* while */
  if (!d) {
    DBG_ERROR(0, "Unknown driverId \"%08x\" in command \"%s\"",
              driverId, GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  DBG_NOTICE(0, "Driver %s: Reader error", LCDM_Driver_GetDriverName(d));

  /* get reader id */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
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
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  text=GWEN_DB_GetCharValue(dbReq, "data/text", 0, 0);
  if (!text || !*text)
    text="Reader error flagged by driver";

  /* notify all instances */
  LCS_Server_ReaderChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Reader_GetReaderId(r),
                       LCDM_Reader_GetReaderType(r),
                       LCDM_Reader_GetReaderName(r),
                       LCDM_Reader_GetReaderInfo(r),
                       LC_ReaderStatusAborted,
                       text);


  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
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
                         LCDM_Reader_GetReaderInfo(r),
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

  rid=GWEN_IpcManager_SendRequest(dm->ipcManager,
                                  LCDM_Driver_GetIpcId(d),
                                  dbCmd);
  if (rid==0) {
    DBG_ERROR(0, "Could not send request");
    return 0;
  }

  return rid;
}



GWEN_TYPE_UINT32
LCDM_DeviceManager_SendReaderCommand(LCDM_DEVICEMANAGER *dm,
                                     GWEN_TYPE_UINT32 readerId,
                                     GWEN_DB_NODE *dbCmd) {
  LCDM_READER *r;
  LCDM_DRIVER *d;
  char numbuf[16];
  GWEN_TYPE_UINT32 rid;

  assert(dm);
  assert(readerId);

  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==readerId)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  if (!r) {
    DBG_ERROR(0, "Unknown reader \"%08x\"", readerId);
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

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

  rid=GWEN_IpcManager_SendRequest(dm->ipcManager,
                                  LCDM_Driver_GetIpcId(d),
                                  dbCmd);
  if (rid==0) {
    DBG_ERROR(0, "Could not send request");
    return 0;
  }

  return rid;
}




/* __________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *                      Determining port values from device
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */


int LCDM_DeviceManager__GetAutoPortByDeviceId(GWEN_DB_NODE *dbReader,
					      const LC_DEVICE *dev) {
  GWEN_DB_NODE *dbAutoPort;
  int offset;
  int port;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                              "autoport");
  assert(dbAutoPort);
  offset=GWEN_DB_GetIntValue(dbAutoPort, "offset", 0, 0);

  port=LC_Device_GetDeviceId(dev);
  port+=offset;

  return port;
}



int LCDM_DeviceManager__GetAutoPortByPos(GWEN_DB_NODE *dbReader,
					 const LC_DEVICE *dev,
					 const LC_DEVICE_LIST *deviceList) {
  GWEN_DB_NODE *dbAutoPort;
  int pos=0;
  int i;
  LC_DEVICE_BUSTYPE busType;
  int foundDev=0;
  int offset;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
			      "autoport");
  assert(dbAutoPort);
  offset=GWEN_DB_GetIntValue(dbAutoPort, "offset", 0, 0);

  for (i=0; ; i++) {
    const char *bt;

    bt=GWEN_DB_GetCharValue(dbAutoPort, "busorder", i, 0);
    if (!bt)
      break;
    else {
      busType=LC_Device_BusType_fromString(bt);
      if (busType!=LC_Device_BusType_Unknown) {
	const char *sortKey;

	sortKey=GWEN_DB_GetCharValue(dbAutoPort, "sortKey", i, "vendorId");
	if (strcasecmp(sortKey, "vendorId")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
	  while(tdev) {
	    if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev) &&
		LC_Device_GetBusId(tdev)==LC_Device_GetBusId(dev) &&
		LC_Device_GetDeviceId(tdev)==LC_Device_GetDeviceId(dev)) {
	      foundDev=1;
	      break;
	    }
	    if (LC_Device_GetBusType(tdev)==busType &&
		LC_Device_GetVendorId(tdev)==LC_Device_GetVendorId(dev)) {
	      pos++;
	    }
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by vendor */
	else if (strcasecmp(sortKey, "productId")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
	  while(tdev) {
	    if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev) &&
		LC_Device_GetBusId(tdev)==LC_Device_GetBusId(dev) &&
		LC_Device_GetDeviceId(tdev)==LC_Device_GetDeviceId(dev)) {
	      foundDev=1;
	      break;
	    }
	    if (LC_Device_GetBusType(tdev)==busType &&
		LC_Device_GetVendorId(tdev)==LC_Device_GetVendorId(dev) &&
		LC_Device_GetProductId(tdev)==LC_Device_GetProductId(dev)) {
	      pos++;
	    }
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by vendor and product */
        else if (strcasecmp(sortKey, "BusPos")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
	  while(tdev) {
	    if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev) &&
		LC_Device_GetBusId(tdev)==LC_Device_GetBusId(dev) &&
		LC_Device_GetDeviceId(tdev)==LC_Device_GetDeviceId(dev)) {
	      foundDev=1;
	      break;
	    }
	    if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev))
	      pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by position */
        else if (strcasecmp(sortKey, "DriverType")==0) {
          LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
          while(tdev) {
            const char *s1, *s2;

            if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev) &&
		LC_Device_GetBusId(tdev)==LC_Device_GetBusId(dev) &&
		LC_Device_GetDeviceId(tdev)==LC_Device_GetDeviceId(dev)) {
	      foundDev=1;
	      break;
            }
            s1=LC_Device_GetDriverType(dev);
            s2=LC_Device_GetDriverType(tdev);
            if (s1 && s2 && strcasecmp(s1, s2)==0)
              pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by driverType */
        else if (strcasecmp(sortKey, "ReaderType")==0) {
          LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
          while(tdev) {
            const char *s1, *s2;

            if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev) &&
		LC_Device_GetBusId(tdev)==LC_Device_GetBusId(dev) &&
		LC_Device_GetDeviceId(tdev)==LC_Device_GetDeviceId(dev)) {
	      foundDev=1;
	      break;
            }
            s1=LC_Device_GetReaderType(dev);
            s2=LC_Device_GetReaderType(tdev);
            if (s1 && s2 && strcasecmp(s1, s2)==0)
              pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by readerType */
	else {
	  DBG_ERROR(0, "Unknown sort key \"%s\"", sortKey);
          return -1;
        }
      }
      else {
        DBG_WARN(0, "Unknown bus type \"%s\"", bt);
      }
    }
    if (foundDev)
      break;
  } /* for busorder elements */

  i=pos+offset;
  return i;
}



int LCDM_DeviceManager__GetAutoPortByFixed(GWEN_DB_NODE *dbReader,
					   const LC_DEVICE *dev) {
  GWEN_DB_NODE *dbAutoPort;
  GWEN_BUFFER *tbuf;
  LC_DEVICE_BUSTYPE busType;
  int port;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
			      "autoport");
  assert(dbAutoPort);
  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(tbuf, "ports/");
  busType=LC_Device_GetBusType(dev);
  GWEN_Buffer_AppendString(tbuf, LC_Device_BusType_toString(busType));
  GWEN_Buffer_AppendString(tbuf, "-fixed");
  port=GWEN_DB_GetIntValue(dbReader, GWEN_Buffer_GetStart(tbuf), 0, 0);
  GWEN_Buffer_free(tbuf);

  return port;
}



int LCDM_DeviceManager_GetAutoPort(LCDM_DEVICEMANAGER *dm,
				   GWEN_DB_NODE *dbReader,
				   const LC_DEVICE *dev,
				   const LC_DEVICE_LIST *deviceList) {
  const char *s;
  int port;

  s=GWEN_DB_GetCharValue(dbReader, "autoport/mode", 0, 0);
  if (!s) {
    DBG_ERROR(0, "No autoport mode");
    return -1;
  }

  if (strcasecmp(s, "deviceId")==0)
    port=LCDM_DeviceManager__GetAutoPortByDeviceId(dbReader, dev);
  else if (strcasecmp(s, "pos")==0)
    port=LCDM_DeviceManager__GetAutoPortByPos(dbReader,
					     dev, deviceList);
  else if (strcasecmp(s, "fixed")==0)
    port=LCDM_DeviceManager__GetAutoPortByFixed(dbReader, dev);
  else {
    DBG_ERROR(0, "Invalid autoport mode \"%s\"", s);
    return -1;
  }

  return port;
}



void LCDM_DeviceManager_SetDriverLogFile(LCDM_DEVICEMANAGER *dm,
                                         LCDM_DRIVER *d) {
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
  GWEN_StringList_free(sl);
}



int LCDM_DeviceManager_DeviceUp(LCDM_DEVICEMANAGER *dm,
                                LC_DEVICE *ud,
				const LC_DEVICE_LIST *deviceList) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader=0;
  const char *busType;

  DBG_DEBUG(0, "Device %s/%04x/%04x up",
            LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
            LC_Device_GetVendorId(ud),
            LC_Device_GetProductId(ud));

  busType=LC_Device_BusType_toString(LC_Device_GetBusType(ud));

  /* find driver and reader configuration */
  dbDriver=GWEN_DB_FindFirstGroup(dm->dbDrivers, "driver");
  while(dbDriver) {
    dbReader=GWEN_DB_FindFirstGroup(dbDriver, "reader");
    while(dbReader) {
      if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
                                          "busType", 0,
                                          "serial"),
		     busType)==0) {
	if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
	     (int)LC_Device_GetVendorId(ud)) &&
	    (GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
	     (int)LC_Device_GetProductId(ud))) {
	  /* reader found */
	  break;
	}
      }
      dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
    } /* while */

    if (dbReader)
      break;
    dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
  } /* while */

  if (dbDriver && dbReader) {
    LCDM_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LCDM_READER *r;
    char numbuf[32];
    int port;

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);
    LC_Device_SetDriverType(ud, dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);
    LC_Device_SetReaderType(ud, rtype);

    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      if (strcasecmp(LCDM_Driver_GetDriverName(d), dname)==0) {
	if (LCDM_Driver_GetMaxReaders(d)>
	    LCDM_Driver_GetAssignedReadersCount(d)) {
	  DBG_NOTICE(0,
		     "Reusing driver %s: MaxReaders: %d, Active Readers: %d",
		     LCDM_Driver_GetDriverName(d),
                     LCDM_Driver_GetMaxReaders(d),
                     LCDM_Driver_GetAssignedReadersCount(d));
	  break;
	}
      }
      d=LCDM_Driver_List_Next(d);
    } /* while */

    if (!d) {
      /* no driver exists, create one */
      d=LCDM_Driver_fromDb(dbDriver);
      assert(d);
      if (!LCDM_Driver_GetLogFile(d)) {
        LCDM_DeviceManager_SetDriverLogFile(dm, d);
      }
      LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);
      LCDM_Driver_List_Add(d, dm->drivers);
    }

    /* create reader */
    r=LCDM_Reader_fromDb(d, dbReader);
    assert(r);
    LCDM_Driver_IncAssignedReadersCount(d);
    LCDM_Reader_List_Add(r, dm->readers);
    LCDM_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(dm->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LCDM_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (%s/%04x/%04x)",
               GWEN_Buffer_GetStart(nbuf),
               LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
	       LC_Device_GetVendorId(ud),
	       LC_Device_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    port=LCDM_DeviceManager_GetAutoPort(dm, dbReader, ud, deviceList);
    if (port<0) {
      DBG_WARN(0, "Could not automatically assign port, assuming 1");
      port=1;
    }
    else {
      DBG_NOTICE(0, "Automatically assigned port %d to reader \"%s\"",
                 port, LCDM_Reader_GetReaderName(r));
    }
    LCDM_Reader_SetPort(r, port);
    LCDM_Reader_SetBusType(r, LC_Device_GetBusType(ud));
    LCDM_Reader_SetBusId(r, LC_Device_GetBusId(ud));
    LCDM_Reader_SetDeviceId(r, LC_Device_GetDeviceId(ud));
    LCDM_Reader_SetIsAvailable(r, 1);
    LCDM_Reader_SetStatus(r, LC_ReaderStatusDown);
    if (dm->readerUsage)
      LCDM_Reader_IncUsageCount(r, dm->readerUsage);
    LCS_Server_ReaderChg(dm->server,
			 LCDM_Driver_GetDriverId(d),
			 LCDM_Reader_GetReaderId(r),
			 LCDM_Reader_GetReaderType(r),
			 LCDM_Reader_GetReaderName(r),
                         LCDM_Reader_GetReaderInfo(r),
			 LC_ReaderStatusDown, "New reader detected");
  }
  else {
    DBG_INFO(0, "Device %s/%04x/%04x is not a known reader",
             LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
             LC_Device_GetVendorId(ud),
             LC_Device_GetProductId(ud));
  }

  return 0;
}



int LCDM_DeviceManager_DeviceDown(LCDM_DEVICEMANAGER *dm,
				  const LC_DEVICE *ud) {
  LCDM_READER *r;

  assert(dm);
  assert(ud);

  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetBusType(r)==LC_Device_GetBusType(ud) &&
	LCDM_Reader_GetBusId(r)==LC_Device_GetBusId(ud) &&
	LCDM_Reader_GetDeviceId(r)==LC_Device_GetDeviceId(ud)) {
      DBG_NOTICE(0, "Reader \"%s\" disconnected",
		 LCDM_Reader_GetReaderName(r));
      LCDM_Reader_SetIsAvailable(r, 0);
      /* the rest will be handled in the CheckReader function */
      break;
    }
    r=LCDM_Reader_List_Next(r);
  }

  return 0;
}



int LCDM_DeviceManager_HardwareScan(LCDM_DEVICEMANAGER *dm) {
  time_t t1;
  int reloadedDrivers=0;

  if (dm->disableAutoConf)
    return 0; /* nothing done */

  t1=time(0);
  if (dm->lastHardwareScan==0 ||
      difftime(t1, dm->lastHardwareScan)>dm->hardwareScanInterval) {
    int rv;

    rv=LC_DevMonitor_Scan(dm->deviceMonitor);
    dm->lastHardwareScan=t1;

    if (rv==1) {
      return 0; /* no changes */
    }
    else if (rv==0) {
      LC_DEVICE_LIST *devFullList;
      LC_DEVICE_LIST *devList;
      LC_DEVICE *dev;
      int changes=0;

      DBG_NOTICE(0, "Changes in hardware list");
      devFullList=LC_DevMonitor_GetCurrentDevices(dm->deviceMonitor);

      /* first handle devices which went down */
      devList=LC_DevMonitor_GetLostDevices(dm->deviceMonitor);
      dev=LC_Device_List_First(devList);
      while(dev) {
        LCDM_DeviceManager_DeviceDown(dm, dev);
        changes++;
        dev=LC_Device_List_Next(dev);
      }

      /* then handle devices which are up */
      devList=LC_DevMonitor_GetNewDevices(dm->deviceMonitor);
      dev=LC_Device_List_First(devList);
      while(dev) {
        if (reloadedDrivers==0) {
          LCDM_DeviceManager_ReloadDrivers(dm);
          reloadedDrivers=1;
        }

        LCDM_DeviceManager_DeviceUp(dm, dev, devFullList);
        changes++;
        dev=LC_Device_List_Next(dev);
      }

      if (changes)
        return 1;
    }
    else {
      DBG_ERROR(0, "Error scanning");
    }
  }

  return 0; /* nothing done */
}



const char *LCDM_DeviceManager_GetDriverVar(LCDM_DEVICEMANAGER *dm,
                                            LCCO_CARD *card,
                                            const char *vname) {
  LCDM_READER *r;
  LCDM_DRIVER *d;
  GWEN_DB_NODE *dbT;

  assert(dm);
  r=LCDM_Card_GetReader(card);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);

  dbT=LCDM_Driver_GetDriverVars(d);
  assert(dbT);

  return GWEN_DB_GetCharValue(dbT, vname, 0, 0);
}



void LCDM_DeviceManager_DumpState(const LCDM_DEVICEMANAGER *dm) {
  if (!dm) {
    fprintf(stderr, "No device manager.\n");
    return;
  }
  else {
    LCDM_DRIVER *d;
    LCDM_READER *r;

    fprintf(stderr, "DeviceManager\n");
    fprintf(stderr, "=====================================\n");
    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      LCDM_Driver_Dump(d, stderr, 2);
      d=LCDM_Driver_List_Next(d);
    }

    r=LCDM_Reader_List_First(dm->readers);
    while(r) {
      LCDM_Reader_Dump(r, stderr, 2);
      r=LCDM_Reader_List_Next(r);
    }
  }
}



LCS_LOCKMANAGER*
LCDM_DeviceManager_GetLockManager(const LCDM_DEVICEMANAGER *dm,
                                  GWEN_TYPE_UINT32 rid,
                                  int slot) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  if (r)
    return LCDM_Reader_GetLockManager(r, slot);
  else {
    DBG_WARN(0, "Reader \"%08x\" not found", rid);
    return 0;
  }

}



void LCDM_DeviceManager_ClientDown(LCDM_DEVICEMANAGER *dm,
                                   GWEN_TYPE_UINT32 clid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    int slots;
    int i;

    slots=LCDM_Reader_GetSlots(r);
    for (i=0; i<slots; i++) {
      LCS_LOCKMANAGER *lm;

      lm=LCDM_Reader_GetLockManager(r, i);
      assert(lm);
      LCS_LockManager_RemoveAllClientRequests(lm, clid);
    }
    r=LCDM_Reader_List_Next(r);
  }
}



GWEN_TYPE_UINT32 LCDM_DeviceManager_LockReader(LCDM_DEVICEMANAGER *dm,
                                               GWEN_TYPE_UINT32 rid,
                                               GWEN_TYPE_UINT32 clid,
                                               int maxLockTime,
                                               int maxLockCount) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_LockReader(r, clid, maxLockTime, maxLockCount);
}



int LCDM_DeviceManager_CheckLockReaderRequest(LCDM_DEVICEMANAGER *dm,
                                              GWEN_TYPE_UINT32 rid,
                                              GWEN_TYPE_UINT32 rqid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_CheckLockRequest(r, rqid);
}



int LCDM_DeviceManager_CheckLockReaderAccess(LCDM_DEVICEMANAGER *dm,
                                             GWEN_TYPE_UINT32 rid,
                                             GWEN_TYPE_UINT32 rqid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_CheckLockAccess(r, rqid);
}



int LCDM_DeviceManager_RemoveLockReaderRequest(LCDM_DEVICEMANAGER *dm,
                                               GWEN_TYPE_UINT32 rid,
                                               GWEN_TYPE_UINT32 rqid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_RemoveLockRequest(r, rqid);
}



int LCDM_DeviceManager_UnlockReader(LCDM_DEVICEMANAGER *dm,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_TYPE_UINT32 rqid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_Unlock(r, rqid);
}



int LCDM_DeviceManager_SuspendReaderCheck(LCDM_DEVICEMANAGER *dm,
                                          GWEN_TYPE_UINT32 rid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  LCDM_Reader_AddFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
  if (LCDM_Reader_GetStatus(r)==LC_ReaderStatusUp) {
    GWEN_TYPE_UINT32 rqid;

    rqid=LCDM_DeviceManager_SendSuspendCheck(dm, r);
    if (rqid==0) {
      DBG_INFO(0, "here");
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                       "Could not send SuspendCheck "
                                       "command");
      return -1;
    }

    /* immediately remove this request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }

  return 0;
}



void LCDM_DeviceManager_ResumeReaderCheck(LCDM_DEVICEMANAGER *dm,
                                          GWEN_TYPE_UINT32 rid) {
  LCDM_READER *r;

  assert(dm);
  r=LCDM_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetReaderId(r)==rid)
      break;
    r=LCDM_Reader_List_Next(r);
  }

  assert(r);
  if (LCDM_Reader_GetStatus(r)==LC_ReaderStatusUp) {
    GWEN_TYPE_UINT32 rqid;

    rqid=LCDM_DeviceManager_SendResumeCheck(dm, r);
    if (rqid==0) {
      DBG_INFO(0, "here");
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                       "Could not send ResumeCheck "
                                       "command");
      return;
    }

    /* immediately remove this request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }
  LCDM_Reader_SubFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
}















