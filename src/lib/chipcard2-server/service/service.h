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

#include <chipcard2-server/service/client.h>


typedef enum {
  LC_ServiceCheckArgsResultOk=0,
  LC_ServiceCheckArgsResultError,
  LC_ServiceCheckArgsResultVersion,
  LC_ServiceCheckArgsResultHelp
} LC_SERVICE_CHECKARGS_RESULT;


GWEN_INHERIT_FUNCTION_DEFS(LC_SERVICE);


typedef GWEN_TYPE_UINT32 (*LC_SERVICE_OPEN_FN)(LC_SERVICE *d,
                                               LC_SERVICECLIENT *cl);

typedef GWEN_TYPE_UINT32 (*LC_SERVICE_CLOSE_FN)(LC_SERVICE *d,
                                                LC_SERVICECLIENT *cl);

typedef GWEN_TYPE_UINT32 (*LC_SERVICE_COMMAND_FN)(LC_SERVICE *d,
                                                  LC_SERVICECLIENT *cl,
                                                  GWEN_DB_NODE *dbRequest,
                                                  GWEN_DB_NODE *dbResponse);

typedef const char* (*LC_SERVICE_GETERRORTEXT_FN)(LC_SERVICE *d,
                                                  GWEN_TYPE_UINT32 err);


void LC_Service_Usage(const char *prgName);

LC_SERVICE *LC_Service_new(int argc, char **argv);
void LC_Service_free(LC_SERVICE *d);

const char *LC_Service_GetServiceDataDir(const LC_SERVICE *d);
const char *LC_Service_GetLibraryFile(const LC_SERVICE *d);
const char *LC_Service_GetServiceId(const LC_SERVICE *d);

LC_SERVICECLIENT_LIST *LC_Service_GetClients(const LC_SERVICE *d);


GWEN_TYPE_UINT32 LC_Service_SendCommand(LC_SERVICE *d,
                                       GWEN_DB_NODE *dbCommand);
int LC_Service_SendResponse(LC_SERVICE *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand);

int LC_Service_SendResult(LC_SERVICE *d,
                         GWEN_TYPE_UINT32 rid,
                         const char *name,
                         const char *code,
                         const char *text);

int LC_Service_RemoveCommand(LC_SERVICE *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound);

GWEN_TYPE_UINT32 LC_Service_GetNextInRequest(LC_SERVICE *d);
GWEN_DB_NODE *LC_Service_GetInRequestData(LC_SERVICE *d,
                                         GWEN_TYPE_UINT32 rid);

int LC_Service_Work(LC_SERVICE *d, int timeout, int maxmsg);


GWEN_TYPE_UINT32 LC_Service_Open(LC_SERVICE *d,
                                 LC_SERVICECLIENT *cl);

GWEN_TYPE_UINT32 LC_Service_Close(LC_SERVICE *d,
                                  LC_SERVICECLIENT *cl);

GWEN_TYPE_UINT32 LC_Service_Command(LC_SERVICE *d,
                                    LC_SERVICECLIENT *cl,
                                    GWEN_DB_NODE *dbRequest,
                                    GWEN_DB_NODE *dbResponse);

const char *LC_Service_GetErrorText(LC_SERVICE *d,
                                    GWEN_TYPE_UINT32 err);


void LC_Service_SetOpenFn(LC_SERVICE *d, LC_SERVICE_OPEN_FN fn);
void LC_Service_SetCloseFn(LC_SERVICE *d, LC_SERVICE_CLOSE_FN fn);
void LC_Service_SetGetErrorTextFn(LC_SERVICE *d,
                                  LC_SERVICE_GETERRORTEXT_FN fn);




LC_SERVICECLIENT *LC_Service_FindClientById(const LC_SERVICE *d,
                                     GWEN_TYPE_UINT32 id);

void LC_Service_AddClient(LC_SERVICE *d, LC_SERVICECLIENT *cl);
void LC_Service_DelClient(LC_SERVICE *d, LC_SERVICECLIENT *cl);

int LC_Service_Connect(LC_SERVICE *d,
                       const char *code,
                       const char *text);
void LC_Service_Disconnect(LC_SERVICE *d);







#endif /* CHIPCARD_SERVICE_SERVICE_H */




