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

#include <chipcard2-client/fs/fsnode.h>
#include <gwenhywfar/buffer.h>


typedef struct LC_FS_PATH_CTX LC_FS_PATH_CTX;
struct LC_FS_PATH_CTX {
  GWEN_BUFFER *path;
  LC_FS_NODE *node;
};



struct LC_FS {
  LC_FS_MODULE *rootFsModule;
  LC_FS_NODE *rootFsNode;
};




#endif /* LC_FS_P_H */
