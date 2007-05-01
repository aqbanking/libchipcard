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


#include "pcmciascanner_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/stringlist.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#ifdef USE_LIBSYSFS
# include <sysfs/libsysfs.h>
#endif


LC_DEVSCANNER *LC_PcmciaScanner_new() {
  LC_DEVSCANNER *sc;
#ifdef USE_LIBSYSFS
  char sysfspath[256];
#endif

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_PcmciaScanner_ReadDevs);

#ifdef USE_LIBSYSFS
  if (! sysfs_get_mnt_path(sysfspath, sizeof(sysfspath))) {
    DBG_NOTICE(0, "Will use sysfs to scan for ttyUSB devices")
  }
#endif

  return sc;
}



/* this function has been submitted by Thomas Viehmann. Thanks ;-) */
int LC_PcmciaScanner_ScanSysFS_Pcmcia(LC_DEVICE_LIST *dl) {
#ifndef USE_LIBSYSFS
  DBG_INFO(0, "LibSysFS not supsknumed");
  return -1;
#else
  struct sysfs_bus *bus = NULL;
  struct sysfs_device *curdev = NULL;
#ifndef HAVE_SYSFS2
  struct sysfs_device *temp_device = NULL;
#endif
  struct sysfs_attribute *cur = NULL;
  struct dlist *devlist = NULL;
  struct dlist *attributes = NULL;
  int sknum=0, vendorId=0, productId=0;
  LC_DEVICE *currentDevice;

  bus = sysfs_open_bus("pcmcia");
  if (bus == NULL) {
    DBG_DEBUG(0,"No PCMCIA bus");
    return 0;
  }

  devlist = sysfs_get_bus_devices(bus);
  if (devlist != NULL) {
    dlist_for_each_data(devlist, curdev, 
                        struct sysfs_device) {
	  sknum = strtol(curdev->bus_id, NULL, 16);
          attributes = sysfs_get_device_attributes(curdev);
          dlist_for_each_data(attributes, cur, 
                              struct sysfs_attribute) {
            if (strcmp(cur->name,"manf_id")==0) {
              if (cur->value != NULL) {
                vendorId = strtol(cur->value, NULL, 16);
              }
              else {
		DBG_ERROR(0,"manf_id empty");
	      }
	    }
            if (strcmp(cur->name,"card_id")==0) {
              if (cur->value != NULL) {
                productId = strtol(cur->value, NULL, 16);
              }
	      else {
		DBG_ERROR(0, "card_id empty");
	      }
            }
          }
          currentDevice=LC_Device_new(LC_Device_BusType_Pcmcia,
                                      0,
                                      sknum,
                                      vendorId, productId);
          DBG_DEBUG(0, "Adding device %d (%04x/%04x)",
                    sknum,
                    vendorId,
                    productId);
          LC_Device_SetDevicePos(currentDevice, sknum);
          LC_Device_List_Add(currentDevice, dl);
    }	  
  }
  sysfs_close_bus(bus);
  return 0;
#endif
}


int LC_PcmciaScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  if ( LC_PcmciaScanner_ScanSysFS_Pcmcia(dl) ) {
    DBG_DEBUG(0, "here");
    return -1;
  }

  return 0;
}



