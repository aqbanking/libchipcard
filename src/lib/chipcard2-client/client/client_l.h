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


#ifndef CHIPCARD_CLIENT_CLIENT_L_H
#define CHIPCARD_CLIENT_CLIENT_L_H


#include <chipcard2-client/client/client.h>


int LC_Client_SelectApp(LC_CLIENT *cl,
                        LC_CARD *cd,
                        const char *appName);


#endif /* CHIPCARD_CLIENT_CLIENT_L_H */
