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


#ifndef LC_RFS_H
#define LC_RFS_H

#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/types.h>
#include <gwenhywfar/db.h>

#include <chipcard2-client/fs/fsnode.h>
#include <stdio.h>


typedef struct LC_RFS LC_RFS;

GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_RFS, CHIPCARD_API)


typedef int (*LC_RFS_EXCHANGE_FN)(LC_RFS *fs,
                                  GWEN_DB_NODE *dbRequest,
                                  GWEN_DB_NODE *dbResponse);


LC_RFS *LC_RFS_new();
void LC_RFS_free(LC_RFS *fs);


void LC_RFS_SetExchangeFn(LC_RFS *fs, LC_RFS_EXCHANGE_FN fn);


GWEN_TYPE_UINT32 LC_RFS_CreateClient(LC_RFS *fs);
void LC_RFS_DestroyClient(LC_RFS *fs, GWEN_TYPE_UINT32 clid);


int LC_RFS_ChangeWorkingDir(LC_RFS *fs,
                           GWEN_TYPE_UINT32 clid,
                           const char *path);

int LC_RFS_OpenDir(LC_RFS *fs,
                  GWEN_TYPE_UINT32 clid,
                  const char *path,
                  GWEN_TYPE_UINT32 *pHid);

int LC_RFS_MkDir(LC_RFS *fs,
                GWEN_TYPE_UINT32 clid,
                const char *path,
                GWEN_TYPE_UINT32 mode,
                GWEN_TYPE_UINT32 *pHid);

int LC_RFS_ReadDir(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid,
                   GWEN_STRINGLIST2 *sl);

int LC_RFS_CloseDir(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid);

int LC_RFS_OpenFile(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   const char *path,
                   GWEN_TYPE_UINT32 mode,
                   GWEN_TYPE_UINT32 *pHid);

int LC_RFS_CreateFile(LC_RFS *fs,
                     GWEN_TYPE_UINT32 clid,
                     const char *path,
                     GWEN_TYPE_UINT32 mode,
                     GWEN_TYPE_UINT32 *pHid);

int LC_RFS_CloseFile(LC_RFS *fs,
                     GWEN_TYPE_UINT32 clid,
                     GWEN_TYPE_UINT32 hid);

int LC_RFS_ReadFile(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid,
                   GWEN_TYPE_UINT32 offset,
                   GWEN_TYPE_UINT32 len,
                   GWEN_BUFFER *buf);

int LC_RFS_Unlink(LC_RFS *fs,
                  GWEN_TYPE_UINT32 clid,
                  const char *path);


int LC_RFS_Stat(LC_RFS *fs,
                GWEN_TYPE_UINT32 clid,
                const char *path,
                LC_FS_STAT **pStat);



#endif /* LC_RFS_H */




