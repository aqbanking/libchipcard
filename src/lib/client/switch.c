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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "switch.h"
#include "base/client_l.h"
#include <chipcard/client/io/pcsc/clientpcsc.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/db.h>



LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion) {
  LC_CLIENT *cl;

  if (LC_Client_InitCommon()) {
    DBG_ERROR(0, "Unable to initialize, aborting");
    return NULL;
  }

  cl=LC_ClientPcsc_new(programName, programVersion);

  /* The client constructor used in LC_Client_Factory also calls
   * LC_Client_InitCommon, so it is safe here to call LC_Client_FiniCommon
   * without loosing runtime data.
   */
  LC_Client_FiniCommon();

  return cl;
}



void LC_Client_Version(int *major,
		       int *minor,
		       int *patchlevel,
		       int *build){
  *major=CHIPCARD_VERSION_MAJOR;
  *minor=CHIPCARD_VERSION_MINOR;
  *patchlevel=CHIPCARD_VERSION_PATCHLEVEL;
  *build=CHIPCARD_VERSION_BUILD;
}



