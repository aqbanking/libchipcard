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


#include "usbttymonitor_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/stringlist.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>


GWEN_LIST_FUNCTIONS(LC_USBTTYDEVICE, LC_USBTTYDevice)


static const char *lc_usbttymonitor_filename=0;


LC_USBTTYDEVICE *LC_USBTTYDevice_new(GWEN_TYPE_UINT32 port,
                                     GWEN_TYPE_UINT32 vendorId,
                                     GWEN_TYPE_UINT32 productId) {
  LC_USBTTYDEVICE *ud;

  GWEN_NEW_OBJECT(LC_USBTTYDEVICE, ud);
  GWEN_LIST_INIT(LC_USBTTYDEVICE, ud);

  ud->port=port;
  ud->vendorId=vendorId;
  ud->productId=productId;

  return ud;
}



void LC_USBTTYDevice_free(LC_USBTTYDEVICE *ud) {
  if (ud) {
    GWEN_LIST_FINI(LC_USBTTYDEVICE, ud);
    GWEN_FREE_OBJECT(ud);
  }
}




GWEN_TYPE_UINT32 LC_USBTTYDevice_GetPort(const LC_USBTTYDEVICE *ud){
  assert(ud);
  return ud->port;
}



GWEN_TYPE_UINT32 LC_USBTTYDevice_GetVendorId(const LC_USBTTYDEVICE *ud){
  assert(ud);
  return ud->vendorId;
}



GWEN_TYPE_UINT32 LC_USBTTYDevice_GetProductId(const LC_USBTTYDEVICE *ud){
  assert(ud);
  return ud->productId;
}





LC_USBTTYMONITOR *LC_USBTTYMonitor_new() {
  LC_USBTTYMONITOR *um;
  FILE *f;

  GWEN_NEW_OBJECT(LC_USBTTYMONITOR, um);
  um->currentDevices=LC_USBTTYDevice_List_new();
  um->newDevices=LC_USBTTYDevice_List_new();
  um->lostDevices=LC_USBTTYDevice_List_new();

  f=fopen(LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL_FILE, "r");
  if (f) {
    fclose(f);
    lc_usbttymonitor_filename=LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL_FILE;
  }
  else {
    f=fopen(LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL2_6_FILE, "r");
    if (f) {
      fclose(f);
      lc_usbttymonitor_filename=LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL2_6_FILE;
    }
  }
  if (!f) {
    DBG_ERROR(0, "Unable to open USB-serial file: %s",
              strerror(errno));
  }

  return um;
}



void LC_USBTTYMonitor_free(LC_USBTTYMONITOR *um) {
  if (um) {
    GWEN_IdList_free(um->lastList);

    LC_USBTTYDevice_List_free(um->currentDevices);
    LC_USBTTYDevice_List_free(um->newDevices);
    LC_USBTTYDevice_List_free(um->lostDevices);
    GWEN_FREE_OBJECT(um);
  }
}




int LC_USBTTYMonitor_Read_ProcTtyDriverUsbSerial(LC_USBTTYDEVICE_LIST *dl) {
  FILE *f;
  char linebuf[256];
  int first;
  GWEN_STRINGLIST *sl;

  if (lc_usbttymonitor_filename==0) {
    DBG_DEBUG(0, "USB-TTY interface not available (missing proc-file)");
    return -1;
  }
  f=fopen(lc_usbttymonitor_filename, "r");
  if (!f) {
    DBG_ERROR(0, "fopen(%s): %s",
              lc_usbttymonitor_filename,
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
      LC_USBTTYDEVICE *currentDevice;
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
        currentDevice=LC_USBTTYDevice_new(port, vendorId, productId);
        DBG_DEBUG(0, "Adding device %d (%04x/%04x)",
                  port,
                  vendorId,
                  productId);
        LC_USBTTYDevice_List_Add(currentDevice, dl);
      }
    } /* if not first line */
  } /* while lines */

  GWEN_StringList_free(sl);
  fclose(f);
  return 0;
}



LC_USBTTYDEVICE *LC_USBTTYDevice_Find(LC_USBTTYDEVICE_LIST *dl,
                                      GWEN_TYPE_UINT32 port,
                                      GWEN_TYPE_UINT32 vendorId,
                                      GWEN_TYPE_UINT32 productId) {
  LC_USBTTYDEVICE *d;

  d=LC_USBTTYDevice_List_First(dl);
  while(d) {
    if ((port==0 || port==d->port) &&
        (vendorId==0 || vendorId==d->vendorId) &&
        (productId==0 || productId==d->productId))
      return d;
    d=LC_USBTTYDevice_List_Next(d);
  } /* while */

  return 0;
}



int LC_USBTTYMonitor_Scan(LC_USBTTYMONITOR *um) {
  LC_USBTTYDEVICE_LIST *dl;
  LC_USBTTYDEVICE *d;

  LC_USBTTYDevice_List_Clear(um->newDevices);
  LC_USBTTYDevice_List_Clear(um->lostDevices);

  dl=LC_USBTTYDevice_List_new();
  if (LC_USBTTYMonitor_Read_ProcTtyDriverUsbSerial(dl)) {
    DBG_DEBUG(0, "here");
    LC_USBTTYDevice_List_free(dl);
    return -1;
  }

  /* find new devices */
  d=LC_USBTTYDevice_List_First(dl);
  while(d) {
    LC_USBTTYDEVICE *dd;

    dd=LC_USBTTYDevice_Find(um->currentDevices,
                            d->port,
                            d->vendorId,
                            d->productId);
    if (!dd) {
      LC_USBTTYDEVICE *newd;

      DBG_INFO(0, "Device %d is new (%04x/%04x)",
               d->port,
               d->vendorId,
               d->productId);
      newd=LC_USBTTYDevice_new(d->port,
                               d->vendorId,
                               d->productId);
      LC_USBTTYDevice_List_Add(newd, um->newDevices);
    }
    d=LC_USBTTYDevice_List_Next(d);
  }

  /* find lost devices */
  d=LC_USBTTYDevice_List_First(um->currentDevices);
  while(d) {
    LC_USBTTYDEVICE *dd;

    dd=LC_USBTTYDevice_Find(dl,
                            d->port,
                            d->vendorId,
                            d->productId);
    if (!dd) {
      LC_USBTTYDEVICE *lostd;

      DBG_INFO(0, "Device %d was lost (%04x/%04x)",
               d->port,
               d->vendorId,
               d->productId);
      lostd=LC_USBTTYDevice_new(d->port,
                                d->vendorId,
                                d->productId);
      LC_USBTTYDevice_List_Add(lostd, um->lostDevices);
    }
    d=LC_USBTTYDevice_List_Next(d);
  }

  LC_USBTTYDevice_List_free(um->currentDevices);
  um->currentDevices=dl;
  return 0;
}



LC_USBTTYDEVICE_LIST*
LC_USBTTYMonitor_GetNewDevices(const LC_USBTTYMONITOR *um){
  assert(um);
  return um->newDevices;
}



LC_USBTTYDEVICE_LIST*
LC_USBTTYMonitor_GetLostDevices(const LC_USBTTYMONITOR *um){
  assert(um);
  return um->lostDevices;
}















