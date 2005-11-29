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


#ifndef CHIPCARD_DRIVER_CCID_H
#define CHIPCARD_DRIVER_CCID_H

typedef struct DRIVER_CCID DRIVER_CCID;


#include <gwenhywfar/libloader.h>
#include "driver_l.h"

LCD_DRIVER *DriverCCID_new(int argc, char **argv);
int DriverCCID_Start(LCD_DRIVER *d);




#endif /* CHIPCARD_DRIVER_CCID_H */



