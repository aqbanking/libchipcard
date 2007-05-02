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


#ifndef CHIPCARD_SERVICE_SERVICE_H
#define CHIPCARD_SERVICE_SERVICE_H


typedef struct LC_SERVICE LC_SERVICE;

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>

#include <chipcard/client/service/serviceclient.h>
#include <chipcard/client/client.h>


typedef enum {
  LC_ServiceCheckArgsResultOk=0,
  LC_ServiceCheckArgsResultError,
  LC_ServiceCheckArgsResultVersion,
  LC_ServiceCheckArgsResultHelp
} LC_SERVICE_CHECKARGS_RESULT;


typedef GWEN_TYPE_UINT32 (*LC_SERVICE_OPEN_FN)(LC_CLIENT *cl,
                                               LC_SERVICECLIENT *scl,
                                               GWEN_DB_NODE *dbData);

typedef GWEN_TYPE_UINT32 (*LC_SERVICE_CLOSE_FN)(LC_CLIENT *cl,
                                                LC_SERVICECLIENT *scl,
                                                GWEN_DB_NODE *dbData);

typedef GWEN_TYPE_UINT32 (*LC_SERVICE_COMMAND_FN)(LC_CLIENT *cl,
                                                  LC_SERVICECLIENT *scl,
                                                  GWEN_DB_NODE *dbRequest,
                                                  GWEN_DB_NODE *dbResponse);

typedef int (*LC_SERVICE_WORK_FN)(LC_CLIENT *cl);

typedef const char* (*LC_SERVICE_GETERRORTEXT_FN)(LC_CLIENT *cl,
                                                  GWEN_TYPE_UINT32 err);


CHIPCARD_API
void LC_Service_Usage(const char *prgName);


CHIPCARD_API
LC_CLIENT *LC_Service_new(int argc, char **argv);

CHIPCARD_API
void LC_Service_free(LC_SERVICE *d);


CHIPCARD_API
const char *LC_Service_GetServiceDataDir(const LC_CLIENT *d);

CHIPCARD_API
const char *LC_Service_GetLibraryFile(const LC_CLIENT *d);

CHIPCARD_API
const char *LC_Service_GetServiceId(const LC_CLIENT *d);


CHIPCARD_API
LC_SERVICECLIENT_LIST *LC_Service_GetClients(const LC_CLIENT *d);



CHIPCARD_API
int LC_Service_Connect(LC_CLIENT *cl, const char *code, const char *text);


CHIPCARD_API
int LC_Service_Work(LC_CLIENT *d);


CHIPCARD_API
GWEN_TYPE_UINT32 LC_Service_Open(LC_CLIENT *d,
                                 LC_SERVICECLIENT *scl,
                                 GWEN_DB_NODE *dbData);


CHIPCARD_API
GWEN_TYPE_UINT32 LC_Service_Close(LC_CLIENT *d,
                                  LC_SERVICECLIENT *scl,
                                  GWEN_DB_NODE *dbData);


CHIPCARD_API
GWEN_TYPE_UINT32 LC_Service_Command(LC_CLIENT *d,
                                    LC_SERVICECLIENT *scl,
                                    GWEN_DB_NODE *dbRequest,
                                    GWEN_DB_NODE *dbResponse);


CHIPCARD_API
const char *LC_Service_GetErrorText(LC_CLIENT *d,
                                    GWEN_TYPE_UINT32 err);



CHIPCARD_API
void LC_Service_SetOpenFn(LC_CLIENT *d, LC_SERVICE_OPEN_FN fn);

CHIPCARD_API
void LC_Service_SetCloseFn(LC_CLIENT *d, LC_SERVICE_CLOSE_FN fn);

CHIPCARD_API
void LC_Service_SetCommandFn(LC_CLIENT *d, LC_SERVICE_COMMAND_FN fn);

CHIPCARD_API
void LC_Service_SetGetErrorTextFn(LC_CLIENT *d,
                                  LC_SERVICE_GETERRORTEXT_FN fn);

CHIPCARD_API
void LC_Service_SetWorkFn(LC_CLIENT *d, LC_SERVICE_WORK_FN fn);




CHIPCARD_API
LC_SERVICECLIENT *LC_Service_FindClientById(const LC_CLIENT *d,
                                            GWEN_TYPE_UINT32 id);

CHIPCARD_API
void LC_Service_AddClient(LC_CLIENT *d, LC_SERVICECLIENT *cl);

CHIPCARD_API
void LC_Service_DelClient(LC_CLIENT *d, LC_SERVICECLIENT *cl);

CHIPCARD_API
int LC_Service_Connect(LC_CLIENT *d,
                       const char *code,
                       const char *text);







#endif /* CHIPCARD_SERVICE_SERVICE_H */




