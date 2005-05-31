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


#ifndef LC_FS_H
#define LC_FS_H

#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/types.h>

#include <chipcard2-client/fs/fsnode.h>
#include <chipcard2-client/fs/fsclient.h>
#include <stdio.h>


#define LC_FS_MOUNT_FLAGS_READONLY 0x00000001



typedef struct LC_FS_PATH_CTX LC_FS_PATH_CTX;
typedef struct LC_FS LC_FS;

LC_FS_PATH_CTX *LC_FSPathCtx_new(const char *path, LC_FS_NODE *node);
void LC_FSPathCtx_free(LC_FS_PATH_CTX *ctx);

LC_FS_PATH_CTX *LC_FSPathCtx_dup(const LC_FS_PATH_CTX *octx);


GWEN_BUFFER *LC_FSPathCtx_GetPathBuffer(const LC_FS_PATH_CTX *ctx);
void LC_FSPathCtx_SetPath(LC_FS_PATH_CTX *ctx, const char *path);

LC_FS_NODE *LC_FSPathCtx_GetNode(const LC_FS_PATH_CTX *ctx);
void LC_FSPathCtx_SetNode(LC_FS_PATH_CTX *ctx, LC_FS_NODE *node);



LC_FS *LC_FS_new();
void LC_FS_free(LC_FS *fs);


GWEN_TYPE_UINT32 LC_FS_CreateClient(LC_FS *fs);
void LC_FS_DestroyClient(LC_FS *fs, GWEN_TYPE_UINT32 clid);


int LC_FS_ChangeWorkingDir(LC_FS *fs,
                           GWEN_TYPE_UINT32 clid,
                           const char *path);

int LC_FS_OpenDir(LC_FS *fs,
                  GWEN_TYPE_UINT32 clid,
                  const char *path,
                  GWEN_TYPE_UINT32 *pHid);

int LC_FS_MkDir(LC_FS *fs,
                GWEN_TYPE_UINT32 clid,
                const char *path,
                GWEN_TYPE_UINT32 mode,
                GWEN_TYPE_UINT32 *pHid);

int LC_FS_CloseDir(LC_FS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid);

int LC_FS_OpenFile(LC_FS *fs,
                   GWEN_TYPE_UINT32 clid,
                   const char *path,
                   GWEN_TYPE_UINT32 mode,
                   GWEN_TYPE_UINT32 *pHid);

int LC_FS_CreateFile(LC_FS *fs,
                     GWEN_TYPE_UINT32 clid,
                     const char *path,
                     GWEN_TYPE_UINT32 mode,
                     GWEN_TYPE_UINT32 *pHid);

int LC_FS_CloseFile(LC_FS *fs,
                    GWEN_TYPE_UINT32 clid,
                    GWEN_TYPE_UINT32 hid);

int LC_FS_ReadFile(LC_FS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid,
                   GWEN_TYPE_UINT32 offset,
                   GWEN_TYPE_UINT32 len,
                   GWEN_BUFFER *buf);



int LC_FS_Dump(LC_FS *fs,
               GWEN_TYPE_UINT32 clid,
               const char *path,
               FILE *f,
               int indent);


int LC_FS_Mount(LC_FS *fs,
                LC_FS_MODULE *fsm,
                const char *path);


int LC_FS_HandleRequest(LC_FS *fs,
                        GWEN_DB_NODE *dbRequest,
                        GWEN_DB_NODE *dbResponse);


#endif /* LC_FS_H */




