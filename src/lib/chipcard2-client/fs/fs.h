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

#include <stdio.h>
#include <time.h>


#define LC_FS_MOUNT_FLAGS_READONLY 0x00000001


#define LC_FS_MODE_RIGHTS_OWNER_MASK  0x00000700
#define LC_FS_MODE_RIGHTS_OWNER_EXEC  0x00000100
#define LC_FS_MODE_RIGHTS_OWNER_WRITE 0x00000200
#define LC_FS_MODE_RIGHTS_OWNER_READ  0x00000400

#define LC_FS_MODE_RIGHTS_GROUP_MASK  0x00000070
#define LC_FS_MODE_RIGHTS_GROUP_EXEC  0x00000010
#define LC_FS_MODE_RIGHTS_GROUP_WRITE 0x00000020
#define LC_FS_MODE_RIGHTS_GROUP_READ  0x00000040

#define LC_FS_MODE_RIGHTS_OTHER_MASK  0x00000007
#define LC_FS_MODE_RIGHTS_OTHER_EXEC  0x00000001
#define LC_FS_MODE_RIGHTS_OTHER_WRITE 0x00000002
#define LC_FS_MODE_RIGHTS_OTHER_READ  0x00000004

#define LC_FS_MODE_FTYPE_MASK         0x0000f000
#define LC_FS_MODE_FTYPE_FILE         0x00001000
#define LC_FS_MODE_FTYPE_DIR          0x00002000



typedef struct LC_FS_STAT LC_FS_STAT;
typedef struct LC_FS LC_FS;


#include <chipcard2-client/fs/fsnode.h>
#include <chipcard2-client/fs/fsmodule.h>


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


int LC_FS_ReadDir(LC_FS *fs,
                  GWEN_TYPE_UINT32 clid,
                  GWEN_TYPE_UINT32 hid,
                  GWEN_STRINGLIST2 *sl);

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

int LC_FS_Unlink(LC_FS *fs,
                 GWEN_TYPE_UINT32 clid,
                 const char *path);

int LC_FS_Stat(LC_FS *fs,
               GWEN_TYPE_UINT32 clid,
               const char *path,
               LC_FS_STAT **pStat);


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





LC_FS_STAT *LC_FSStat_new();
void LC_FSStat_free(LC_FS_STAT *st);
LC_FS_STAT *LC_FSStat_dup(const LC_FS_STAT *ost);

LC_FS_STAT *LC_FSStat_fromDb(GWEN_DB_NODE *db);
int LC_FSStat_toDb(const LC_FS_STAT *st, GWEN_DB_NODE *db);

GWEN_TYPE_UINT32 LC_FSStat_GetFileMode(const LC_FS_STAT *st);
void LC_FSStat_SetFileMode(LC_FS_STAT *st, GWEN_TYPE_UINT32 m);

GWEN_TYPE_UINT32 LC_FSStat_GetFileSize(const LC_FS_STAT *st);
void LC_FSStat_SetFileSize(LC_FS_STAT *st, GWEN_TYPE_UINT32 s);

time_t LC_FSStat_GetCTime(const LC_FS_STAT *st);
void LC_FSStat_SetCTime(LC_FS_STAT *st, time_t ti);

time_t LC_FSStat_GetATime(const LC_FS_STAT *st);
void LC_FSStat_SetATime(LC_FS_STAT *st, time_t ti);

time_t LC_FSStat_GetMTime(const LC_FS_STAT *st);
void LC_FSStat_SetMTime(LC_FS_STAT *st, time_t ti);


#endif /* LC_FS_H */




