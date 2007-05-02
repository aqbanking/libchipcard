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



#ifndef CHIPCARD_SERVICE_CLIENT_H
#define CHIPCARD_SERVICE_CLIENT_H

typedef struct LC_SERVICECLIENT LC_SERVICECLIENT;


#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>

#include <chipcard3/chipcard3.h>


GWEN_LIST_FUNCTION_LIB_DEFS(LC_SERVICECLIENT, LC_ServiceClient, CHIPCARD_API);
GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_SERVICECLIENT, CHIPCARD_API);




CHIPCARD_API
LC_SERVICECLIENT *LC_ServiceClient_new(GWEN_TYPE_UINT32 id);

CHIPCARD_API
void LC_ServiceClient_free(LC_SERVICECLIENT *cl);


CHIPCARD_API
GWEN_TYPE_UINT32 LC_ServiceClient_GetClientId(const LC_SERVICECLIENT *cl);


CHIPCARD_API
const char *LC_ServiceClient_GetUserName(const LC_SERVICECLIENT *cl);

CHIPCARD_API
void LC_ServiceClient_SetUserName(LC_SERVICECLIENT *cl, const char *s);

#endif /* CHIPCARD_SERVICE_CLIENT_H */





