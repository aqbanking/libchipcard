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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cardfs_p.h"
#include <gwenhywfar/debug.h>

#include <unistd.h>


GWEN_INHERIT(LC_SERVICE, SERVICE_CARDFS)



LC_SERVICE *ServiceCardFS_new(int argc, char **argv){
  SERVICE_CARDFS *cardfs;
  LC_SERVICE *d;

  d=LC_Service_new(argc, argv);
  if (!d) {
    DBG_ERROR(0, "Could not create service, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(SERVICE_CARDFS, cardfs);
  GWEN_INHERIT_SETDATA(LC_SERVICE, SERVICE_CARDFS,
                       d, cardfs,
                       ServiceCardFS_freeData);
  LC_Service_SetCommandFn(d, ServiceCardFS_Command);
  return d;
}



void ServiceCardFS_freeData(void *bp, void *p) {
  SERVICE_CARDFS *cardfs;

  cardfs=(SERVICE_CARDFS*)p;
  GWEN_FREE_OBJECT(cardfs);
}



int ServiceCardFS_Start(LC_SERVICE *d){
  SERVICE_CARDFS *cardfs;

  assert(d);
  cardfs=GWEN_INHERIT_GETDATA(LC_SERVICE, SERVICE_CARDFS, d);
  assert(cardfs);

  /* send status report to server */
  if (LC_Service_Connect(d, "OK", "Service started")) {
    DBG_ERROR(0, "Error communicating with the server");
    return -1;
  }
  DBG_NOTICE(0, "Connected.");
  return 0;
}



const char *ServiceCardFS_GetErrorText(LC_SERVICE *d, GWEN_TYPE_UINT32 err){
  const char *s;

  switch(err) {
  default:
    s="Unknown error";
    break;
  } /* switch */

  return s;
}




GWEN_TYPE_UINT32 ServiceCardFS_Command(LC_SERVICE *d,
                                       LC_SERVICECLIENT *cl,
                                       GWEN_DB_NODE *dbRequest,
                                       GWEN_DB_NODE *dbResponse) {

}







int main(int argc, char **argv) {
  LC_SERVICE *sv;

  sv=ServiceCardFS_new(argc, argv);
  if (!sv) {
    DBG_ERROR(0, "Could not initialize service");
    return 1;
  }

  if (ServiceCardFS_Start(sv)) {
    DBG_ERROR(0, "Could not start service");
    LC_Service_free(sv);
    return 1;
  }

  if (LC_Service_Work(sv)) {
    DBG_ERROR(0, "An error occurred");
  }

  DBG_NOTICE(0, "Stopping service \"%s\"", argv[0]);
  sleep(1);

  LC_Service_free(sv);
  return 0;
}







