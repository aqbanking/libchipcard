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


#ifndef CHIPCARD_DRIVER_IFD_H
#define CHIPCARD_DRIVER_IFD_H

typedef struct DRIVER_IFD DRIVER_IFD;


#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/driver.h>

LCD_DRIVER *DriverIFD_new(int argc, char **argv);
int DriverIFD_Start(LCD_DRIVER *d);




#endif /* CHIPCARD_DRIVER_IFD_H */



