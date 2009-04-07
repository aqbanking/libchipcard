/***************************************************************************
    begin       : Thu Sep 02 2008
    copyright   : (C) 2008 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_SERVER_HALSCANNER_P_H
#define CHIPCARD_SERVER_HALSCANNER_P_H

#include "halscanner_l.h"

#include <hal/libhal.h>
#include <dbus/dbus.h>


typedef struct LC_HALSCANNER LC_HALSCANNER;
struct LC_HALSCANNER {
  DBusError dbus_error;
  DBusConnection *dbus_conn;
  LibHalContext *ctx;
};

static void LC_HalScanner_FreeData(void *bp, void *p);


static
int LC_HalScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl);


#endif
