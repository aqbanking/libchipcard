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


#ifndef CHIPCARD_CLIENT_SWITCH_H
#define CHIPCARD_CLIENT_SWITCH_H

#include <gwenhywfar/inherit.h>
#include <chipcard/chipcard.h>
#include <chipcard/client/client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function creates a libchipcard client. It loads the client
 * configuration file an reads the variable @b resmgr to determine which
 * ressource manager to use (defaults to @b lcc on Linux).
 * @param programName name of the program which wants to create the client
 * @param programVersion version string of that program
 */
CHIPCARD_API
LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion);

/**
 * This function creates a libchipcard client which uses the given ressource
 * manager.
 * @param resmgr Currently supported names are "lcc" and "pcsc"
 * @param programName name of the program which wants to create the client
 * @param programVersion version string of that program
 */
CHIPCARD_API
LC_CLIENT *LC_Client_Factory(const char *resmgr,
                             const char *programName,
                             const char *programVersion);


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CLIENT_SWITCH_H */



