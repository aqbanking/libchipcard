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


#include "usbttyscanner_p.h"
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

static const char *lc_usbttyscanner_filename=0;
static int lc_usbttyscanner_sysfs=0;



LC_DEVSCANNER *LC_UsbTtyScanner_new() {
  LC_DEVSCANNER *sc;
  FILE *f;
#ifdef USE_LIBSYSFS
  char sysfspath[256];
#endif

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_UsbTtyScanner_ReadDevs);

  f=fopen(LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL_FILE, "r");
  if (f) {
    fclose(f);
    lc_usbttyscanner_filename=LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL_FILE;
    DBG_NOTICE(0,
               "USB: Using proc file for kernel <2.6 for ttyUSB support");
  }
  else {
    f=fopen(LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL2_6_FILE, "r");
    if (f) {
      fclose(f);
      lc_usbttyscanner_filename=LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL2_6_FILE;
      DBG_NOTICE(0,
                 "USB: Using proc file for kernel >=2.6 for ttyUSB support");
    }
#ifdef USE_LIBSYSFS
    else {
      if (! sysfs_get_mnt_path(sysfspath, sizeof(sysfspath))) {
        lc_usbttyscanner_sysfs = 1;
        DBG_NOTICE(0, "Will use sysfs to scan for ttyUSB devices")
      }
    }
#endif
  }
  if (!f && !lc_usbttyscanner_sysfs) {
    DBG_ERROR(0, "Unable to open USB-serial file: %s",
              strerror(errno));
  }

  return sc;
}



/* this function has been submitted by Thomas Viehmann. Thanks ;-) */
int LC_UsbTtyScanner_ScanSysFS_UsbSerial(LC_DEVICE_LIST *dl) {
#ifndef USE_LIBSYSFS
  DBG_INFO(0, "LibSysFS not supported");
  return -1;
#else
  struct sysfs_bus *bus = NULL;
  struct sysfs_device *curdev = NULL;
  struct sysfs_device *temp_device = NULL;
  struct sysfs_device *parent = NULL;
  struct sysfs_device *child = NULL;
  struct sysfs_attribute *cur = NULL;
  struct dlist *devlist = NULL;
  struct dlist *attributes = NULL;
  int port=0, vendorId=0, productId=0;
  LC_DEVICE *currentDevice;

  bus = sysfs_open_bus("usb");
  if (bus == NULL) {
    DBG_ERROR(0,"Error accessing sysfs");
    return -1;
  }

  devlist = sysfs_get_bus_devices(bus);
  if (devlist != NULL) {
    dlist_for_each_data(devlist, curdev, 
                        struct sysfs_device) {
      /* for each device: look for ttyUSB in children*/
      port = -1;
      temp_device = sysfs_open_device_tree(curdev->path);
      if (temp_device) {
        if (temp_device->children) {
          dlist_for_each_data(temp_device->children, child,
                              struct sysfs_device) {
            if (strncmp(child->name, "ttyUSB", 6) == 0)
              port = atoi(&child->name[6]);
          }
        }
        sysfs_close_device_tree(temp_device);
      }
      /* found something, well maybe */
      if (port != -1) {
	parent = sysfs_get_device_parent(curdev);
	if (parent) {	
          attributes = sysfs_get_device_attributes(parent);
          dlist_for_each_data(attributes, cur, 
                              struct sysfs_attribute) {
            if (strcmp(cur->name,"idVendor")==0) {
              if (cur->value != NULL) {
                vendorId = strtol(cur->value, NULL, 16);
              }
              else {
		DBG_ERROR(0,"idVendor empty");
	      }
	    }
            if (strcmp(cur->name,"idProduct")==0) {
              if (cur->value != NULL) {
                productId = strtol(cur->value, NULL, 16);
              }
	      else {
		DBG_ERROR(0, "idProduct empty");
	      }
            }
          }
          currentDevice=LC_Device_new(LC_Device_BusType_UsbTty,
                                      0,
                                      port,
                                      vendorId, productId);
          DBG_DEBUG(0, "Adding device %d (%04x/%04x)",
                    port,
                    vendorId,
                    productId);
          LC_Device_List_Add(currentDevice, dl);
        }
        else {
          DBG_ERROR(0,"Error getting device parent for ttyUSB");
        }
      }
    }
  }
  sysfs_close_bus(bus);
  return 0;
#endif
}



int LC_UsbTtyScanner_Read_ProcTtyDriverUsbSerial(LC_DEVICE_LIST *dl) {
  FILE *f;
  char linebuf[256];
  int first;
  GWEN_STRINGLIST *sl;

  if (lc_usbttyscanner_filename==0) {
    DBG_DEBUG(0, "USB-TTY interface not available (missing proc-file)");
    return -1;
  }
  f=fopen(lc_usbttyscanner_filename, "r");
  if (!f) {
    DBG_ERROR(0, "fopen(%s): %s",
              lc_usbttyscanner_filename,
              strerror(errno));
    return -1;
  }

  sl=GWEN_StringList_new();
  first=1;
  while(fgets(linebuf, sizeof(linebuf), f)) {
    if (first) {
      first=0;
    }
    else {
      unsigned int i;
      int port;
      int vendorId;
      int productId;
      char *p;
      char *path=0;
      LC_DEVICE *currentDevice;
      int isDouble;

      i=strlen(linebuf);
      if (i<2) {
        DBG_ERROR(0, "Bad line (too short)\n");
        GWEN_StringList_free(sl);
        fclose(f);
        return -1;
      }
      if (linebuf[i-1]=='\n')
        linebuf[i-1]=0;

      p=linebuf;

      /* get port */
      while(*p && *p!=':')
        p++;
      if (*p!=':') {
        DBG_ERROR(0, "Bad line");
        GWEN_StringList_free(sl);
        fclose(f);
        return -1;
      }
      *p=0;
      if (1!=sscanf(linebuf, "%i", &port)) {
        DBG_ERROR(0, "Bad port value");
        GWEN_StringList_free(sl);
        fclose(f);
        return -1;
      }
      p++;

      /* get variable/value pairs */
      while(*p) {
        char *pName;
        char *pValue;

        while(*p && *p<33) p++;
        if (!*p)
          break;

        /* read name */
        pName=p;
        while(*p && *p!=':') p++;
        if (*p!=':') {
          DBG_ERROR(0, "\":\" expected");
          GWEN_StringList_free(sl);
          fclose(f);
          return -1;
        }
        *p=0;
        p++;

        /* read value */
        while(*p && *p==' ') p++;
        if (!*p) {
          DBG_ERROR(0, "Value expected");
          GWEN_StringList_free(sl);
          fclose(f);
          return -1;
        }

        if (*p=='"') {
          p++;
          pValue=p;
          while(*p && *p!='"') p++;
          if (*p) {
            *p=0;
            p++;
          }
        }
        else {
          pValue=p;
          while(*p && *p!=' ') p++;
          if (*p) {
            *p=0;
            p++;
          }
        }

        /* handle variables */
        if (strcasecmp(pName, "vendor")==0) {
          if (1!=sscanf(pValue, "%x", &vendorId)) {
            DBG_ERROR(0, "Bad value for \"vendor\" (%s)", pValue);
            GWEN_StringList_free(sl);
            fclose(f);
            return -1;
          }
        }
        else if (strcasecmp(pName, "product")==0) {
          if (1!=sscanf(pValue, "%x", &productId)) {
            DBG_ERROR(0, "Bad value for \"product\" (%s)", pValue);
            GWEN_StringList_free(sl);
            fclose(f);
            return -1;
          }
        }
        else if (strcasecmp(pName, "path")==0) {
          path=pValue;
        }
      }

      isDouble=0;
      if (path)
        if (!GWEN_StringList_AppendString(sl, path, 0, 1))
          isDouble=1;
      if (isDouble) {
        DBG_DEBUG(0, "Device %d (%04x/%04x) already exists",
                  port,
                  vendorId,
                  productId);
      }
      else {
        currentDevice=LC_Device_new(LC_Device_BusType_UsbTty, 0,
                                    port, vendorId, productId);
        DBG_VERBOUS(0, "Adding device %d (%04x/%04x)",
                    port,
                    vendorId,
                    productId);
        LC_Device_List_Add(currentDevice, dl);
      }
    } /* if not first line */
  } /* while lines */

  GWEN_StringList_free(sl);
  fclose(f);
  return 0;
}


int LC_UsbTtyScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  if ((lc_usbttyscanner_sysfs &&
       LC_UsbTtyScanner_ScanSysFS_UsbSerial(dl))
      ||
      (!lc_usbttyscanner_sysfs &&
       LC_UsbTtyScanner_Read_ProcTtyDriverUsbSerial(dl))) {
    DBG_DEBUG(0, "here");
    return -1;
  }

  return 0;
}



