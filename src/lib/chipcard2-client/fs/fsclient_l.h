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


#ifndef LC_FS_CLIENT_L_H
#define LC_FS_CLIENT_L_H


#include <chipcard2-client/fs/fsclient.h>
#include <chipcard2-client/fs/fs.h>
#include <gwenhywfar/misc.h>

#include "fsnode_l.h"


GWEN_LIST_FUNCTION_DEFS(LC_FS_CLIENT, LC_FSClient)


LC_FS_CLIENT *LC_FSClient_new(LC_FS *fs, GWEN_TYPE_UINT32 id);
void LC_FSClient_free(LC_FS_CLIENT *fcl);

LC_FS *LC_FSClient_GetFileSystem(const LC_FS_CLIENT *fcl);


GWEN_TYPE_UINT32 LC_FSClient_GetId(const LC_FS_CLIENT *fcl);
LC_FS_NODE_HANDLE_LIST *LC_FSClient_GetHandles(const LC_FS_CLIENT *fcl);

LC_FS_PATH_CTX *LC_FSClient_GetWorkingCtx(const LC_FS_CLIENT *fcl);
void LC_FSClient_SetWorkingCtx(LC_FS_CLIENT *fcl, LC_FS_PATH_CTX *ctx);


GWEN_TYPE_UINT32 LC_FSClient_GetNextHandleId(LC_FS_CLIENT *fcl);


void LC_FSClient_AddNodeHandle(LC_FS_CLIENT *fcl, LC_FS_NODE_HANDLE *hdl);

LC_FS_NODE_HANDLE *LC_FSClient_FindHandle(LC_FS_CLIENT *fcl,
                                          GWEN_TYPE_UINT32 hid);

#endif /* LC_FS_CLIENT_L_H */


