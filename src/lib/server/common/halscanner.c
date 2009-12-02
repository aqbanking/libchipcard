/***************************************************************************
    begin       : Thu Sep 02 2008
    copyright   : (C) 2008 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef USE_HAL


#include "halscanner_p.h"
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


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



GWEN_INHERIT(LC_DEVSCANNER, LC_HALSCANNER);




LC_DEVSCANNER *LC_HalScanner_new() {
  LC_DEVSCANNER *sc;
  LC_HALSCANNER *xsc;

  sc=LC_DevScanner_new();
  GWEN_NEW_OBJECT(LC_HALSCANNER, xsc);
  GWEN_INHERIT_SETDATA(LC_DEVSCANNER, LC_HALSCANNER, sc, xsc, LC_HalScanner_FreeData);
  LC_DevScanner_SetReadDevsFn(sc, LC_HalScanner_ReadDevs);

  dbus_error_init(&(xsc->dbus_error));
  xsc->dbus_conn=dbus_bus_get (DBUS_BUS_SYSTEM, &(xsc->dbus_error));
  if (dbus_error_is_set(&(xsc->dbus_error))) {
    DBG_ERROR(0,
	      "Could not connect to system bus [%s]",
	      xsc->dbus_error.message);
    LC_DevScanner_free(sc);
    return NULL;
  }

  xsc->ctx=libhal_ctx_new();
  if (xsc->ctx==NULL) {
    DBG_ERROR(0, "Could not create HAL context");
    LC_DevScanner_free(sc);
    return NULL;
  }

  if (!libhal_ctx_set_dbus_connection(xsc->ctx, xsc->dbus_conn)) {
    DBG_ERROR(0, "Failed to set dbus connection for HAL context (is the HAL daemon running?)");
    LC_DevScanner_free(sc);
    return NULL;
  }

  if (!libhal_ctx_init(xsc->ctx, &(xsc->dbus_error))) {
    DBG_ERROR(0, "Failed to initialize HAL context (is the HAL daemon running?)");
    LC_DevScanner_free(sc);
    return NULL;
  }

  return sc;
}



void LC_HalScanner_FreeData(void *bp, void *p) {
  LC_HALSCANNER *xsc;

  xsc=(LC_HALSCANNER*) p;
  DBG_INFO(0, "Closing HAL scanner");

  dbus_error_free(&(xsc->dbus_error));
  if (xsc->dbus_conn) {
    dbus_connection_unref(xsc->dbus_conn);
    xsc->dbus_conn = NULL;
  }
  /*libhal_ctx_shutdown(ctx, NULL);*/
  libhal_ctx_free(xsc->ctx);

  GWEN_FREE_OBJECT(xsc);
}



int LC_HalScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  char **devices;
  int i_devices, i;
  int count=0;
  LC_HALSCANNER *xsc;

  assert(sc);
  xsc=GWEN_INHERIT_GETDATA(LC_DEVSCANNER, LC_HALSCANNER, sc);
  assert(xsc);

  devices=libhal_get_all_devices(xsc->ctx, &i_devices, &(xsc->dbus_error));
  if (devices==NULL) {
    DBG_INFO(0, "HAL not running: %s", xsc->dbus_error.message);
    return GWEN_ERROR_IO;
  }
  if (i_devices<1) {
    DBG_INFO(0, "HAL returned an empty device list, this can't be right...");
  }

  for (i=0; i<i_devices; i++) {
    const char *udi=devices[i];

    if (libhal_device_exists(xsc->ctx, udi, &(xsc->dbus_error))) {
      char *busType;

      busType=libhal_device_get_property_string(xsc->ctx, udi, "info.subsystem", NULL);
      if (busType && (strcasecmp(busType, "usb")!=0)) {
	libhal_free_string(busType);
	busType=NULL; /* non-USB devices are handled below */
      }

      if (busType==NULL)
	busType=libhal_device_get_property_string(xsc->ctx, udi, "info.bus", NULL);
      if (busType) {
	if (strcasecmp(busType, "usb")==0) {
	  /* USB device, look for LibUSB info */
	  if (libhal_device_property_exists(xsc->ctx, udi, "usb.bus_number", NULL) &&
	      libhal_device_property_exists(xsc->ctx, udi, "usb.linux.device_number", NULL)){
	    LC_DEVICE *d;
	    int busId;
	    int busPos;
	    int vendorId;
	    int productId;
	    int usbClass=0;
	    char pbuff[256];
	    struct stat st;
	    int havePath=0;

	    busId=libhal_device_get_property_int(xsc->ctx,
						 udi,
						 "usb.bus_number",
						 NULL);
	    busPos=libhal_device_get_property_int(xsc->ctx,
						  udi,
						  "usb.linux.device_number",
						  NULL);
	    vendorId=libhal_device_get_property_int(xsc->ctx,
						    udi,
						    "usb.vendor_id",
						    NULL);
	    productId=libhal_device_get_property_int(xsc->ctx,
						     udi,
						     "usb.product_id",
						     NULL);

	    if (libhal_device_property_exists(xsc->ctx, udi, "usb.interface.class", NULL))
	      usbClass=libhal_device_get_property_int(xsc->ctx,
						      udi,
						      "usb.interface.class",
						      NULL);

	    d=LC_Device_new(LC_Device_BusType_UsbRaw,
			    busId, busPos,
			    vendorId, productId);
	    LC_Device_SetDevicePos(d, count++);

	    LC_Device_SetHalPath(d, udi);
	    LC_Device_SetUsbClass(d, usbClass);

	    /* determine path for LibUSB */
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
	      char *p;

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
	    }

	    /* all set, add device */
	    LC_Device_List_Add(d, dl);
	  }
	} /* if USB */
	libhal_free_string(busType);
      } /* if bus type */
      else {
	char *subsys;

	subsys=libhal_device_get_property_string(xsc->ctx, udi, "linux.subsystem", NULL);
	if (subsys) {
	  if (strcasecmp(subsys, "tty")==0) {
	    char *parent_udi;

	    /* ttyUSB device, get USB info from parent */
	    parent_udi=libhal_device_get_property_string(xsc->ctx,
							 udi,
							 "info.parent",
							 NULL);
	    if (parent_udi) {
	      if (libhal_device_property_exists(xsc->ctx, parent_udi, "usb.bus_number", NULL) &&
		  libhal_device_property_exists(xsc->ctx, parent_udi, "usb.linux.device_number", NULL)){
		LC_DEVICE *d;
		int busPos;
		int vendorId;
		int productId;
                char *path;

		busPos=libhal_device_get_property_int(xsc->ctx,
						      udi,
						      "serial.port",
						      NULL);
		vendorId=libhal_device_get_property_int(xsc->ctx,
							parent_udi,
							"usb.vendor_id",
							NULL);
		productId=libhal_device_get_property_int(xsc->ctx,
							 parent_udi,
							 "usb.product_id",
							 NULL);
		path=libhal_device_get_property_string(xsc->ctx,
						       udi,
						       "serial.device",
						       NULL);

		d=LC_Device_new(LC_Device_BusType_Tty,
				0, busPos,
				vendorId, productId);
		LC_Device_SetDevicePos(d, count++);

		LC_Device_SetHalPath(d, udi);
		if (path) {
		    LC_Device_SetPath(d, path);
		    libhal_free_string(path);
		}

		/* all set, add device */
		LC_Device_List_Add(d, dl);
	      } /* if USB info exists in parent */
	      else if (libhal_device_property_exists(xsc->ctx, parent_udi, "pcmcia.manf_id", NULL) &&
		       libhal_device_property_exists(xsc->ctx, parent_udi, "pcmcia.card_id", NULL)){
		LC_DEVICE *d;
		int busPos;
		int vendorId;
		int productId;
		char *path;
    
		busPos=libhal_device_get_property_int(xsc->ctx,
						      parent_udi,
						      "pcmcia.socket_number",
						      NULL);
		vendorId=libhal_device_get_property_int(xsc->ctx,
							parent_udi,
							"pcmcia.manf_id",
							NULL);
		productId=libhal_device_get_property_int(xsc->ctx,
							 parent_udi,
							 "pcmcia.card_id",
							 NULL);
		path=libhal_device_get_property_string(xsc->ctx,
						       udi,
						       "serial.device",
						       NULL);

		d=LC_Device_new(LC_Device_BusType_Tty,
				0, busPos,
				vendorId, productId);
		LC_Device_SetDevicePos(d, count++);
    
		LC_Device_SetHalPath(d, udi);
		if (path) {
		  LC_Device_SetPath(d, path);
		  libhal_free_string(path);
		}

		/* all set, add device */
		LC_Device_List_Add(d, dl);
	      } /* if PCMCIA info exists in parent */
	      libhal_free_string(parent_udi);
	    } /* if parent */
	  } /* if tty */
	  else if (strcasecmp(subsys, "pcmcia")==0) {
	    LC_DEVICE *d;
	    int busPos;
	    int vendorId;
	    int productId;
	    char *path;
	    char *name = NULL;

	    busPos=libhal_device_get_property_int(xsc->ctx,
						  udi,
						  "pcmcia.socket_number",
						  NULL);
	    vendorId=libhal_device_get_property_int(xsc->ctx,
						    udi,
						    "pcmcia.manf_id",
						    NULL);
	    productId=libhal_device_get_property_int(xsc->ctx,
						     udi,
						     "pcmcia.card_id",
						     NULL);
	    path=libhal_device_get_property_string(xsc->ctx,
						   udi,
						   "linux.sysfs_path",
						   NULL);
	    /* TODO: maybe this is no longer needed? We no longer support libsysfs anyway */
	    if (path) {
	      name=strrchr(path, '/');
	      if (name)
	        name++;
	    }
  
	    d=LC_Device_new(LC_Device_BusType_Pcmcia,
			    0, busPos,
			    vendorId, productId);
	    LC_Device_SetDevicePos(d, count++);

	    LC_Device_SetHalPath(d, udi);
	    if (path) {
	      LC_Device_SetPath(d, path);
	      libhal_free_string(path);
	    }
	    if (name)
	      LC_Device_SetDeviceName(d, name);

	    /* all set, add device */
            /*
            DBG_DEBUG(0, "Adding device %d (%04x/%04x) with path '%s' and name '%s' (UDI: '%s')",
            busPos, vendorId, productId, path, name, udi);
            */
	    LC_Device_List_Add(d, dl);
	  } /* if PCMCIA */
	  libhal_free_string(subsys);
	} /* if subsys */

      }
    } /* if device exists */
  } /* for */

  libhal_free_string_array(devices);

  return 0;
}


#endif

