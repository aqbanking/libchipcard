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


#include "usbrawscanner_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>


#ifdef USE_LIBSYSFS
# include <sysfs/libsysfs.h>
#endif

#ifdef USE_LIBHAL
# include <hal/libhal.h>
# include <dbus/dbus.h>
#endif


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



LC_DEVSCANNER *LC_UsbRawScanner_new() {
  LC_DEVSCANNER *sc;

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_UsbRawScanner_ReadDevs);

  return sc;
}


#ifdef USE_LIBHAL

int LC_UsbRawScanner_FillHalName(LC_DEVICE *dev) {
  DBusError dbus_error;
  DBusConnection *dbus_conn;
  LibHalContext *ctx;
  char **devices;
  int i_devices, i;
  int foundDev=0;
  
  dbus_error_init(&dbus_error);
  dbus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "Could not connect to system bus [%s]",
	      dbus_error.message);
    dbus_error_free(&dbus_error);
    return GWEN_ERROR_IO;
  }
  
  ctx=libhal_ctx_new();
  if (ctx==NULL) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not create HAL context");
    dbus_error_free(&dbus_error);
    return NULL;
  }

  libhal_ctx_set_dbus_connection(ctx, dbus_conn);

  devices=libhal_get_all_devices(ctx, &i_devices, &dbus_error);
  if (devices==NULL) {
    DBG_INFO(LC_LOGDOMAIN, "HAL not running: %s", dbus_error.message);
    dbus_error_free (&dbus_error);
    /*libhal_ctx_shutdown (ctx, NULL);*/
    libhal_ctx_free (ctx);
    return GWEN_ERROR_IO;
  }

  for (i=0; i<i_devices; i++) {
    const char *udi=devices[i];

    if (libhal_device_exists(ctx, udi, &dbus_error)) {
      if (libhal_device_property_exists(ctx, udi, "usb.bus_number", NULL) &&
	  libhal_device_property_exists(ctx, udi, "usb.linux.device_number", NULL)){
	int busNumber;
	int busPos;

	busNumber=libhal_device_get_property_int(ctx,
						 udi,
						 "usb.bus_number",
						 NULL);
	busPos=libhal_device_get_property_int(ctx,
					      udi,
					      "usb.linux.device_number",
					      NULL);
	LC_Device_SetHalPath(dev, udi);
        foundDev=1;
	break;
      }
    }
    else {
      DBG_INFO(LC_LOGDOMAIN, "Device [%s] does not exist", udi);
      break;
    }
  }

  dbus_error_free(&dbus_error);
  /*libhal_ctx_shutdown(ctx, NULL);*/
  libhal_ctx_free(ctx);
  
  if (!deviceFound) {
    DBG_INFO(LC_LOGDOMAIN, "Device not found");
    return GWEN_ERROR_IO;
  }

  return 0;
}


#endif




#ifndef USE_LIBSYSFS

int LC_UsbRawScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  DBG_VERBOUS(0, "UsbRaw scanner not supported (no LibSysFS)");
  return 0;
}



#else

int LC_UsbRawScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  struct sysfs_bus *bus = NULL;
  struct sysfs_device *curdev = NULL;
#ifndef HAVE_SYSFS2
  struct sysfs_device *temp_device = NULL;
#endif
  struct sysfs_device *parent = NULL;
  struct sysfs_attribute *cur = NULL;
  struct dlist *devlist = NULL;
  struct dlist *attributes = NULL;
  int count=0;

  bus = sysfs_open_bus("usb");
  if (bus == NULL) {
    DBG_ERROR(0,"Error accessing sysfs");
    return -1;
  }

  devlist = sysfs_get_bus_devices(bus);
  if (devlist != NULL) {
    dlist_for_each_data(devlist, curdev, 
                        struct sysfs_device) {
      /* found something, well maybe */
      parent = sysfs_get_device_parent(curdev);
      if (parent) {
	int hasIds=0;
	int busPos=0;
	int vendorId=0, productId=0;

	attributes = sysfs_get_device_attributes(parent);
	dlist_for_each_data(attributes, cur,
			    struct sysfs_attribute) {
	  if (strcmp(cur->name,"idVendor")==0) {
	    if (cur->value != NULL) {
	      vendorId = strtol(cur->value, NULL, 16);
	      hasIds|=0x01;
	    }
	    else {
	      fprintf(stderr,"idVendor empty\n");
	    }
	  }
	  else if (strcmp(cur->name,"idProduct")==0) {
	    if (cur->value != NULL) {
	      productId = strtol(cur->value, NULL, 16);
	      hasIds|=0x02;
	    }
	    else {
	      fprintf(stderr, "idProduct empty\n");
	    }
	  }
	  else if (strcmp(cur->name,"devnum")==0) {
	    if (cur->value != NULL) {
	      busPos=strtol(cur->value, NULL, 10);
	      hasIds|=0x04;
	    }
	    else {
	      fprintf(stderr, "idProduct empty\n");
	    }
	  }
	} /* for attributes */

	if (hasIds==0x07) {
	  char *s;
	  char *p;
	  int busId=0;

	  /* try to get bus id */
	  s=strdup(curdev->bus_id);
	  p=strchr(s, '-');
	  if (p) {
	    *p=0;
	    if (isdigit(*s))
	      busId=atoi(s);
	  }
	  free(s);

	  if (busId) {
	    char pbuff[256];
	    struct stat st;
	    int havePath=0;

	    snprintf(pbuff, sizeof(pbuff),
		     "/dev/bus/usb/%03d/%03d",
		     busId, busPos);
	    if (stat(pbuff, &st)==0) {
	      havePath=1;
	    }
	    else {
	      snprintf(pbuff, sizeof(pbuff),
		       "/proc/bus/usb/%03d/%03d",
		       busId, busPos);
	      if (stat(pbuff, &st)==0) {
		havePath=1;
	      }
	    }

	    if (havePath) {
	      LC_DEVICE *d;
	      char *p;

	      /* create device */
	      d=LC_Device_new(LC_Device_BusType_UsbRaw,
			      busId, busPos,
			      vendorId, productId);
	      LC_Device_SetDevicePos(d, count++);
	      LC_Device_SetPath(d, pbuff);
	      p=strrchr(pbuff, '/');
	      if (p) {
		LC_Device_SetDeviceName(d, p+1);
		*p=0;
		p=strrchr(pbuff, '/');
		if (p) {
		  LC_Device_SetBusName(d, p+1);
		  *p=0;
		}
	      }
	      LC_Device_List_Add(d, dl);
	    }
	    else {
	      fprintf(stderr, "No path for %04x/%04x at %03d/%03d.\n",
		      vendorId, productId, busId, busPos);
	    }
	  }
	}
      }
      else {
	fprintf(stderr,"Error getting device parent for USB\n");
      }
    }
  }
  sysfs_close_bus(bus);
  return 0;
}

#endif



