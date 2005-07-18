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


#ifndef LC_FS_L_H
#define LC_FS_L_H

#include <chipcard2-client/fs/fs.h>
#include <gwenhywfar/buffer.h>

#include "fs.h"

typedef struct LC_FS_PATH_CTX LC_FS_PATH_CTX;

LC_FS_PATH_CTX *LC_FSPathCtx_new(const char *path, LC_FS_NODE *node);
void LC_FSPathCtx_free(LC_FS_PATH_CTX *ctx);

LC_FS_PATH_CTX *LC_FSPathCtx_dup(const LC_FS_PATH_CTX *octx);


GWEN_BUFFER *LC_FSPathCtx_GetPathBuffer(const LC_FS_PATH_CTX *ctx);
void LC_FSPathCtx_SetPath(LC_FS_PATH_CTX *ctx, const char *path);

LC_FS_NODE *LC_FSPathCtx_GetNode(const LC_FS_PATH_CTX *ctx);
void LC_FSPathCtx_SetNode(LC_FS_PATH_CTX *ctx, LC_FS_NODE *node);



LC_FS_STAT *LC_FSStat_new();


#endif /* LC_FS_L_H */
