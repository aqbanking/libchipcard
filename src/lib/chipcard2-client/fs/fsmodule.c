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

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>



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



void LC_FSModule_SetUnmountFn(LC_FS_MODULE *fs, LC_FS_MODULE_UNMOUNT_FN f){
  assert(fs);
  fs->unmountFn=f;
}



void LC_FSModule_SetOpenDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENDIR_FN f){
  assert(fs);
  fs->openDirFn=f;
}



void LC_FSModule_SetMkDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_MKDIR_FN f){
  assert(fs);
  fs->mkDirFn=f;
}



void LC_FSModule_SetReadDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_READDIR_FN f){
  assert(fs);
  fs->readDirFn=f;
}



void LC_FSModule_SetCloseDirFn(LC_FS_MODULE *fs, LC_FS_MODULE_CLOSEDIR_FN f){
  assert(fs);
  fs->closeDirFn=f;
}



void LC_FSModule_SetOpenFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_OPENFILE_FN f){
  assert(fs);
  fs->openFileFn=f;
}



void LC_FSModule_SetCloseFileFn(LC_FS_MODULE *fs,
                                LC_FS_MODULE_CLOSEFILE_FN f){
  assert(fs);
  fs->closeFileFn=f;
}



void LC_FSModule_SetReadFileFn(LC_FS_MODULE *fs, LC_FS_MODULE_READFILE_FN f){
  assert(fs);
  fs->readFileFn=f;
}



void LC_FSModule_SetWriteFileFileFn(LC_FS_MODULE *fs,
                                    LC_FS_MODULE_WRITEFILE_FN f){
  assert(fs);
  fs->writeFileFn=f;
}



void LC_FSModule_SetStatFn(LC_FS_MODULE *fs, LC_FS_MODULE_STAT_FN f){
  assert(fs);
  fs->statFn=f;
}






int LC_FSModule_Mount(LC_FS_MODULE *fs){
  assert(fs);
  if (!fs->mountFn)
    return LC_FS_ErrorNotSupported;
  return fs->mountFn(fs);
}



int LC_FSModule_Unmount(LC_FS_MODULE *fs){
  assert(fs);
  if (!fs->unmountFn)
    return LC_FS_ErrorNotSupported;
  return fs->unmountFn(fs);
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
                         GWEN_TYPE_UINT32 offset,
                         GWEN_TYPE_UINT32 len,
                         GWEN_BUFFER *buf){
  assert(fs);
  if (!fs->readFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->readFileFn(fs, node, offset, len, buf);
}



int LC_FSModule_WriteFile(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          GWEN_TYPE_UINT32 offset,
                          GWEN_BUFFER *buf){
  assert(fs);
  if (!fs->writeFileFn)
    return LC_FS_ErrorNotSupported;
  return fs->writeFileFn(fs, node, offset, buf);
}



int LC_FSModule_Stat(LC_FS_MODULE *fs,
                     LC_FS_NODE *node,
                     const char *name,
                     LC_FS_STAT *st){
  assert(fs);
  if (!fs->statFn)
    return LC_FS_ErrorNotSupported;
  return fs->statFn(fs, node, name, st);
}










