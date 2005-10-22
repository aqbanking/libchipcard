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


#ifndef CHIPCARD_SERVER_PCMCIAMONITOR_P_H
#define CHIPCARD_SERVER_PCMCIAMONITOR_P_H

#define LC_PCMCIA_PROC_FILE "/proc/devices"
#define LC_PCMCIA_MAX_SOCKETS 4

#define LC_PCMCIA_TUP_VENDORID 0x20


#include "pcmciascanner_l.h"

static int LC_PcmciaScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl);


typedef struct LC_PCMCIA_SCANNER LC_PCMCIA_SCANNER;
struct LC_PCMCIA_SCANNER {
  int devMajor;
};


void LC_PcmciaScanner_FreeData(void *bp, void *p);
int LC_PcmciaScanner_GetDevMajor();

int LC_PcmciaScanner_OpenSocket(LC_DEVSCANNER *sc, int sk);


#endif /* CHIPCARD_SERVER_PCMCIAMONITOR_P_H */

