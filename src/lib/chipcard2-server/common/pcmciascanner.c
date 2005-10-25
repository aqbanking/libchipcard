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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#ifdef USE_PCMCIA
# include <pcmcia/version.h>
# include <pcmcia/cs_types.h>
struct pcmcia_socket;
# include <pcmcia/cs.h>
# include <pcmcia/cistpl.h>
# include <pcmcia/ds.h>
#endif

#include "pcmciascanner_p.h"


GWEN_INHERIT(LC_DEVSCANNER, LC_PCMCIA_SCANNER)



LC_DEVSCANNER *LC_PcmciaScanner_new() {
  LC_DEVSCANNER *sc;
  LC_PCMCIA_SCANNER *scp;

  sc=LC_DevScanner_new();
  GWEN_NEW_OBJECT(LC_PCMCIA_SCANNER, scp);
  GWEN_INHERIT_SETDATA(LC_DEVSCANNER, LC_PCMCIA_SCANNER, sc, scp,
                       LC_PcmciaScanner_FreeData);
  LC_DevScanner_SetReadDevsFn(sc, LC_PcmciaScanner_ReadDevs);
  scp->devMajor=LC_PcmciaScanner_GetDevMajor();
  if (scp->devMajor==-1) {
    DBG_DEBUG(0,
	      "Major device number for PCMCIA device not found. "
	      "Maybe kernel module not loaded?");
  }
  return sc;
}



void LC_PcmciaScanner_FreeData(void *bp, void *p) {
  LC_PCMCIA_SCANNER *scp;

  scp=(LC_PCMCIA_SCANNER*)p;

  GWEN_FREE_OBJECT(scp);
}



int LC_PcmciaScanner_GetDevMajor() {
  FILE *f;
  int devMajor;
  char lineBuf[32], name[32];

  f=fopen(LC_PCMCIA_PROC_FILE, "r");
  if (!f) {
    DBG_ERROR(0, "fopen(%s): %s", LC_PCMCIA_PROC_FILE,
              strerror(errno));
    return -1;
  }

  while(fgets(lineBuf, sizeof(lineBuf)-1, f) != NULL) {
    lineBuf[sizeof(lineBuf)-1]=0;
    if (2==sscanf(lineBuf, "%d %s", &devMajor, name)){
      if (strcmp(name, "pcmcia")==0){
        fclose(f);
        return devMajor;
      }
    }
  }
  fclose(f);
  return -1;
}


#ifdef USE_PCMCIA

int LC_PcmciaScanner_OpenSocket(LC_DEVSCANNER *sc, int sk){
  LC_PCMCIA_SCANNER *scp;
  static char *paths[]={"/var/lib/pcmcia", "/var/run", "/tmp", 0};
  int fd;
  char **p;
  char fpath[32];
  dev_t d;

  assert(sc);
  scp=GWEN_INHERIT_GETDATA(LC_DEVSCANNER, LC_PCMCIA_SCANNER, sc);
  assert(scp);

  d=makedev(scp->devMajor, sk);

  for (p = paths; *p; p++) {
    snprintf(fpath, sizeof(fpath), "%s/cc-%d", *p, (int)getpid());
    if (mknod(fpath, (S_IFCHR|S_IREAD|S_IWRITE), d)==0) {
      fd=open(fpath, O_RDONLY);
      unlink(fpath);
      if (fd>=0)
        return fd;
      if (errno==ENODEV) {
        DBG_INFO(0, "PCMCIA socket %d not available", sk);
        break;
      }
    }
  }
  return -1;
}



int LC_PcmciaScanner_GetTuple(int fd, unsigned char code,
                              ds_ioctl_arg_t *arg) {
  arg->tuple.DesiredTuple=code;
  arg->tuple.Attributes=TUPLE_RETURN_COMMON;
  arg->tuple.TupleOffset=0;
  if ((ioctl(fd, DS_GET_FIRST_TUPLE, arg)==0) &&
      (ioctl(fd, DS_GET_TUPLE_DATA, arg)==0) &&
      (ioctl(fd, DS_PARSE_TUPLE, arg)==0))
    return 0;
  else
    return -1;
}

#endif /* USE_PCMCIA */


int LC_PcmciaScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
#ifndef USE_PCMCIA
  return 1; /* no changes */
#else
  LC_PCMCIA_SCANNER *scp;
  int sknum;

  assert(sc);
  scp=GWEN_INHERIT_GETDATA(LC_DEVSCANNER, LC_PCMCIA_SCANNER, sc);
  assert(scp);

  if (scp->devMajor==-1) {
    return -1;
  }

  for (sknum=0; sknum<LC_PCMCIA_MAX_SOCKETS; sknum++) {
    int fd;

    fd=LC_PcmciaScanner_OpenSocket(sc, sknum);
    if (fd<0)
      break;
    else {
      ds_ioctl_arg_t arg;

      if (LC_PcmciaScanner_GetTuple(fd, CISTPL_MANFID, &arg)==0) {
        GWEN_TYPE_UINT32 vendorId;
        GWEN_TYPE_UINT32 productId;
        LC_DEVICE *dev;

        vendorId=arg.tuple_parse.parse.manfid.manf;
        productId=arg.tuple_parse.parse.manfid.card;
        dev=LC_Device_new(LC_Device_BusType_Pcmcia,
                          0, sknum, vendorId, productId);
        LC_Device_SetDevicePos(dev, sknum);
        LC_Device_List_Add(dev, dl);
      }
      else {
        DBG_DEBUG(0,
                  "Could not get info for PCMCIA "
                  "device at socket %d (%s)",
                  sknum,
                  strerror(errno));
      }
      close(fd);
    }
  } /* for */
#endif

  return 0;
}






