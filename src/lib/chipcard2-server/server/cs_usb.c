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


int LC_CardServer_USBDevice_Up(LC_CARDSERVER *cs, LC_USBDEVICE *ud) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(cs->dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USB")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    LC_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LC_READER *r;
    char numbuf[32];
    int port;
    int defPort;

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      if (strcasecmp(LC_Driver_GetDriverName(d), dname)==0) {
        if (LC_Driver_GetMaxReaders(d)>LC_Driver_GetAssignedReadersCount(d)) {
          DBG_NOTICE(0,
                     "Reusing driver %s: MaxReaders: %d, Active Readers: %d",
                     LC_Driver_GetDriverName(d),
                     LC_Driver_GetMaxReaders(d),
                     LC_Driver_GetAssignedReadersCount(d));
          break;
        }
      }
      d=LC_Driver_List_Next(d);
    } /* while */

    if (!d) {
      GWEN_BUFFER *lbuf;

      /* no driver exists, create one */
      d=LC_Driver_FromDb(dbDriver);
      assert(d);
      LC_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);
      lbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LC_CardServer_ReplaceVar(LC_DEFAULT_LOGDIR
                               "/drivers/@driver@"
                               "/@reader@"
                               ".log",
                               "driver", dname, lbuf);
      DBG_DEBUG(0, "Logfile is \"%s\"",
                GWEN_Buffer_GetStart(lbuf));
      LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
      LC_Driver_List_Add(d, cs->drivers);
    }

    /* create reader */
    r=LC_Reader_FromDb(d, dbReader);
    assert(r);
    LC_Driver_IncAssignedReadersCount(d);
    LC_Reader_List_Add(r, cs->readers);
    LC_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(cs->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LC_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (USB: %04x/%04x)",
               GWEN_Buffer_GetStart(nbuf),
               LC_USBDevice_GetVendorId(ud),
               LC_USBDevice_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    defPort=GWEN_DB_GetIntValue(dbReader, "ports/default", 0,
				LC_USBDevice_GetDeviceId(ud));
    snprintf(numbuf, sizeof(numbuf), "ports/USB%d",
	     LC_USBDevice_GetDeviceId(ud));

    if (!GWEN_DB_VariableExists(dbReader, numbuf) &&
        !GWEN_DB_VariableExists(dbReader, "ports/default")) {
      LC_USBDEVICE_LIST *usbDevices;
      LC_USBDEVICE *tud;
      int count;
      int autoPortOffset;

      count=0;
      autoPortOffset=LC_Driver_GetAutoPortOffset(d);
      if (autoPortOffset==-1)
        autoPortOffset=0;

      /* neither port nor default port available, just count the devices */
      usbDevices=LC_USBMonitor_GetCurrentDevices(cs->usbMonitor);
      tud=LC_USBDevice_List_First(usbDevices);
      assert(tud);
      while(tud) {
        GWEN_DB_NODE *dbT;

        dbT=GWEN_DB_GetFirstGroup(dbDriver);
        while(dbT) {
          if (strcasecmp(GWEN_DB_GroupName(dbT), "reader")==0) {
            if (strcasecmp(GWEN_DB_GetCharValue(dbT,
                                                "comType", 0, "serial"),
                           "serial")!=0) {
	      if ((GWEN_DB_GetIntValue(dbT, "vendorId", 0, 0)==
		   (int)LC_USBDevice_GetVendorId(tud)) &&
		  (GWEN_DB_GetIntValue(dbT, "productId", 0, 0)==
		   (int)LC_USBDevice_GetProductId(tud))) {
		/* reader found */
		break;
	      }
	    }
	  }
          dbT=GWEN_DB_GetNextGroup(dbT);
        } /* while */
        if (dbT) {
          count++;
        }
        if (LC_USBDevice_GetDevicePos(tud)==
            LC_USBDevice_GetDevicePos(ud))
          break;

        tud=LC_USBDevice_List_Next(tud);
      } /* while tud */
      assert(tud);
      assert(count);

      port=count-1+autoPortOffset;
      DBG_NOTICE(0, "Assigning port value %d to reader \"%s\"",
                 port, LC_Reader_GetReaderName(r));
    }
    else {
      port=GWEN_DB_GetIntValue(dbReader, numbuf, 0, defPort);
    }
    LC_Reader_SetPort(r, port);
    LC_Reader_SetBusId(r, LC_USBDevice_GetBusId(ud));
    LC_Reader_SetDeviceId(r, LC_USBDevice_GetDeviceId(ud));
    LC_Reader_SetIsAvailable(r, 1);

    if (LC_CardServer_Reader_Up(cs, r)) {
      DBG_INFO(0, "here");
    }
  }
  else {
    DBG_INFO(0, "Device %04x/%04x is not a known non-ttyUSB reader",
	     LC_USBDevice_GetVendorId(ud),
	     LC_USBDevice_GetProductId(ud));
  }

  return 0;
}



int LC_CardServer_USBDevice_Down(LC_CARDSERVER *cs, LC_USBDEVICE *ud) {
  LC_READER *r;

  assert(cs);
  assert(ud);

  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (strcasecmp(LC_Reader_GetComType(r), "USB")==0) {
      if ((LC_Reader_GetBusId(r)==
	   (int)LC_USBDevice_GetBusId(ud)) &&
	  (LC_Reader_GetDeviceId(r)==
	   (int)LC_USBDevice_GetDeviceId(ud))) {
	/* reader found */
	break;
      }
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  if (!r) {
    DBG_DEBUG(0, "Unknown device %04x/%04x",
	      LC_USBDevice_GetBusId(ud),
	      LC_USBDevice_GetDeviceId(ud));
    return 0;
  }

  DBG_NOTICE(0, "Reader \"%s\" unplugged",
             LC_Reader_GetReaderName(r));
  if (LC_CardServer_StopReader(cs, r)) {
    DBG_INFO(0, "here");
  }
  LC_Reader_SetIsAvailable(r, 0);
  return 0;
}



int LC_CardServer_USBTTYDevice_Up(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(cs->dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USBSerial")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBTTYDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBTTYDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    LC_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LC_READER *r;
    char numbuf[32];
    int port;

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      if (strcasecmp(LC_Driver_GetDriverName(d), dname)==0) {
        if (LC_Driver_GetMaxReaders(d)>LC_Driver_GetAssignedReadersCount(d)) {
          DBG_NOTICE(0,
                     "Reusing driver %s: MaxReaders: %d, Active Readers: %d",
                     LC_Driver_GetDriverName(d),
                     LC_Driver_GetMaxReaders(d),
                     LC_Driver_GetAssignedReadersCount(d));
          break;
        }
      }
      d=LC_Driver_List_Next(d);
    } /* while */

    if (!d) {
      GWEN_BUFFER *lbuf;

      /* no driver exists, create one */
      d=LC_Driver_FromDb(dbDriver);
      assert(d);
      LC_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);
      lbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LC_CardServer_ReplaceVar(LC_DEFAULT_LOGDIR
                               "/drivers/@driver@"
                               "/@reader@"
                               ".log",
                               "driver", dname, lbuf);
      DBG_DEBUG(0, "Logfile is \"%s\"",
                GWEN_Buffer_GetStart(lbuf));
      LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
      LC_Driver_List_Add(d, cs->drivers);
    }

    /* create reader */
    r=LC_Reader_FromDb(d, dbReader);
    assert(r);
    LC_Driver_IncAssignedReadersCount(d);
    LC_Reader_List_Add(r, cs->readers);
    LC_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(cs->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LC_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (USBTTY: %04x/%04x)",
	       GWEN_Buffer_GetStart(nbuf),
	       LC_USBTTYDevice_GetVendorId(ud),
	       LC_USBTTYDevice_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    snprintf(numbuf, sizeof(numbuf), "ports/USB%d",
	     LC_USBTTYDevice_GetPort(ud));
    port=GWEN_DB_GetIntValue(dbReader, numbuf, 0,
			     LC_USBTTYDevice_GetPort(ud));
    DBG_DEBUG(0, "Looked up value for port \"%s\" (%d)", numbuf, port);
    LC_Reader_SetPort(r, port);
    LC_Reader_SetDeviceId(r, LC_USBTTYDevice_GetPort(ud));
    LC_Reader_SetIsAvailable(r, 1);

    if (LC_CardServer_Reader_Up(cs, r)) {
      DBG_INFO(0, "here");
    }
  }
  else {
    DBG_INFO(0, "Device %04x/%04x is not a known ttyUSB reader",
	     LC_USBTTYDevice_GetVendorId(ud),
	     LC_USBTTYDevice_GetProductId(ud));
  }

  return 0;
}



int LC_CardServer_USBTTYDevice_Down(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud) {
  LC_READER *r;

  assert(cs);
  assert(ud);

  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (strcasecmp(LC_Reader_GetComType(r), "USBSerial")==0) {
      if (LC_Reader_GetDeviceId(r)==(int)LC_USBTTYDevice_GetPort(ud)) {
	/* reader found */
	break;
      }
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  if (!r) {
    DBG_DEBUG(0, "Unknown device %02x",
	      LC_USBTTYDevice_GetPort(ud));
    return 0;
  }

  DBG_NOTICE(0, "Reader \"%s\" unplugged",
	     LC_Reader_GetReaderName(r));
  if (LC_CardServer_StopReader(cs, r)) {
    DBG_INFO(0, "here");
  }

  LC_Reader_SetIsAvailable(r, 0);
  return 0;
}



int LC_CardServer_ScanUSB(LC_CARDSERVER *cs) {
  int rv;
  int reloaded;

  reloaded=0;
  if (cs->lastUsbScan==0 ||
      (cs->usbScanInterval &&
       difftime(time(0), cs->lastUsbScan)>=cs->usbScanInterval)){
    rv=LC_USBMonitor_Scan(cs->usbMonitor);
    if (rv==-1) {
      DBG_INFO(0, "Error scanning USB bus");
    }
    else if (rv==1) {
      DBG_VERBOUS(0, "No changes on USB bus");
    }
    else {
      LC_USBDEVICE_LIST *newDevices;
      LC_USBDEVICE_LIST *lostDevices;
      LC_USBDEVICE *ud;
  
      lostDevices=LC_USBMonitor_GetLostDevices(cs->usbMonitor);
      assert(lostDevices);
      ud=LC_USBDevice_List_First(lostDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x down",
		  LC_USBDevice_GetVendorId(ud),
		  LC_USBDevice_GetProductId(ud));
	if (LC_CardServer_USBDevice_Down(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBDevice_List_Next(ud);
      } /* while */

      newDevices=LC_USBMonitor_GetNewDevices(cs->usbMonitor);
      assert(newDevices);
      ud=LC_USBDevice_List_First(newDevices);
      while(ud) {
        DBG_DEBUG(0, "Device %02x/%02x up",
		  LC_USBDevice_GetVendorId(ud),
		  LC_USBDevice_GetProductId(ud));
	if (!reloaded) {
	  /* reload driver list */
	  GWEN_DB_Group_free(cs->dbDrivers);
	  cs->dbDrivers=GWEN_DB_Group_dup(cs->dbConfigDrivers);
	  reloaded=!LC_CardServer_ReadDrivers(cs->dataDir, cs->dbDrivers, 1);
        }

	if (LC_CardServer_USBDevice_Up(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBDevice_List_Next(ud);
      } /* while */
    }
    cs->lastUsbScan=time(0);
  }

  if (cs->lastUsbTtyScan==0 ||
      (cs->usbTtyScanInterval &&
       difftime(time(0), cs->lastUsbTtyScan)>=cs->usbTtyScanInterval)){
    if (LC_USBTTYMonitor_Scan(cs->usbTtyMonitor)) {
      DBG_DEBUG(0, "Error scanning USB bus (serial)");
    }
    else {
      LC_USBTTYDEVICE_LIST *newDevices;
      LC_USBTTYDEVICE_LIST *lostDevices;
      LC_USBTTYDEVICE *ud;

      lostDevices=LC_USBTTYMonitor_GetLostDevices(cs->usbTtyMonitor);
      assert(lostDevices);
      ud=LC_USBTTYDevice_List_First(lostDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x down",
		  LC_USBTTYDevice_GetVendorId(ud),
		  LC_USBTTYDevice_GetProductId(ud));
	if (LC_CardServer_USBTTYDevice_Down(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBTTYDevice_List_Next(ud);
      } /* while */

      newDevices=LC_USBTTYMonitor_GetNewDevices(cs->usbTtyMonitor);
      assert(newDevices);
      ud=LC_USBTTYDevice_List_First(newDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x up",
		  LC_USBTTYDevice_GetVendorId(ud),
		  LC_USBTTYDevice_GetProductId(ud));
	if (!reloaded) {
	  /* reload driver list */
	  GWEN_DB_Group_free(cs->dbDrivers);
	  cs->dbDrivers=GWEN_DB_Group_dup(cs->dbConfigDrivers);
	  reloaded=!LC_CardServer_ReadDrivers(cs->dataDir, cs->dbDrivers, 1);
	}
	if (LC_CardServer_USBTTYDevice_Up(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBTTYDevice_List_Next(ud);
      } /* while */
    }
    cs->lastUsbTtyScan=time(0);
  }

  return 0;
}




int LC_CardServer__USBDeviceToDB(LC_USBDEVICE *ud,
                                 GWEN_DB_NODE *dbDrivers,
                                 GWEN_DB_NODE *dbDriverStore,
                                 GWEN_DB_NODE *dbReaderStore) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
        if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
          if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
                                              "comType", 0, "serial"),
                         "USB")==0) {
            if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
                 (int)LC_USBDevice_GetVendorId(ud)) &&
                (GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
                 (int)LC_USBDevice_GetProductId(ud))) {
              /* reader found */
              break;
            }
          }
        }
        dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    /* found reader and driver */

    /* delete all reader sections here */
    GWEN_DB_ClearGroup(dbDriverStore, 0);
    GWEN_DB_AddGroupChildren(dbDriverStore, dbDriver);
    while(!GWEN_DB_DeleteGroup(dbDriverStore, "reader"));
    GWEN_DB_AddGroupChildren(dbReaderStore, dbReader);
  }
  else {
    DBG_INFO(0, "Device %02x/%02x is not a known reader",
             LC_USBDevice_GetVendorId(ud),
             LC_USBDevice_GetProductId(ud));
    return -1;
  }

  return 0;
}



int LC_CardServer__USBTTYDeviceToDB(LC_USBTTYDEVICE *ud,
                                    GWEN_DB_NODE *dbDrivers,
                                    GWEN_DB_NODE *dbDriverStore,
                                    GWEN_DB_NODE *dbReaderStore) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USBSerial")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBTTYDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBTTYDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    /* found reader and driver */

    /* delete all reader sections */
    GWEN_DB_ClearGroup(dbDriverStore, 0);
    GWEN_DB_AddGroupChildren(dbDriverStore, dbDriver);
    while(!GWEN_DB_DeleteGroup(dbDriverStore, "reader"));
    GWEN_DB_AddGroupChildren(dbReaderStore, dbReader);
  }
  else {
    DBG_INFO(0, "Device %02x/%02x is not a known reader",
	     LC_USBTTYDevice_GetVendorId(ud),
             LC_USBTTYDevice_GetProductId(ud));
    return -1;
  }

  return 0;
}




int LC_CardServer_GetUSBDevices(GWEN_DB_NODE *dbKnownDrivers,
                                GWEN_DB_NODE *dbReaders) {
  int rv;
  LC_USBMONITOR *usbMonitor;
  LC_USBTTYMONITOR *usbTtyMonitor;

  usbTtyMonitor=LC_USBTTYMonitor_new();
  usbMonitor=LC_USBMonitor_new();

  rv=LC_USBMonitor_Scan(usbMonitor);
  if (rv==-1) {
    DBG_INFO(0, "Error scanning USB bus");
  }
  else if (rv==1) {
    DBG_VERBOUS(0, "No changes on USB bus");
  }
  else {
    LC_USBDEVICE_LIST *newDevices;
    LC_USBDEVICE *ud;

    newDevices=LC_USBMonitor_GetNewDevices(usbMonitor);
    assert(newDevices);
    ud=LC_USBDevice_List_First(newDevices);
    while(ud) {
      GWEN_DB_NODE *dbReaderStore;
      GWEN_DB_NODE *dbDriverStore;

      DBG_DEBUG(0, "Device %02x/%02x found",
                LC_USBDevice_GetVendorId(ud),
                LC_USBDevice_GetProductId(ud));
      dbReaderStore=GWEN_DB_Group_new("readerStore");
      dbDriverStore=GWEN_DB_Group_new("driverStore");

      if (LC_CardServer__USBDeviceToDB(ud,
                                       dbKnownDrivers,
                                       dbDriverStore,
                                       dbReaderStore)) {
        DBG_INFO(0, "here");
      }
      else {
        GWEN_DB_NODE *dbReader;
        const char *p;

        dbReader=GWEN_DB_Group_new("reader");
        p=GWEN_DB_GetCharValue(dbDriverStore, "manufacturer", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "manufacturer", p);
        p=GWEN_DB_GetCharValue(dbReaderStore, "shortName", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "name", p);
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "type", "USB");
        GWEN_DB_AddGroup(dbReaders, dbReader);
      }
      ud=LC_USBDevice_List_Next(ud);
    } /* while */
  }

  if (LC_USBTTYMonitor_Scan(usbTtyMonitor)) {
    DBG_INFO(0, "Error scanning USB bus (serial)");
  }
  else {
    LC_USBTTYDEVICE_LIST *newDevices;
    LC_USBTTYDEVICE *ud;

    newDevices=LC_USBTTYMonitor_GetNewDevices(usbTtyMonitor);
    assert(newDevices);
    ud=LC_USBTTYDevice_List_First(newDevices);
    while(ud) {
      GWEN_DB_NODE *dbReaderStore=0;
      GWEN_DB_NODE *dbDriverStore=0;

      DBG_DEBUG(0, "Device %02x/%02x found",
                LC_USBTTYDevice_GetVendorId(ud),
                LC_USBTTYDevice_GetProductId(ud));
      if (LC_CardServer__USBTTYDeviceToDB(ud,
                                          dbKnownDrivers,
                                          dbDriverStore,
                                          dbReaderStore)) {
        DBG_INFO(0, "here");
      }
      else {
        GWEN_DB_NODE *dbReader;
        const char *p;

        dbReader=GWEN_DB_Group_new("reader");
        p=GWEN_DB_GetCharValue(dbDriverStore, "manufacturer", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "manufacturer", p);
        p=GWEN_DB_GetCharValue(dbReaderStore, "shortName", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "name", p);
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "type", "USB-Serial");
        GWEN_DB_AddGroup(dbReaders, dbReader);
      }
      ud=LC_USBTTYDevice_List_Next(ud);
    } /* while */
  }

  LC_USBMonitor_free(usbMonitor);
  LC_USBTTYMonitor_free(usbTtyMonitor);
  return 0;
}


