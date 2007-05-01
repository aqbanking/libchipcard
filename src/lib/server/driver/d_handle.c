/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driver.c 284 2006-09-22 00:53:00Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/* included by driver.c */






int LCD_Driver_HandleRequest(LCD_DRIVER *d,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq){
  int rv;

  DBG_NOTICE(0, "Incoming request \"%s\"", name);

  /* if there is a virtual function set go ask the function first */
  if (d->handleRequestFn) {
    rv=d->handleRequestFn(d, rid, name, dbReq);
    if (rv!=1)
      return rv;
  }

  if (strcasecmp(name, "Driver_StartReader")==0) {
    rv=LCD_Driver_HandleStartReader(d, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_StopReader")==0) {
    rv=LCD_Driver_HandleStopReader(d, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardCommand")==0) {
    rv=LCD_Driver_HandleCardCommand(d, rid, dbReq);
   }
  else if (strcasecmp(name, "Driver_ResetCard")==0) {
    rv=LCD_Driver_HandleResetCard(d, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_StopDriver")==0) {
    rv=LCD_Driver_HandleStopDriver(d, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_SuspendCheck")==0) {
    rv=LCD_Driver_HandleSuspendCheck(d, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_ResumeCheck")==0) {
    rv=LCD_Driver_HandleResumeCheck(d, rid, dbReq);
  }

  else
    rv=1; /* not handled */

  return rv;
}


#include "d_cardcommand.c"
#include "d_resetcard.c"
#include "d_resumecheck.c"
#include "d_startreader.c"
#include "d_stopdriver.c"
#include "d_stopreader.c"
#include "d_suspendcheck.c"






















