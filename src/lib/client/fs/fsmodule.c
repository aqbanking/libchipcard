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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fsmodule_p.h"
#include "fs.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_LIST_FUNCTIONS(LC_FS_MODULE, LC_FSModule)
GWEN_INHERIT_FUNCTIONS(LC_FS_MODULE)



LC_FS_MODULE *LC_FSModule_new() {
  LC_FS_MODULE *fs;

  GWEN_NEW_OBJECT(LC_FS_MODULE, fs);
  fs->usage=1;
  GWEN_LIST_INIT(LC_FS_MODULE, fs);
  GWEN_INHERIT_INIT(LC_FS_MODULE, fs);

  return fs;
}



void LC_FSModule_free(LC_FS_MODULE *fs) {
  if (fs) {
    assert(fs->usage);
    if (--(fs->usage)==0) {
      GWEN_INHERIT_FINI(LC_FS_MODULE, fs);
      GWEN_LIST_FINI(LC_FS_MODULE, fs);

      GWEN_FREE_OBJECT(fs);
    }
  }
}



void LC_FSModule_Attach(LC_FS_MODULE *fs) {
  assert(fs);
  fs->usage++;
}



void LC_FSModule_SetMountFn(LC_FS_MODULE *fs, LC_FS_MODULE_MOUNT_FN f){
  assert(fs);
  fs->mountFn=f;
}



LC_FS_MODULE_MOUNT_FN LC_FSModule_GetMountFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->mountFn;
}



void LC_FSModule_SetUnmountFn(LC_FS_MODULE *fs, LC_FS_MODULE_UNMOUNT_FN f){
  assert(fs);
  fs->unmountFn=f;
}



LC_FS_MODULE_UNMOUNT_FN LC_FSModule_GetUnmountFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->unmountFn;
}



void LC_FSModule_SetOpenDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENDIR_FN f){
  assert(fs);
  fs->openDirFn=f;
}



LC_FS_MODULE_OPENDIR_FN LC_FSModule_GetOpenDirFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->openDirFn;
}



void LC_FSModule_SetMkDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_MKDIR_FN f){
  assert(fs);
  fs->mkDirFn=f;
}



LC_FS_MODULE_MKDIR_FN LC_FSModule_GetMkDirFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->mkDirFn;
}



void LC_FSModule_SetReadDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_READDIR_FN f){
  assert(fs);
  fs->readDirFn=f;
}



LC_FS_MODULE_READDIR_FN LC_FSModule_GetReadDirFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->readDirFn;
}



void LC_FSModule_SetCloseDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_CLOSEDIR_FN f){
  assert(fs);
  fs->closeDirFn=f;
}



LC_FS_MODULE_CLOSEDIR_FN LC_FSModule_GetCloseDirFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->closeDirFn;
}



void LC_FSModule_SetOpenFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENFILE_FN f){
  assert(fs);
  fs->openFileFn=f;
}



LC_FS_MODULE_OPENFILE_FN LC_FSModule_GetOpenFileFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->openFileFn;
}



void LC_FSModule_SetCreateFileFn(LC_FS_MODULE *fs,
                                 LC_FS_MODULE_CREATEFILE_FN f){
  assert(fs);
  fs->createFileFn=f;
}



LC_FS_MODULE_CREATEFILE_FN
LC_FSModule_GetCreateFileFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->createFileFn;
}



void LC_FSModule_SetCloseFileFn(LC_FS_MODULE *fs,
                                LC_FS_MODULE_CLOSEFILE_FN f){
  assert(fs);
  fs->closeFileFn=f;
}



LC_FS_MODULE_CLOSEFILE_FN LC_FSModule_GetCloseFileFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->closeFileFn;
}



void LC_FSModule_SetReadFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_READFILE_FN f){
  assert(fs);
  fs->readFileFn=f;
}



LC_FS_MODULE_READFILE_FN LC_FSModule_GetReadFileFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->readFileFn;
}



void LC_FSModule_SetWriteFileFileFn(LC_FS_MODULE *fs,
                                    LC_FS_MODULE_WRITEFILE_FN f){
  assert(fs);
  fs->writeFileFn=f;
}



LC_FS_MODULE_WRITEFILE_FN
LC_FSModule_GetWriteFileFileFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->writeFileFn;
}



void LC_FSModule_SetLookupFn(LC_FS_MODULE *fs, LC_FS_MODULE_LOOKUP_FN f){
  assert(fs);
  fs->lookupFn=f;
}



LC_FS_MODULE_LOOKUP_FN LC_FSModule_GetLookupFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->lookupFn;
}



void LC_FSModule_SetUnlinkFn(LC_FS_MODULE *fs, LC_FS_MODULE_UNLINK_FN f){
  assert(fs);
  fs->unlinkFn=f;
}



LC_FS_MODULE_UNLINK_FN LC_FSModule_GetUnlinkFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->unlinkFn;
}



void LC_FSModule_SetDumpFn(LC_FS_MODULE *fs, LC_FS_MODULE_DUMP_FN f){
  assert(fs);
  fs->dumpFn=f;
}



LC_FS_MODULE_DUMP_FN LC_FSModule_GetDumpFn(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->dumpFn;
}



GWEN_TYPE_UINT32 LC_FSModule_GetMountFlags(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->mountFlags;
}



void LC_FSModule_SetMountFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl){
  assert(fs);
  fs->mountFlags=fl;
}



void LC_FSModule_AddMountFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl){
  assert(fs);
  fs->mountFlags|=fl;
}



void LC_FSModule_SubMountFlags(LC_FS_MODULE *fs, GWEN_TYPE_UINT32 fl){
  assert(fs);
  fs->mountFlags&=~fl;
}



GWEN_TYPE_UINT32 LC_FSModule_GetActiveNodes(const LC_FS_MODULE *fs){
  assert(fs);
  return fs->activeNodes;
}



void LC_FSModule_IncActiveNodes(LC_FS_MODULE *fs){
  assert(fs);
  fs->activeNodes++;
}



void LC_FSModule_DecActiveNodes(LC_FS_MODULE *fs){
  assert(fs);
  fs->activeNodes--;
}






int LC_FSModule_Mount(LC_FS_MODULE *fs,
                      LC_FS_NODE **nPtr){
  assert(fs);
  if (!fs->mountFn)
    return LC_FS_ErrorNotSupported;
  return fs->mountFn(fs, nPtr);
}



int LC_FSModule_Unmount(LC_FS_MODULE *fs, LC_FS_NODE *node){
  assert(fs);
  if (!fs->unmountFn)
    return LC_FS_ErrorNotSupported;
  return fs->unmountFn(fs, node);
}



int LC_FSModule_OpenDir(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        const char *name,
                        LC_FS_NODE **nPtr){
  assert(fs);
  if (!fs->openDirFn)
    return LC_FS_ErrorNotSupported;
  return fs->openDirFn(fs, node, name, nPtr);
}



int LC_FSModule_MkDir(LC_FS_MODULE *fs,
                      LC_FS_NODE *node,
                      const char *name,
                      GWEN_TYPE_UINT32 flags,
                      LC_FS_NODE **nPtr){
  assert(fs);
  if (fs->mountFlags & LC_FS_MOUNT_FLAGS_READONLY) {
    DBG_ERROR(LC_LOGDOMAIN, "Read-only filesystem");
    return LC_FS_ErrorInvalid;
  }
  if (!fs->mkDirFn)
    return LC_FS_ErrorNotSupported;
  return fs->mkDirFn(fs, node, name, flags, nPtr);
}



int LC_FSModule_ReadDir(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        GWEN_STRINGLIST2 *sl){
  assert(fs);
  if (!fs->readDirFn)
    return LC_FS_ErrorNotSupported;
  return fs->readDirFn(fs, node, sl);
}



int LC_FSModule_CloseDir(LC_FS_MODULE *fs, LC_FS_NODE *node){
  assert(fs);
  if (!fs->closeDirFn)
    return LC_FS_ErrorNotSupported;
  return fs->closeDirFn(fs, node);
}



int LC_FSModule_OpenFile(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         const char *name,
                         LC_FS_NODE **nPtr){
  assert(fs);
  if (!fs->openFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->openFileFn(fs, node, name, nPtr);
}



int LC_FSModule_CreateFile(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           const char *name,
                           GWEN_TYPE_UINT32 flags,
                           LC_FS_NODE **nPtr){
  assert(fs);
  if (fs->mountFlags & LC_FS_MOUNT_FLAGS_READONLY) {
    DBG_ERROR(LC_LOGDOMAIN, "Read-only filesystem");
    return LC_FS_ErrorInvalid;
  }
  if (!fs->createFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->createFileFn(fs, node, name, flags, nPtr);
}



int LC_FSModule_CloseFile(LC_FS_MODULE *fs, LC_FS_NODE *node){
  assert(fs);
  if (!fs->closeFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->closeFileFn(fs, node);
}



int LC_FSModule_ReadFile(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         GWEN_TYPE_UINT32 mode,
                         GWEN_TYPE_UINT32 offset,
                         GWEN_TYPE_UINT32 len,
                         GWEN_BUFFER *buf){
  assert(fs);
  if (!fs->readFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->readFileFn(fs, node, mode, offset, len, buf);
}



int LC_FSModule_WriteFile(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          GWEN_TYPE_UINT32 mode,
                          GWEN_TYPE_UINT32 offset,
                          GWEN_BUFFER *buf){
  assert(fs);
  if (fs->mountFlags & LC_FS_MOUNT_FLAGS_READONLY) {
    DBG_ERROR(LC_LOGDOMAIN, "Read-only filesystem");
    return LC_FS_ErrorInvalid;
  }
  if (!fs->writeFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->writeFileFn(fs, node, mode, offset, buf);
}



int LC_FSModule_Lookup(LC_FS_MODULE *fs,
                       LC_FS_NODE *node,
                       const char *name,
                       LC_FS_NODE **nPtr) {
  assert(fs);
  if (!fs->lookupFn)
    return LC_FS_ErrorNotSupported;
  return fs->lookupFn(fs, node, name, nPtr);
}



int LC_FSModule_Unlink(LC_FS_MODULE *fs,
                       LC_FS_NODE *node,
                       const char *name) {
  assert(fs);
  if (!fs->unlinkFn)
    return LC_FS_ErrorNotSupported;
  return fs->unlinkFn(fs, node, name);
}



int LC_FSModule_Dump(LC_FS_MODULE *fs,
                     LC_FS_NODE *node,
                     FILE *f,
                     int indent){
  assert(fs);
  if (!fs->dumpFn)
    return LC_FS_ErrorNotSupported;
  return fs->dumpFn(fs, node, f, indent);
}






