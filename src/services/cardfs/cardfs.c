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
#undef BUILDING_LIBCHIPCARD2_DLL

#include "cardfs_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inetsocket.h>

#include <unistd.h>


GWEN_INHERIT(LC_CLIENT, SERVICE_CARDFS)



LC_CLIENT *ServiceCardFS_new(int argc, char **argv){
  SERVICE_CARDFS *cardfs;
  LC_CLIENT *cl;

  cl=LC_Service_new(argc, argv);
  if (!cl) {
    DBG_ERROR(0, "Could not create service, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(SERVICE_CARDFS, cardfs);
  GWEN_INHERIT_SETDATA(LC_CLIENT, SERVICE_CARDFS,
                       cl, cardfs,
                       ServiceCardFS_freeData);
  LC_Service_SetCommandFn(cl, ServiceCardFS_Command);
  return cl;
}



void ServiceCardFS_freeData(void *bp, void *p) {
  SERVICE_CARDFS *cardfs;

  cardfs=(SERVICE_CARDFS*)p;
  GWEN_FREE_OBJECT(cardfs);
}



int ServiceCardFS_Start(LC_CLIENT *cl){
  SERVICE_CARDFS *cardfs;

  assert(cl);
  cardfs=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_CARDFS, cl);
  assert(cardfs);

  /* send status report to server */
  if (LC_Service_Connect(cl, "OK", "Service started")) {
    DBG_ERROR(0, "Error communicating with the server");
    return -1;
  }
  DBG_NOTICE(0, "Connected.");
  return 0;
}



const char *ServiceCardFS_GetErrorText(LC_CLIENT *cl, GWEN_TYPE_UINT32 err){
  const char *s;

  switch(err) {
  default:
    s="Unknown error";
    break;
  } /* switch */

  return s;
}




GWEN_TYPE_UINT32 ServiceCardFS_Command(LC_CLIENT *cl,
                                       LC_SERVICECLIENT *scl,
                                       GWEN_DB_NODE *dbRequest,
                                       GWEN_DB_NODE *dbResponse) {

}







int main(int argc, char **argv) {
  LC_CLIENT *sv;

  sv=ServiceCardFS_new(argc, argv);
  if (!sv) {
    DBG_ERROR(0, "Could not initialize service");
    return 1;
  }

  if (ServiceCardFS_Start(sv)) {
    DBG_ERROR(0, "Could not start service");
    LC_Client_free(sv);
    return 1;
  }

  if (LC_Service_Work(sv)) {
    DBG_ERROR(0, "An error occurred");
  }

  DBG_NOTICE(0, "Stopping service \"%s\"", argv[0]);
  GWEN_Socket_Select(0, 0, 0, 1000);

  LC_Client_free(sv);
  return 0;
}







