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

#include <hal/libhal.h>
#include <dbus/dbus.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



LC_DEVSCANNER *LC_HalScanner_new() {
  LC_DEVSCANNER *sc;

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_HalScanner_ReadDevs);

  return sc;
}



int LC_HalScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  DBusError dbus_error;
  DBusConnection *dbus_conn;
  LibHalContext *ctx;
  char **devices;
  int i_devices, i;
  int count=0;
  
  dbus_error_init(&dbus_error);
  dbus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    DBG_ERROR(0,
	      "Could not connect to system bus [%s]",
	      dbus_error.message);
    dbus_error_free(&dbus_error);
    return GWEN_ERROR_IO;
  }
  
  ctx=libhal_ctx_new();
  if (ctx==NULL) {
    DBG_ERROR(0, "Could not create HAL context");
    dbus_error_free(&dbus_error);
    return GWEN_ERROR_IO;
  }

  libhal_ctx_set_dbus_connection(ctx, dbus_conn);

  devices=libhal_get_all_devices(ctx, &i_devices, &dbus_error);
  if (devices==NULL) {
    DBG_INFO(0, "HAL not running: %s", dbus_error.message);
    dbus_error_free (&dbus_error);
    /*libhal_ctx_shutdown (ctx, NULL);*/
    libhal_ctx_free (ctx);
    return GWEN_ERROR_IO;
  }
  if (i_devices<1) {
    DBG_INFO(0, "HAL returned an empty device list, this can't be right...");
  }

  for (i=0; i<i_devices; i++) {
    const char *udi=devices[i];

    if (libhal_device_exists(ctx, udi, &dbus_error)) {
      const char *busType;

      busType=libhal_device_get_property_string(ctx, udi, "info.subsystem", NULL);
      if (busType && (strcasecmp(busType, "usb")!=0))
        busType=NULL; /* non-USB devices are handled below */

      if (busType==NULL)
	busType=libhal_device_get_property_string(ctx, udi, "info.bus", NULL);
      if (busType) {
	if (strcasecmp(busType, "usb")==0) {
	  /* USB device, look for LibUSB info */
	  if (libhal_device_property_exists(ctx, udi, "usb.bus_number", NULL) &&
	      libhal_device_property_exists(ctx, udi, "usb.linux.device_number", NULL)){
	    LC_DEVICE *d;
	    int busId;
	    int busPos;
	    int vendorId;
	    int productId;
	    char pbuff[256];
	    struct stat st;
	    int havePath=0;

	    busId=libhal_device_get_property_int(ctx,
						 udi,
						 "usb.bus_number",
						 NULL);
	    busPos=libhal_device_get_property_int(ctx,
						  udi,
						  "usb.linux.device_number",
						  NULL);
	    vendorId=libhal_device_get_property_int(ctx,
						    udi,
						    "usb.vendor_id",
						    NULL);
	    productId=libhal_device_get_property_int(ctx,
						     udi,
						     "usb.product_id",
						     NULL);

	    d=LC_Device_new(LC_Device_BusType_UsbRaw,
			    busId, busPos,
			    vendorId, productId);
	    LC_Device_SetDevicePos(d, count++);

	    LC_Device_SetHalPath(d, udi);

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

      } /* if bus type */
      else {
	const char *subsys;

	subsys=libhal_device_get_property_string(ctx, udi, "linux.subsystem", NULL);
	if (subsys) {
	  if (strcasecmp(subsys, "tty")==0) {
	    char *parent_udi;

	    /* ttyUSB device, get USB info from parent */
	    parent_udi=libhal_device_get_property_string(ctx,
							 udi,
							 "info.parent",
							 NULL);
	    if (parent_udi) {
	      if (libhal_device_property_exists(ctx, parent_udi, "usb.bus_number", NULL) &&
		  libhal_device_property_exists(ctx, parent_udi, "usb.linux.device_number", NULL)){
		LC_DEVICE *d;
		int busPos;
		int vendorId;
		int productId;
                const char *path;

		busPos=libhal_device_get_property_int(ctx,
						      udi,
						      "serial.port",
						      NULL);
		vendorId=libhal_device_get_property_int(ctx,
							parent_udi,
							"usb.vendor_id",
							NULL);
		productId=libhal_device_get_property_int(ctx,
							 parent_udi,
							 "usb.product_id",
							 NULL);
		path=libhal_device_get_property_string(ctx,
						       udi,
						       "serial.device",
						       NULL);

		d=LC_Device_new(LC_Device_BusType_Tty,
				0, busPos,
				vendorId, productId);
		LC_Device_SetDevicePos(d, count++);

		LC_Device_SetHalPath(d, udi);
                if (path)
		  LC_Device_SetPath(d, path);

		/* all set, add device */
		LC_Device_List_Add(d, dl);
	      } /* if USB info exists in parent */
	      else if (libhal_device_property_exists(ctx, parent_udi, "pcmcia.manf_id", NULL) &&
		       libhal_device_property_exists(ctx, parent_udi, "pcmcia.card_id", NULL)){
		LC_DEVICE *d;
		int busPos;
		int vendorId;
		int productId;
		const char *path;
    
		busPos=libhal_device_get_property_int(ctx,
						      parent_udi,
						      "pcmcia.socket_number",
						      NULL);
		vendorId=libhal_device_get_property_int(ctx,
							parent_udi,
							"pcmcia.manf_id",
							NULL);
		productId=libhal_device_get_property_int(ctx,
							 parent_udi,
							 "pcmcia.card_id",
							 NULL);
		path=libhal_device_get_property_string(ctx,
						       udi,
						       "serial.device",
						       NULL);

		d=LC_Device_new(LC_Device_BusType_Tty,
				0, busPos,
				vendorId, productId);
		LC_Device_SetDevicePos(d, count++);
    
		LC_Device_SetHalPath(d, udi);
		if (path)
		  LC_Device_SetPath(d, path);

		/* all set, add device */
		LC_Device_List_Add(d, dl);
	      } /* if PCMCIA info exists in parent */
	    } /* if parent */
	  } /* if tty */
	  else if (strcasecmp(subsys, "pcmcia")==0) {
	    LC_DEVICE *d;
	    int busPos;
	    int vendorId;
	    int productId;
	    char *path;
	    char *name = NULL;

	    busPos=libhal_device_get_property_int(ctx,
						  udi,
						  "pcmcia.socket_number",
						  NULL);
	    vendorId=libhal_device_get_property_int(ctx,
						    udi,
						    "pcmcia.manf_id",
						    NULL);
	    productId=libhal_device_get_property_int(ctx,
						     udi,
						     "pcmcia.card_id",
						     NULL);
	    path=libhal_device_get_property_string(ctx,
						   udi,
						   "linux.sysfs_path",
						   NULL);
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
	    if (path)
	      LC_Device_SetPath(d, path);
	    if (name)
	      LC_Device_SetDeviceName(d, name);

	    /* all set, add device */
            /*
            DBG_DEBUG(0, "Adding device %d (%04x/%04x) with path '%s' and name '%s' (UDI: '%s')",
            busPos, vendorId, productId, path, name, udi);
            */
	    LC_Device_List_Add(d, dl);
	  } /* if PCMCIA */
	} /* if subsys */

      }
    } /* if device exists */
  } /* for */

  dbus_error_free(&dbus_error);
  /*libhal_ctx_shutdown(ctx, NULL);*/
  libhal_ctx_free(ctx);
  
  return 0;
}


#endif

