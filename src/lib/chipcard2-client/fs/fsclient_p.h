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


#ifndef LC_FS_CLIENT_P_H
#define LC_FS_CLIENT_P_H


#include "fsclient_l.h"

struct LC_FS_CLIENT {
  GWEN_LIST_ELEMENT(LC_FS_CLIENT);
  LC_FS *fileSystem;
  GWEN_TYPE_UINT32 id;
  LC_FS_NODE_HANDLE_LIST *handles;
  LC_FS_PATH_CTX *workingCtx;
  GWEN_TYPE_UINT32 lastHandleId;
};


#endif /* LC_FS_CLIENT_P_H */
