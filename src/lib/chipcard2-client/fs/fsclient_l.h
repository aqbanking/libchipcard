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
#include "fsnode_l.h"


LC_FS_CLIENT *LC_FSClient_new();
void LC_FSClient_free(LC_FS_CLIENT *fcl);

GWEN_TYPE_UINT32 LC_FSClient_GetId(const LC_FS_CLIENT *fcl);
LC_FS_NODE_HANDLE_LIST2 *LC_FSClient_GetHandles(const LC_FS_CLIENT *fcl);
LC_FS_NODE *LC_FSClient_GetWorkingDirNode(const LC_FS_CLIENT *fcl);
const char *LC_FSClient_GetWorkingDirPath(const LC_FS_CLIENT *fcl);



#endif /* LC_FS_CLIENT_L_H */
