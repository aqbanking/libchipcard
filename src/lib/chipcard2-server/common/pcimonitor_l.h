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


#ifndef CHIPCARD_SERVER_PCIMONITOR_L_H
#define CHIPCARD_SERVER_PCIMONITOR_L_H

#include <gwenhywfar/idlist.h>
#include <gwenhywfar/misc.h>

#include "devmonitor.h"

int LC_PciMonitor_ReadDevs(LC_DEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_PCIMONITOR_L_H */
