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


#include "pciscanner_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



LC_DEVSCANNER *LC_PciScanner_new() {
  LC_DEVSCANNER *sc;

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_PciScanner_ReadDevs);

  return sc;
}



int LC_PciScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
  GWEN_DIRECTORYDATA *dDir;
  GWEN_BUFFER *nbuf;
  GWEN_TYPE_UINT32 pos;
  int count=0;

  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(nbuf, LC_PCI_PROC_DIR);
  GWEN_Buffer_AppendString(nbuf, "/");
  pos=GWEN_Buffer_GetPos(nbuf);
  dDir=GWEN_Directory_new();
  if (!GWEN_Directory_Open(dDir, LC_PCI_PROC_DIR)) {
    char nameBuf[256];

    /* search for name in this folder */
    while(!GWEN_Directory_Read(dDir, nameBuf, sizeof(nameBuf))) {
      if (strcmp(nameBuf, ".")!=0 &&
          strcmp(nameBuf, "..")!=0) {
        int busId;

	if (1==sscanf(nameBuf, "%d", &busId)) {
          struct stat st;
  
	  /* found name, add it to the buffer */
	  DBG_VERBOUS(0, "Scanning bus "GWEN_TYPE_TMPL_UINT32, busId);
          GWEN_Buffer_Crop(nbuf, 0, pos);
          GWEN_Buffer_SetPos(nbuf, pos);
          GWEN_Buffer_AppendString(nbuf, nameBuf);
          if (stat(GWEN_Buffer_GetStart(nbuf), &st)) {
            /* error */
            DBG_WARN(0, "stat(%s): %s",
                     GWEN_Buffer_GetStart(nbuf),
                     strerror(errno));
          }
          else {
	    /* check for folder */
	    if (S_ISDIR(st.st_mode)) {
	      GWEN_DIRECTORYDATA *dBus;
	      GWEN_TYPE_UINT32 dpos;

	      /* check devices */
	      GWEN_Buffer_AppendString(nbuf, "/");
	      dpos=GWEN_Buffer_GetPos(nbuf);
	      dBus=GWEN_Directory_new();
	      if (!GWEN_Directory_Open(dBus, GWEN_Buffer_GetStart(nbuf))) {
		while(!GWEN_Directory_Read(dBus, nameBuf, sizeof(nameBuf))) {
		  if (strcmp(nameBuf, ".")!=0 &&
		      strcmp(nameBuf, "..")!=0) {
		    int devId, funcId;
	    
		    if (2==sscanf(nameBuf, "%d.%d", &devId, &funcId)) {
		      struct stat st;

		      DBG_VERBOUS(0, "Checking device %d function %d",
				  devId, funcId);

		      /* found name, add it to the buffer */
		      GWEN_Buffer_Crop(nbuf, 0, dpos);
		      GWEN_Buffer_SetPos(nbuf, dpos);
		      GWEN_Buffer_AppendString(nbuf, nameBuf);
		      if (stat(GWEN_Buffer_GetStart(nbuf), &st)) {
			/* error */
			DBG_WARN(0, "stat(%s): %s",
				 GWEN_Buffer_GetStart(nbuf),
				 strerror(errno));
		      }
		      else {
			/* check for file */
			if (S_ISREG(st.st_mode)) {
			  int fd;

			  /* have a device file, open it */
			  fd=open(GWEN_Buffer_GetStart(nbuf), O_RDONLY);
			  if (fd!=-1) {
			    unsigned char devinfo[4];
			    int rv;

			    rv=read(fd, &devinfo, sizeof(devinfo));
			    if (rv==sizeof(devinfo)) {
			      GWEN_TYPE_UINT32 vendorId;
                              GWEN_TYPE_UINT32 productId;
                              LC_DEVICE *dev;

                              vendorId=(devinfo[1]<<8)+devinfo[0];
			      productId=(devinfo[3]<<8)+devinfo[2];
                              dev=LC_Device_new(LC_Device_BusType_Pci,
                                                busId,
                                                (devId<<24)+funcId,
                                                vendorId,
                                                productId);
			      DBG_VERBOUS(0, "Adding device \"%s\"",
                                          GWEN_Buffer_GetStart(nbuf));
                              LC_Device_SetDevicePos(dev, count++);
                              LC_Device_List_Add(dev, dl);
                            }
                            else {
			      DBG_ERROR(0, "Error reading device \"%s\"",
                                        GWEN_Buffer_GetStart(nbuf));
			    }
                            close(fd);
			  }
			}
		      }
		    } /* if name pattern matches */
		  } /* if not a special entry */
		} /* while still entries */
		GWEN_Directory_Close(dBus);
	      }
	      else {
		DBG_ERROR(0, "Could not open folder \"%s\"",
			  GWEN_Buffer_GetStart(nbuf));
	      }
	      GWEN_Directory_free(dBus);
	    }
	  }
	} /* if name pattern matches */
      } /* if not a special entry */
    } /* while still entries */
    GWEN_Directory_Close(dDir);
  }
  else {
    DBG_ERROR(0, "Could not open folder \"%s\"",
	      LC_PCI_PROC_DIR);
  }
  GWEN_Directory_free(dDir);
  GWEN_Buffer_free(nbuf);
  return 0;
}



