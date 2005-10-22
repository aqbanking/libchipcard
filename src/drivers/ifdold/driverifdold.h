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


#ifndef CHIPCARD_DRIVER_IFDOLD_H
#define CHIPCARD_DRIVER_IFDOLD_H

typedef struct DRIVER_IFDOLD DRIVER_IFDOLD;


#include <gwenhywfar/libloader.h>
#include "driver_l.h"

LCD_DRIVER *DriverIFDOld_new(int argc, char **argv);
int DriverIFDOld_Start(LCD_DRIVER *d);

#endif /* CHIPCARD_DRIVER_IFDOLD_H */



