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


#ifndef CHIPCARD_DRIVER_CTAPI_H
#define CHIPCARD_DRIVER_CTAPI_H

typedef struct DRIVER_CTAPI DRIVER_CTAPI;


#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/driver.h>

LCD_DRIVER *DriverCTAPI_new(int argc, char **argv);
void DriverCTAPI_free(DRIVER_CTAPI *dct);
int DriverCTAPI_Start(LCD_DRIVER *d);





#endif /* CHIPCARD_DRIVER_CTAPI_H */



