/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENTPCSC_H
#define CHIPCARD_CLIENT_CLIENTPCSC_H


#include <chipcard3/client/client_imp.h>


#define LC_CLIENT_PCSC_NAME "pcsc"


CHIPCARD_API
LC_CLIENT *LC_ClientPcsc_new(const char *programName,
                             const char *programVersion);


#endif /* CHIPCARD_CLIENT_CLIENTPCSC_H */

