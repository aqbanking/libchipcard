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


#include "devmonitor_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define LCDM_INTFORMAT_DEC 1
#define LCDM_INTFORMAT_HEX 2
#define LCDM_INTFORMAT_OCT 3


GWEN_LIST_FUNCTIONS(LC_DEVICE, LC_Device)
GWEN_LIST_FUNCTIONS(LC_DEVSCANNER, LC_DevScanner)
GWEN_INHERIT_FUNCTIONS(LC_DEVSCANNER)



LC_DEVICE_BUSTYPE LC_Device_BusType_fromString(const char *s) {
  if (strcasecmp(s, "any")==0)
    return LC_Device_BusType_Any;
  else if (strcasecmp(s, "UsbRaw")==0)
    return LC_Device_BusType_UsbRaw;
  else if (strcasecmp(s, "UsbTty")==0)
    return LC_Device_BusType_UsbTty;
  else if (strcasecmp(s, "pci")==0)
    return LC_Device_BusType_Pci;
  else if (strcasecmp(s, "pcmcia")==0)
    return LC_Device_BusType_Pcmcia;
  else if (strcasecmp(s, "serial")==0)
    return LC_Device_BusType_Serial;
  return LC_Device_BusType_Unknown;
}



const char *LC_Device_BusType_toString(LC_DEVICE_BUSTYPE i) {
  switch(i) {
  case LC_Device_BusType_Any:    return "any";
  case LC_Device_BusType_UsbRaw: return "UsbRaw";
  case LC_Device_BusType_UsbTty: return "UsbTty";
  case LC_Device_BusType_Pci:    return "pci";
  case LC_Device_BusType_Pcmcia: return "pcmcia";
  case LC_Device_BusType_Serial: return "serial";
  default:                       return "unknown";
  }
}




LC_DEVICE *LC_Device_new(LC_DEVICE_BUSTYPE busType,
                         uint32_t busId,
                         uint32_t deviceId,
                         uint32_t vendorId,
                         uint32_t productId) {
  LC_DEVICE *ud;

  GWEN_NEW_OBJECT(LC_DEVICE, ud);
  DBG_MEM_INC("LC_DEVICE", 0);
  GWEN_LIST_INIT(LC_DEVICE, ud);

  ud->busType=busType;
  ud->busId=busId;
  ud->deviceId=deviceId;
  ud->vendorId=vendorId;
  ud->productId=productId;

  return ud;
}



void LC_Device_free(LC_DEVICE *ud) {
  if (ud) {
    GWEN_LIST_FINI(LC_DEVICE, ud);
    free(ud->driverType);
    free(ud->readerType);
    free(ud->path);
    free(ud->deviceName);
    free(ud->busName);
    free(ud->halPath);
    GWEN_FREE_OBJECT(ud);
    DBG_MEM_DEC("LC_DEVICE");
  }
}



LC_DEVICE *LC_Device_dup(const LC_DEVICE *od) {
  LC_DEVICE *ud;

  GWEN_NEW_OBJECT(LC_DEVICE, ud);
  DBG_MEM_INC("LC_DEVICE", 0);
  GWEN_LIST_INIT(LC_DEVICE, ud);

  ud->busType=od->busType;
  ud->busId=od->busId;
  ud->deviceId=od->deviceId;
  ud->vendorId=od->vendorId;
  ud->productId=od->productId;
  if (od->path)
    ud->path=strdup(od->path);
  if (od->busName)
    ud->busName=strdup(od->busName);
  if (od->deviceName)
    ud->deviceName=strdup(od->deviceName);
  if (od->readerType)
    ud->readerType=strdup(od->readerType);
  if (od->driverType)
    ud->driverType=strdup(od->driverType);
  if (od->halPath)
    ud->halPath=strdup(od->halPath);

  return ud;
}



LC_DEVICE_BUSTYPE LC_Device_GetBusType(const LC_DEVICE *ud) {
  assert(ud);
  return ud->busType;
}



uint32_t LC_Device_GetDevicePos(const LC_DEVICE *ud){
  assert(ud);
  return ud->devicePos;
}



void LC_Device_SetDevicePos(LC_DEVICE *ud, uint32_t i) {
  assert(ud);
  ud->devicePos=i;
}




uint32_t LC_Device_GetBusId(const LC_DEVICE *ud){
  assert(ud);
  return ud->busId;
}



uint32_t LC_Device_GetDeviceId(const LC_DEVICE *ud){
  assert(ud);
  return ud->deviceId;
}



uint32_t LC_Device_GetVendorId(const LC_DEVICE *ud){
  assert(ud);
  return ud->vendorId;
}



uint32_t LC_Device_GetProductId(const LC_DEVICE *ud){
  assert(ud);
  return ud->productId;
}



const char *LC_Device_GetBusName(const LC_DEVICE *ud) {
  assert(ud);
  return ud->busName;
}



void LC_Device_SetBusName(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->busName);
  if (s) ud->busName=strdup(s);
  else ud->busName=0;
}



const char *LC_Device_GetDeviceName(const LC_DEVICE *ud) {
  assert(ud);
  return ud->deviceName;
}



void LC_Device_SetDeviceName(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->deviceName);
  if (s) ud->deviceName=strdup(s);
  else ud->deviceName=0;
}



const char *LC_Device_GetPath(const LC_DEVICE *ud) {
  assert(ud);
  return ud->path;
}



void LC_Device_SetPath(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->path);
  if (s) ud->path=strdup(s);
  else ud->path=0;
}



const char *LC_Device_GetDriverType(const LC_DEVICE *ud) {
  assert(ud);
  return ud->driverType;
}



void LC_Device_SetDriverType(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->driverType);
  if (s) ud->driverType=strdup(s);
  else ud->driverType=0;
}



const char *LC_Device_GetReaderType(const LC_DEVICE *ud) {
  assert(ud);
  return ud->readerType;
}



void LC_Device_SetReaderType(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->readerType);
  if (s) ud->readerType=strdup(s);
  else ud->readerType=0;
}



const char *LC_Device_GetHalPath(const LC_DEVICE *ud) {
  assert(ud);
  return ud->halPath;
}



void LC_Device_SetHalPath(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->halPath);
  if (s) ud->halPath=strdup(s);
  else ud->halPath=NULL;
}



LC_DEVICE *LC_Device_List_Find(LC_DEVICE_LIST *dl,
                               LC_DEVICE_BUSTYPE busType,
                               uint32_t busId,
                               uint32_t deviceId,
                               uint32_t vendorId,
                               uint32_t productId) {
  LC_DEVICE *d;

  d=LC_Device_List_First(dl);
  while(d) {
    if ((busType==LC_Device_BusType_Any || busType==d->busType) &&
	(busId==0 || busId==d->busId) &&
        (deviceId==0 || deviceId==d->deviceId) &&
        (vendorId==0 || vendorId==d->vendorId) &&
        (productId==0 || productId==d->productId))
      return d;
    d=LC_Device_List_Next(d);
  } /* while */

  return 0;
}



LC_DEVICE *LC_Device_Get(LC_DEVICE_LIST *dl,
			 LC_DEVICE_BUSTYPE busType,
			 uint32_t dpos) {
  LC_DEVICE *d;

  d=LC_Device_List_First(dl);
  while(d) {
    if ((d->busType==busType) &&
	(dpos==d->devicePos))
      return d;
    d=LC_Device_List_Next(d);
  } /* while */

  return 0;
}



int LC_Device__WriteIntVar(int v,
                           int format_type,
			   int format_len, int format_null,
			   GWEN_BUFFER *buf) {
  char tmplbuf[32];
  char numbuf[32];
  int rv;
  int i;

  i=0;
  tmplbuf[i++]='%';
  if (format_len) {
    if (format_null)
      tmplbuf[i++]='0';
    tmplbuf[i++]='*';
  }

  switch(format_type) {
  case LCDM_INTFORMAT_DEC: tmplbuf[i++]='d'; break;
  case LCDM_INTFORMAT_HEX: tmplbuf[i++]='x'; break;
  case LCDM_INTFORMAT_OCT: tmplbuf[i++]='o'; break;
  default:
    DBG_ERROR(0, "Invalid format type %d", format_type);
    return -1;
  }
  tmplbuf[i++]=0;

  if (format_len) {
    if (format_null)
      rv=snprintf(numbuf, sizeof(numbuf), tmplbuf, format_len, v);
    else
      rv=snprintf(numbuf, sizeof(numbuf), tmplbuf, format_len, v);
  }
  else
    rv=snprintf(numbuf, sizeof(numbuf), tmplbuf, v);

  if (rv<1 || rv>=sizeof(numbuf))
    return -1;
  GWEN_Buffer_AppendString(buf, numbuf);
  return 0;
}



int LC_Device_ReplaceVars(const LC_DEVICE *d, const char *tmpl,
                          GWEN_BUFFER *buf) {
  const char *s;

  assert(tmpl);
  s=tmpl;
  while(*s) {
    if (*s=='$') {
      if (s[1]=='(') {
        GWEN_BUFFER *vbuf;
        const char *vname;
	char numbuf[32];
        int format_len=0;
	int format_null=0;
        int format_type=LCDM_INTFORMAT_DEC;

        /* get variable name */
	vbuf=GWEN_Buffer_new(0, 256, 0, 1);
        s+=2;
	while(*s) {
	  if (*s==')' || *s==':')
	    break;
	  GWEN_Buffer_AppendByte(vbuf, *(s++));
	}
	if (*s==':') {
	  /* format string follows */
	  s++;
	  if (*s=='0') {
	    format_null=1;
            s++;
	  }
	  while(*s && isdigit(*s)) {
	    format_len*=10;
	    format_len+=(*s)-'0';
            s++;
	  }

	  if (isalpha(*s)) {
	    switch(*s) {
	    case 'd': format_type=LCDM_INTFORMAT_DEC; break;
	    case 'x': format_type=LCDM_INTFORMAT_HEX; break;
	    case 'o': format_type=LCDM_INTFORMAT_OCT; break;
	    default :
	      DBG_INFO(0, "Invalid format type specifier \"%c\"", *s);
	      return -1;
	    }
	    s++;
	  }
	}

        if (*s!=')') {
          DBG_ERROR(0, "Bad replace string (\"%s\" doesn't end in bracket)",
                    s);
          GWEN_Buffer_free(vbuf);
          return -1;
        }
        vname=GWEN_Buffer_GetStart(vbuf);
        if (strcasecmp(vname, "devicePos")==0) {
          snprintf(numbuf, sizeof(numbuf)-1, "%d", d->devicePos);
          GWEN_Buffer_AppendString(buf, numbuf);
        }
        else if (strcasecmp(vname, "busId")==0) {
	  LC_Device__WriteIntVar(d->busId,
				 format_type, format_len, format_null, buf);
        }
        else if (strcasecmp(vname, "deviceId")==0) {
	  LC_Device__WriteIntVar(d->deviceId,
				 format_type, format_len, format_null, buf);
	}
        else if (strcasecmp(vname, "vendorId")==0) {
	  LC_Device__WriteIntVar(d->vendorId,
				 format_type, format_len, format_null, buf);
	}
        else if (strcasecmp(vname, "productId")==0) {
	  LC_Device__WriteIntVar(d->productId,
				 format_type, format_len, format_null, buf);
	}
	else if (strcasecmp(vname, "path")==0) {
	  if (d->path)
	    GWEN_Buffer_AppendString(buf, d->path);
	}
	else if (strcasecmp(vname, "busName")==0) {
	  if (d->busName)
	    GWEN_Buffer_AppendString(buf, d->busName);
	}
	else if (strcasecmp(vname, "deviceName")==0) {
	  if (d->deviceName)
	    GWEN_Buffer_AppendString(buf, d->deviceName);
	}
        else if (strcasecmp(vname, "readerType")==0) {
          if (d->readerType)
            GWEN_Buffer_AppendString(buf, d->readerType);
        }
        else if (strcasecmp(vname, "driverType")==0) {
          if (d->driverType)
            GWEN_Buffer_AppendString(buf, d->driverType);
        }
	else if (strcasecmp(vname, "halpath")==0) {
	  if (d->halPath)
	    GWEN_Buffer_AppendString(buf, d->halPath);
	}
        else {
          DBG_ERROR(0, "Bad replace string (unknown var \"%s\" in \"%s\")",
                    vname, tmpl);
          GWEN_Buffer_free(vbuf);
          return -1;
        }
        GWEN_Buffer_free(vbuf);
        s++;
      }
      else
        GWEN_Buffer_AppendByte(buf, *(s++));
    }
    else
      GWEN_Buffer_AppendByte(buf, *(s++));
  } /* while */

  return 0;
}






LC_DEVSCANNER *LC_DevScanner_new() {
  LC_DEVSCANNER *um;

  GWEN_NEW_OBJECT(LC_DEVSCANNER, um);
  DBG_MEM_INC("LC_DEVSCANNER", 0);
  GWEN_INHERIT_INIT(LC_DEVSCANNER, um);
  GWEN_LIST_INIT(LC_DEVSCANNER, um);

  return um;
}



void LC_DevScanner_free(LC_DEVSCANNER *um) {
  if (um) {
    GWEN_LIST_FINI(LC_DEVSCANNER, um);
    GWEN_INHERIT_FINI(LC_DEVSCANNER, um);
    GWEN_FREE_OBJECT(um);
    DBG_MEM_DEC("LC_DEVSCANNER");
  }
}




int LC_DevScanner_ReadDevs(LC_DEVSCANNER *um, LC_DEVICE_LIST *dl) {
  assert(um);
  assert(um->readDevsFn);
  return um->readDevsFn(um, dl);
}



void LC_DevScanner_SetReadDevsFn(LC_DEVSCANNER *um,
                                 LC_DEVSCANNER_READ_DEVS_FN fn) {
  assert(um);
  um->readDevsFn=fn;
}














LC_DEVMONITOR *LC_DevMonitor_new() {
  LC_DEVMONITOR *um;

  GWEN_NEW_OBJECT(LC_DEVMONITOR, um);
  DBG_MEM_INC("LC_DEVMONITOR", 0);
  um->currentDevices=LC_Device_List_new();
  um->newDevices=LC_Device_List_new();
  um->lostDevices=LC_Device_List_new();
  um->scanners=LC_DevScanner_List_new();
  return um;
}



void LC_DevMonitor_free(LC_DEVMONITOR *um) {
  if (um) {
    LC_DevScanner_List_free(um->scanners);
    LC_Device_List_free(um->currentDevices);
    LC_Device_List_free(um->newDevices);
    LC_Device_List_free(um->lostDevices);

    GWEN_FREE_OBJECT(um);
    DBG_MEM_DEC("LC_DEVMONITOR");
  }
}



int LC_DevMonitor_Scan(LC_DEVMONITOR *um) {
  LC_DEVICE_LIST *dl;
  LC_DEVICE *d;
  LC_DEVSCANNER *scanner;
  int oks=0;
  int changes=0;

  LC_Device_List_Clear(um->newDevices);
  LC_Device_List_Clear(um->lostDevices);

  dl=LC_Device_List_new();

  scanner=LC_DevScanner_List_First(um->scanners);
  while(scanner) {
    int rv;

    rv=LC_DevScanner_ReadDevs(scanner, dl);
    if (rv==-1) {
      DBG_VERBOUS(0, "here");
    }
    else if (rv==0)
      oks++;
    scanner=LC_DevScanner_List_Next(scanner);
  }

  if (oks==0) {
    DBG_INFO(0, "No scanner succeeded");
    LC_Device_List_free(dl);
    return -1;
  }

  /* find new devices */
  d=LC_Device_List_First(dl);
  while(d) {
    LC_DEVICE *dd;

    dd=LC_Device_List_Find(um->currentDevices,
			   d->busType,
			   d->busId,
			   d->deviceId,
			   d->vendorId,
			   d->productId);
    if (!dd) {
      LC_DEVICE *newd;

      DBG_DEBUG(0, "Device %s/%d/%d is new (%04x/%04x)",
                LC_Device_BusType_toString(LC_Device_GetBusType(d)),
                d->busId,
                d->deviceId,
                d->vendorId,
		d->productId);
      newd=LC_Device_dup(d);
      newd->devicePos=d->devicePos;
      LC_Device_List_Add(newd, um->newDevices);
      changes++;
    }
    d=LC_Device_List_Next(d);
  }

  /* find lost devices */
  d=LC_Device_List_First(um->currentDevices);
  while(d) {
    LC_DEVICE *dd;

    dd=LC_Device_List_Find(dl,
			   d->busType,
			   d->busId,
			   d->deviceId,
			   d->vendorId,
			   d->productId);
    if (!dd) {
      LC_DEVICE *lostd;

      DBG_DEBUG(0, "Device %s/%d/%d was lost (%04x/%04x)",
                LC_Device_BusType_toString(LC_Device_GetBusType(d)),
                d->busId,
                d->deviceId,
                d->vendorId,
                d->productId);
      lostd=LC_Device_dup(d);
      lostd->devicePos=d->devicePos;
      LC_Device_List_Add(lostd, um->lostDevices);
      changes++;
    }
    d=LC_Device_List_Next(d);
  }

  LC_Device_List_free(um->currentDevices);
  um->currentDevices=dl;
  if (changes)
    return 0;
  return 1;
}



LC_DEVICE_LIST *LC_DevMonitor_GetNewDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->newDevices;
}



LC_DEVICE_LIST *LC_DevMonitor_GetLostDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->lostDevices;
}



LC_DEVICE_LIST *LC_DevMonitor_GetCurrentDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->currentDevices;
}



void LC_DevMonitor_AddScanner(LC_DEVMONITOR *um, LC_DEVSCANNER *sc) {
  assert(um);
  assert(sc);
  LC_DevScanner_List_Add(sc, um->scanners);
}

















