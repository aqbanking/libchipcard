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


#ifndef LC_FS_P_H
#define LC_FS_P_H

#include <chipcard2-client/fs/fs.h>
#include <gwenhywfar/buffer.h>

#include "fsclient_l.h"


struct LC_FS_PATH_CTX {
  GWEN_BUFFER *path;
  LC_FS_NODE *node;
};



struct LC_FS {
  LC_FS_MODULE *rootFsModule;
  LC_FS_NODE *rootFsNode;
  LC_FS_CLIENT_LIST *clients;
};



void *LC_FS__HandlePathElement(const char *entry,
                               void *data,
                               unsigned int flags);

int LC_FS__GetNode(LC_FS *fs,
                   LC_FS_PATH_CTX *ctx,
                   const char *path,
                   GWEN_TYPE_UINT32 flags);


LC_FS_CLIENT *LC_FS__FindClient(LC_FS *fs, GWEN_TYPE_UINT32 id);



#endif /* LC_FS_P_H */
