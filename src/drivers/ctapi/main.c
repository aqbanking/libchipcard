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

#include "driverctapi.h"
#include <gwenhywfar/debug.h>

#include <unistd.h>


int main(int argc, char **argv) {
  LC_DRIVER *d;

  d=DriverCTAPI_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not initialize driver");
    return 1;
  }

  if (DriverCTAPI_Start(d)) {
    DBG_ERROR(0, "Could not start driver");
    LC_Driver_free(d);
    return 1;
  }

  if (LC_Driver_IsTestMode(d)) {
    DBG_INFO(0, "Driver is in test mode");
    if (LC_Driver_Test(d)) {
      fprintf(stderr, "Reader is not available.\n");
      return 1;
    }
  }
  else {
    if (LC_Driver_Work(d)) {
      DBG_ERROR(0, "An error occurred");
    }

    DBG_NOTICE(0, "Stopping driver \"%s\"", argv[0]);
    sleep(1);
  }

  LC_Driver_free(d);
  return 0;
}

