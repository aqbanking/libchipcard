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


#ifndef CHIPCARD_SERVER_PCIMONITOR_P_H
#define CHIPCARD_SERVER_PCIMONITOR_P_H


#define LC_PCI_PROC_DIR "/proc/bus/pci"
#define LC_PCI_PROC_BUS_PCI_DEVICES_FILE "/proc/bus/pci/devices"

#include "pciscanner_l.h"

static int LC_PciScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_PCIMONITOR_P_H */
