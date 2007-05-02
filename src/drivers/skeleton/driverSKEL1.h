/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driverSKEL1.h 122 2005-10-22 00:42:09Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_SKEL1_H
#define CHIPCARD_DRIVER_SKEL1_H

typedef struct DRIVER_SKEL2 DRIVER_SKEL2;


#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/driver.h>

LCD_DRIVER *DriverSKEL3_new(int argc, char **argv);
int DriverSKEL3_Start(LCD_DRIVER *d);





#endif /* CHIPCARD_DRIVER_SKEL1_H */



