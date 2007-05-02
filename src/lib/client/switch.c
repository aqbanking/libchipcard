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
#include <chipcard/client/io/lcc/clientlcc.h>
#include <chipcard/client/io/pcsc/clientpcsc.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/db.h>



LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion) {
  GWEN_DB_NODE *db;
  const char *s;
  LC_CLIENT *cl;

  if (LC_Client_InitCommon()) {
    DBG_ERROR(0, "Unable to initialize, aborting");
    return 0;
  }

  db=LC_Client_GetCommonConfig();
  s=GWEN_DB_GetCharValue(db, "resmgr", 0, LC_CLIENT_LCC_NAME);
  assert(s);
  cl=LC_Client_Factory(s, programName, programVersion);

  /* The client constructor used in LC_Client_Factory also calls
   * LC_Client_InitCommon, so it is safe here to call LC_Client_FiniCommon
   * without loosing runtime data.
   */
  LC_Client_FiniCommon();

  return cl;
}



LC_CLIENT *LC_Client_Factory(const char *resmgr,
                             const char *programName,
                             const char *programVersion) {
  LC_CLIENT *cl;

  assert(resmgr);
  if (strcasecmp(resmgr, LC_CLIENT_PCSC_NAME)==0) {
#ifdef HAVE_PCSC
    cl=LC_ClientPcsc_new(programName, programVersion);
#else
    DBG_ERROR(LC_LOGDOMAIN, "No support for PC/SC");
    return 0;
#endif
  }
  else if (strcasecmp(resmgr, LC_CLIENT_LCC_NAME)==0)
    cl=LC_ClientLcc_new(programName, programVersion);
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Ressource manager backend \"%s\" not found",
              resmgr);
    return 0;
  }

  return cl;
}






