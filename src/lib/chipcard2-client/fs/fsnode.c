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

#include "fsnode_p.h"

#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/debug.h>



GWEN_LIST_FUNCTIONS(LC_FS_NODE_HANDLE, LC_FSNodeHandle)
GWEN_LIST2_FUNCTIONS(LC_FS_NODE_HANDLE, LC_FSNodeHandle)


LC_FS_NODE *LC_FSNode_new(LC_FS_MODULE *fs){
  LC_FS_NODE *fn;

  assert(fs);
  GWEN_NEW_OBJECT(LC_FS_NODE, fn);
  fn->usageCounter=1;
  fn->fileSystem=fs;
  LC_FSModule_Attach(fn->fileSystem);
  GWEN_LIST_INIT(LC_FS_NODE, fn);
  GWEN_INHERIT_INIT(LC_FS_NODE, fn);

  return fn;
}



void LC_FSNode_free(LC_FS_NODE *fn){
  if (fn) {
    assert(fn->usageCounter);
    if (--(fn->usageCounter)==0) {
      GWEN_INHERIT_FINI(LC_FS_NODE, fn);
      GWEN_LIST_FINI(LC_FS_NODE, fn);
      LC_FSNode_free(fn->mounted);
      LC_FSModule_free(fn->fileSystem);

      GWEN_FREE_OBJECT(fn);
    }
  }
}



void LC_FSNode_Attach(LC_FS_NODE *fn){
  assert(fn);
  assert(fn->usageCounter);
  fn->usageCounter++;
}



GWEN_TYPE_UINT32 LC_FSNode_GetLockedById(const LC_FS_NODE *fn){
  assert(fn);
  return fn->lockedById;
}



void LC_FSNode_SetLockedById(LC_FS_NODE *fn, GWEN_TYPE_UINT32 id){
  assert(fn);
  fn->lockedById=id;
}



GWEN_TYPE_UINT32 LC_FSNode_GetFlags(const LC_FS_NODE *fn){
  assert(fn);
  return fn->flags;
}



void LC_FSNode_SetFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl){
  assert(fn);
  fn->flags=fl;
}



void LC_FSNode_AddFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl){
  assert(fn);
  fn->flags|=fl;
}



void LC_FSNode_SubFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl){
  assert(fn);
  fn->flags&=~fl;
}



LC_FS_NODE *LC_FSNode_GetMounted(const LC_FS_NODE *fn){
  assert(fn);
  return fn->mounted;
}



void LC_FSNode_SetMounted(LC_FS_NODE *fn, LC_FS_NODE *n){
  assert(fn);
  fn->mounted=n;
}



LC_FS_MODULE *LC_FSNode_GetFileSystem(const LC_FS_NODE *fn){
  assert(fn);
  return fn->fileSystem;
}



void LC_FSNode_SetFileSystem(LC_FS_NODE *fn, LC_FS_MODULE *fs){
  assert(fn);
  fn->fileSystem=fs;
}



GWEN_TYPE_UINT32 LC_FSNode_GetFileMode(const LC_FS_NODE *fn){
  assert(fn);
  return fn->fileMode;
}



void LC_FSNode_SetFileMode(LC_FS_NODE *fn, GWEN_TYPE_UINT32 m){
  assert(fn);
  fn->fileMode=m;
}



GWEN_TYPE_UINT32 LC_FSNode_GetFileSize(const LC_FS_NODE *fn){
  assert(fn);
  return fn->fileSize;
}



void LC_FSNode_SetFileSize(LC_FS_NODE *fn, GWEN_TYPE_UINT32 s){
  assert(fn);
  fn->fileSize=s;
}
























LC_FS_NODE_HANDLE *LC_FSNodeHandle_new(const char *name,
                                       LC_FS_NODE *fn){
  LC_FS_NODE_HANDLE *fh;

  assert(fn);
  assert(name);
  GWEN_NEW_OBJECT(LC_FS_NODE_HANDLE, fh);
  fh->usageCounter=1;
  GWEN_LIST_INIT(LC_FS_NODE_HANDLE, fh);
  fh->node=fn;
  fh->name=strdup(name);
  LC_FSNode_Attach(fh->node);
  fh->entryList=GWEN_StringList2_new();

  return fh;
}



void LC_FSNodeHandle_free(LC_FS_NODE_HANDLE *fh){
  if (fh) {
    assert(fh->usageCounter);
    if (--(fh->usageCounter)==0) {
      GWEN_LIST_FINI(LC_FS_NODE_HANDLE, fh);
      GWEN_StringList2Iterator_free(fh->entryIterator);
      GWEN_StringList2_free(fh->entryList);
      LC_FSNode_free(fh->node);
      GWEN_FREE_OBJECT(fh);
    }
  }
}



void LC_FSNodeHandle_Attach(LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  fh->usageCounter++;
}




const char *LC_FSNodeHandle_GetName(const LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  return fh->name;
}



GWEN_TYPE_UINT32 LC_FSNodeHandle_GetFid(const LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  return fh->fid;
}



void LC_FSNodeHandle_SetFid(LC_FS_NODE_HANDLE *fh,
                            GWEN_TYPE_UINT32 id){
  assert(fh);
  assert(fh->usageCounter);
  fh->fid=id;
}



LC_FS_NODE *LC_FSNodeHandle_GetNode(const LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  return fh->node;
}



GWEN_TYPE_UINT32 LC_FSNodeHandle_GetFlags(const LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  return fh->flags;
}



void LC_FSNodeHandle_SetFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl){
  assert(fh);
  assert(fh->usageCounter);
  fh->flags=fl;
}



void LC_FSNodeHandle_AddFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl){
  assert(fh);
  assert(fh->usageCounter);
  fh->flags|=fl;
}



void LC_FSNodeHandle_SubFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl){
  assert(fh);
  assert(fh->usageCounter);
  fh->flags&=~fl;
}



GWEN_TYPE_UINT32 LC_FSNodeHandle_GetFilePointer(const LC_FS_NODE_HANDLE *fh){
  assert(fh);
  assert(fh->usageCounter);
  return fh->fpointer;
}



void LC_FSNodeHandle_SetFilePointer(LC_FS_NODE_HANDLE *fh,
                                    GWEN_TYPE_UINT32 fpos){
  assert(fh);
  assert(fh->usageCounter);
  fh->fpointer=fpos;
}



int LC_FSNodeHandle_GetFirstEntry(LC_FS_NODE_HANDLE *fh,
                                  GWEN_BUFFER *ebuf){
  const char *s;

  assert(fh);
  assert(fh->usageCounter);
  if (fh->entryIterator)
    GWEN_StringList2Iterator_free(fh->entryIterator);
  fh->entryIterator=GWEN_StringList2_First(fh->entryList);
  if (fh->entryIterator) {
    s=GWEN_StringList2Iterator_Data(fh->entryIterator);
    assert(s);
    GWEN_Buffer_AppendString(ebuf, s);
    return 0;
  }
  return -1;
}



int LC_FSNodeHandle_GetNextEntry(LC_FS_NODE_HANDLE *fh,
                                 GWEN_BUFFER *ebuf){
  const char *s;

  assert(fh);
  assert(fh->usageCounter);
  assert(fh->entryIterator);
  s=GWEN_StringList2Iterator_Next(fh->entryIterator);
  if (s) {
    GWEN_Buffer_AppendString(ebuf, s);
    return 0;
  }
  GWEN_StringList2Iterator_free(fh->entryIterator);
  fh->entryIterator=0;
  return -1;
}



void LC_FSNodeHandle_AddEntry(LC_FS_NODE_HANDLE *fh,
                              const char *name){
  GWEN_StringList2_AppendString(fh->entryList, name, 0,
                                GWEN_StringList2_IntertModeAlwaysAdd);
}













