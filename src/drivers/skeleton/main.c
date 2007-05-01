/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: main.c 122 2005-10-22 00:42:09Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "driverSKEL1.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inetsocket.h>

#include <unistd.h>


int main(int argc, char **argv) {
  LCD_DRIVER *d;
  int rv;

  d=DriverSKEL3_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not initialize driver");
    return 1;
  }

  rv=LCD_Driver_Init(d, argc, argv);
  if (rv) {
    DBG_ERROR(0, "Could not init driver (%d)", rv);
    LCD_Driver_free(d);
    return 1;
  }

  if (DriverSKEL3_Start(d)) {
    DBG_ERROR(0, "Could not start driver");
    LCD_Driver_free(d);
    return 1;
  }

  if (LCD_Driver_IsTestMode(d)) {
    DBG_INFO(0, "Driver is in test mode");
    if (LCD_Driver_Test(d)) {
      fprintf(stderr, "Reader is not available.\n");
      return 1;
    }
  }
  else {
    if (LCD_Driver_Work(d)) {
      DBG_ERROR(0, "An error occurred");
    }

    DBG_NOTICE(0, "Stopping driver \"%s\"", argv[0]);
    GWEN_Socket_Select(0, 0, 0, 1000);
  }

  LCD_Driver_free(d);
  return 0;
}

