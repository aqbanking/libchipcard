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


#ifndef CHIPCARD_SERVICE_CARDFS_P_H
#define CHIPCARD_SERVICE_CARDFS_P_H


#include <chipcard2/chipcard2.h>
#include <chipcard2-service/service.h>


#define SERVICE_CARDFS_ERROR_BAD_RESPONSE          1


typedef struct SERVICE_CARDFS SERVICE_CARDFS;
struct SERVICE_CARDFS {

};


LC_CLIENT *ServiceCardFS_new(int argc, char **argv);
int ServiceCardFS_Start(LC_CLIENT *sv);


void ServiceCardFS_freeData(void *bp, void *p);


const char *ServiceCardFS_GetErrorText(LC_CLIENT *cl, GWEN_TYPE_UINT32 err);


GWEN_TYPE_UINT32 ServiceCardFS_Command(LC_CLIENT *cl,
                                       LC_SERVICECLIENT *scl,
                                       GWEN_DB_NODE *dbRequest,
                                       GWEN_DB_NODE *dbResponse);


#endif /* CHIPCARD_SERVICE_CARDFS_H */






