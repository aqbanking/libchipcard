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


#ifndef LC_FS_MODULE_H
#define LC_FS_MODULE_H


#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/stringlist2.h>
#include <gwenhywfar/buffer.h>

#include <chipcard2/chipcard2.h>


enum LC_FS_ERROR {
  LC_FS_ErrorNone=0,
  LC_FS_ErrorGeneric,
  LC_FS_ErrorNotSupported,
  LC_FS_ErrorNotFound,
  LC_FS_ErrorNotFile,
  LC_FS_ErrorNotDir,
  LC_FS_ErrorExists,
  LC_FS_ErrorFull,
  LC_FS_ErrorInvalid
};



typedef struct LC_FS_MODULE LC_FS_MODULE;

GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_FS_MODULE, LC_CLIENT_API)


#include <chipcard2-client/fs/fsnode.h>


LC_FS_MODULE *LC_FSModule_new();
void LC_FSModule_free(LC_FS_MODULE *fs);
void LC_FSModule_Attach(LC_FS_MODULE *fs);



typedef int (*LC_FS_MODULE_MOUNT_FN)(LC_FS_MODULE *fs,
                                     GWEN_TYPE_UINT32 flags,
                                     LC_FS_NODE **nPtr);
typedef int (*LC_FS_MODULE_UNMOUNT_FN)(LC_FS_MODULE *fs,
                                       LC_FS_NODE *node);


typedef int (*LC_FS_MODULE_OPENDIR_FN)(LC_FS_MODULE *fs,
                                       LC_FS_NODE *node,
                                       const char *name,
                                       LC_FS_NODE **nPtr);

typedef int (*LC_FS_MODULE_MKDIR_FN)(LC_FS_MODULE *fs,
                                     LC_FS_NODE *node,
                                     const char *name,
                                     GWEN_TYPE_UINT32 flags,
                                     LC_FS_NODE **nPtr);


typedef int (*LC_FS_MODULE_READDIR_FN)(LC_FS_MODULE *fs,
                                       LC_FS_NODE *node,
                                       GWEN_STRINGLIST2 *sl);

typedef int (*LC_FS_MODULE_CLOSEDIR_FN)(LC_FS_MODULE *fs,
                                        LC_FS_NODE *node);


typedef int (*LC_FS_MODULE_OPENFILE_FN)(LC_FS_MODULE *fs,
                                        LC_FS_NODE *node,
                                        const char *name,
                                        LC_FS_NODE **nPtr);

typedef int (*LC_FS_MODULE_CREATEFILE_FN)(LC_FS_MODULE *fs,
                                          LC_FS_NODE *node,
                                          const char *name,
                                          GWEN_TYPE_UINT32 flags,
                                          LC_FS_NODE **nPtr);

typedef int (*LC_FS_MODULE_CLOSEFILE_FN)(LC_FS_MODULE *fs,
                                         LC_FS_NODE *node);


typedef int (*LC_FS_MODULE_READFILE_FN)(LC_FS_MODULE *fs,
                                        LC_FS_NODE *node,
                                        GWEN_TYPE_UINT32 offset,
                                        GWEN_TYPE_UINT32 len,
                                        GWEN_BUFFER *buf);

typedef int (*LC_FS_MODULE_WRITEFILE_FN)(LC_FS_MODULE *fs,
                                         LC_FS_NODE *node,
                                         GWEN_TYPE_UINT32 offset,
                                         GWEN_BUFFER *buf);

typedef int (*LC_FS_MODULE_LOOKUP_FN)(LC_FS_MODULE *fs,
                                      LC_FS_NODE *node,
                                      const char *name,
                                      LC_FS_NODE **nPtr);
typedef int (*LC_FS_MODULE_DUMP_FN)(LC_FS_MODULE *fs,
                                    LC_FS_NODE *node,
                                    FILE *f,
                                    int indent);



void LC_FSModule_SetMountFn(LC_FS_MODULE *fs, LC_FS_MODULE_MOUNT_FN f);
void LC_FSModule_SetUnmountFn(LC_FS_MODULE *fs, LC_FS_MODULE_UNMOUNT_FN f);
void LC_FSModule_SetOpenDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENDIR_FN f);
void LC_FSModule_SetMkDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_MKDIR_FN f);
void LC_FSModule_SetReadDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_READDIR_FN f);
void LC_FSModule_SetCloseDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_CLOSEDIR_FN f);

void LC_FSModule_SetOpenFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENFILE_FN f);
void LC_FSModule_SetCreateFileFn(LC_FS_MODULE *fs,
                                 LC_FS_MODULE_CREATEFILE_FN f);
void LC_FSModule_SetCloseFileFn(LC_FS_MODULE *fs,
                                LC_FS_MODULE_CLOSEFILE_FN f);
void LC_FSModule_SetReadFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_READFILE_FN f);
void LC_FSModule_SetWriteFileFileFn(LC_FS_MODULE *fs,
                                    LC_FS_MODULE_WRITEFILE_FN f);
void LC_FSModule_SetLookupFn(LC_FS_MODULE *fs, LC_FS_MODULE_LOOKUP_FN f);

void LC_FSModule_SetDumpFn(LC_FS_MODULE *fs, LC_FS_MODULE_DUMP_FN f);


GWEN_TYPE_UINT32 LC_FSModule_GetFlags(const LC_FS_MODULE *fs);
void LC_FSModule_SetFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl);
void LC_FSModule_AddFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl);
void LC_FSModule_SubFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl);
GWEN_TYPE_UINT32 LC_FSModule_GetActiveNodes(const LC_FS_MODULE *fs);
void LC_FSModule_IncActiveNodes(LC_FS_MODULE *fs);
void LC_FSModule_DecActiveNodes(LC_FS_MODULE *fs);


#endif /* LC_FS_MODULE_H */
