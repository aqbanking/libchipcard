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

#include "fsmem_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(LC_FS_NODE, LC_FSMEM_NODE);
GWEN_INHERIT(LC_FS_MODULE, LC_FSMEM_MODULE);




LC_FS_NODE *LC_FSMemNode_new(LC_FS_MODULE *fs, const char *name){
  LC_FS_NODE *n;
  LC_FSMEM_NODE *mn;

  assert(fs);
  assert(name);
  n=LC_FSNode_new(fs);
  GWEN_NEW_OBJECT(LC_FSMEM_NODE, mn);
  GWEN_INHERIT_SETDATA(LC_FS_NODE, LC_FSMEM_NODE,
		      n, mn, LC_FSMemNode_FreeData);
  mn->children=LC_FSNode_List_new();
  mn->data=GWEN_Buffer_new(0, 1024, 0, 1);
  mn->name=strdup(name);

  return n;
}



void LC_FSMemNode_FreeData(void *bp, void *p) {
  LC_FSMEM_NODE *mn;
  LC_FS_NODE *n;
  LC_FS_NODE *nn;

  mn=(LC_FSMEM_NODE*)p;
  n=(LC_FS_NODE*)bp;

  /* unlink from all children */
  nn=LC_FSNode_List_First(mn->children);
  while(nn) {
    LC_FSMemNode_SetParent(nn, 0);
    nn=LC_FSNode_List_Next(nn);
  }
  free(mn->name);
  GWEN_Buffer_free(mn->data);
  LC_FSNode_List_free(mn->children);
  GWEN_FREE_OBJECT(mn);
}



LC_FS_NODE_LIST *LC_FSMemNode_GetChildren(const LC_FS_NODE *n) {
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  return mn->children;
}



void LC_FSMemNode_AddChild(LC_FS_NODE *n, LC_FS_NODE *nchild){
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  LC_FSNode_List_Add(nchild, mn->children);
}



LC_FS_NODE *LC_FSMemNode_GetParent(const LC_FS_NODE *n) {
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  return mn->parent;
}



void LC_FSMemNode_SetParent(LC_FS_NODE *n, LC_FS_NODE *p) {
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  mn->parent=p;
}



const char *LC_FSMemNode_GetName(const LC_FS_NODE *n) {
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  return mn->name;
}



void LC_FSMemNode_SetName(LC_FS_NODE *n, const char *name){
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  free(mn->name);
  if (name)
    mn->name=strdup(name);
  else
    mn->name=0;
}



GWEN_BUFFER *LC_FSMemNode_GetDataBuffer(const LC_FS_NODE *n) {
  LC_FSMEM_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, n);
  assert(mn);

  return mn->data;
}



void LC_FSMemNode_Dump(LC_FS_NODE *node, FILE *f, int indent){
  LC_FSMEM_NODE *mn;
  int i;
  GWEN_TYPE_UINT32 fl;

  assert(node);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSMEM_NODE, node);
  assert(mn);

  for (i=0; i<indent; i++)
    fprintf(f, " ");

  fprintf(f, "%s ", mn->name);
  fl=LC_FSNode_GetFileMode(node);
  if ((fl & LC_FS_NODE_MODE_FTYPE_MASK)==LC_FS_NODE_MODE_FTYPE_FILE)
    fprintf(f, "-");
  else if ((fl & LC_FS_NODE_MODE_FTYPE_MASK)==LC_FS_NODE_MODE_FTYPE_DIR)
    fprintf(f, "d");
  else {
    DBG_ERROR(0, "Unknown file type %08x (%08x)\n",
              fl, fl & LC_FS_NODE_MODE_FTYPE_MASK);
    fprintf(f, "?");
  }
  if (fl & LC_FS_NODE_MODE_RIGHTS_OWNER_READ)
    fprintf(f, "r");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE)
    fprintf(f, "w");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC)
    fprintf(f, "x");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_GROUP_READ)
    fprintf(f, "r");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_GROUP_WRITE)
    fprintf(f, "w");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_GROUP_EXEC)
    fprintf(f, "x");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_OTHER_READ)
    fprintf(f, "r");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_OTHER_WRITE)
    fprintf(f, "w");
  else
    fprintf(f, "-");
  if (fl & LC_FS_NODE_MODE_RIGHTS_OTHER_EXEC)
    fprintf(f, "x");
  else
    fprintf(f, "-");

  fprintf(f, " ");

  fl=LC_FSNode_GetFileSize(node);
  fprintf(f, GWEN_TYPE_TMPL_UINT32, fl);

  fprintf(f, "\n");
}











LC_FS_MODULE *LC_FSMemModule_new(){
  LC_FS_MODULE *mod;
  LC_FSMEM_MODULE *modm;

  mod=LC_FSModule_new();
  GWEN_NEW_OBJECT(LC_FSMEM_MODULE, modm);
  GWEN_INHERIT_SETDATA(LC_FS_MODULE, LC_FSMEM_MODULE,
		       mod, modm, LC_FSMemModule_FreeData);

  LC_FSModule_SetMountFn(mod, LC_FSMemModule_Mount);
  LC_FSModule_SetUnmountFn(mod, LC_FSMemModule_Unmount);
  LC_FSModule_SetOpenDirFn(mod, LC_FSMemModule_OpenDir);
  LC_FSModule_SetMkDirFn(mod, LC_FSMemModule_MkDir);
  LC_FSModule_SetReadDirFn(mod, LC_FSMemModule_ReadDir);
  LC_FSModule_SetCloseDirFn(mod, LC_FSMemModule_CloseDir);
  LC_FSModule_SetOpenFileFn(mod, LC_FSMemModule_OpenFile);
  LC_FSModule_SetCreateFileFn(mod, LC_FSMemModule_CreateFile);
  LC_FSModule_SetCloseFileFn(mod, LC_FSMemModule_CloseFile);
  LC_FSModule_SetReadFileFn(mod, LC_FSMemModule_ReadFile);
  LC_FSModule_SetWriteFileFileFn(mod, LC_FSMemModule_WriteFile);
  LC_FSModule_SetLookupFn(mod, LC_FSMemModule_Lookup);
  LC_FSModule_SetDumpFn(mod, LC_FSMemModule_Dump);

  return mod;
}



void LC_FSMemModule_FreeData(void *bp, void *p){
  LC_FSMEM_MODULE *modm;

  modm=(LC_FSMEM_MODULE*)p;
  GWEN_FREE_OBJECT(modm);
}




LC_FS_NODE *LC_FSMemModule__FindNode(LC_FS_MODULE *fs,
				     LC_FS_NODE *node,
				     const char *name){
  LC_FS_NODE *nn;

  DBG_INFO(LC_LOGDOMAIN, "Searching for entry \"%s\"", name);
  nn=LC_FSNode_List_First(LC_FSMemNode_GetChildren(node));
  while(nn) {
    const char *s;

    s=LC_FSMemNode_GetName(nn);
    assert(s);
    if (strcmp(name, s)==0)
      break;
    nn=LC_FSNode_List_Next(nn);
  }

  return nn;
}




int LC_FSMemModule_Mount(LC_FS_MODULE *fs,
                         GWEN_TYPE_UINT32 flags,
                         LC_FS_NODE **nPtr){
  *nPtr=LC_FSMemNode_new(fs, "");
  LC_FSNode_Attach(*nPtr);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_Unmount(LC_FS_MODULE *fs,
                           LC_FS_NODE *node){
  LC_FSNode_free(node);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_OpenDir(LC_FS_MODULE *fs,
			   LC_FS_NODE *node,
			   const char *name,
			   LC_FS_NODE **nPtr){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Opening folder \"%s\"", name);
  n=LC_FSMemModule__FindNode(fs, node, name);
  if (!n) {
    DBG_INFO(0, "here");
    return LC_FS_ErrorNotFound;
  }
  if ((LC_FSNode_GetFileMode(n) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_DIR) {
    DBG_ERROR(0, "Entry \"%s\" is not a folder", name);
    return LC_FS_ErrorNotDir;
  }
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_MkDir(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         const char *name,
			 GWEN_TYPE_UINT32 mode,
			 LC_FS_NODE **nPtr){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Creating folder \"%s\"", name);
  n=LC_FSMemModule__FindNode(fs, node, name);
  if (n) {
    DBG_INFO(0, "Entry \"%s\" already exists", name);
    return LC_FS_ErrorExists;
  }
  n=LC_FSMemNode_new(fs, name);
  mode&=LC_FS_NODE_MODE_FTYPE_MASK;
  mode|=LC_FS_NODE_MODE_FTYPE_DIR;
  LC_FSNode_SetFileMode(n, mode);
  LC_FSMemNode_AddChild(node, n);
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_ReadDir(LC_FS_MODULE *fs,
			   LC_FS_NODE *node,
			   GWEN_STRINGLIST2 *sl){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  n=LC_FSNode_List_First(LC_FSMemNode_GetChildren(node));
  while(n) {
    const char *s;

    s=LC_FSMemNode_GetName(n);
    assert(s);
    GWEN_StringList2_AppendString(sl, s, 0,
				  GWEN_StringList2_IntertModeAlwaysAdd);
    n=LC_FSNode_List_Next(n);
  } /* while */

  return LC_FS_ErrorNone;
}



int LC_FSMemModule_CloseDir(LC_FS_MODULE *fs, LC_FS_NODE *node){
  LC_FSMEM_MODULE *modm;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_DIR) {
    DBG_ERROR(0, "Node is not a folder");
    return LC_FS_ErrorNotDir;
  }
  LC_FSModule_DecActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_OpenFile(LC_FS_MODULE *fs,
			    LC_FS_NODE *node,
			    const char *name,
			    LC_FS_NODE **nPtr){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Opening file \"%s\"", name);

  n=LC_FSMemModule__FindNode(fs, node, name);
  if (!n) {
    DBG_INFO(0, "here");
    return LC_FS_ErrorNotFound;
  }
  if ((LC_FSNode_GetFileMode(n) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Entry \"%s\" is not a file", name);
    return LC_FS_ErrorNotFile;
  }
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_CreateFile(LC_FS_MODULE *fs,
			      LC_FS_NODE *node,
			      const char *name,
			      GWEN_TYPE_UINT32 mode,
			      LC_FS_NODE **nPtr){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Creating file \"%s\"", name);

  n=LC_FSMemModule__FindNode(fs, node, name);
  if (n) {
    DBG_INFO(0, "Entry \"%s\" already exists", name);
    return LC_FS_ErrorExists;
  }
  n=LC_FSMemNode_new(fs, name);
  mode&=LC_FS_NODE_MODE_FTYPE_MASK;
  mode|=LC_FS_NODE_MODE_FTYPE_FILE;
  LC_FSNode_SetFileMode(n, mode);
  LC_FSMemNode_AddChild(node, n);
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_CloseFile(LC_FS_MODULE *fs, LC_FS_NODE *node){
  LC_FSMEM_MODULE *modm;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotDir;
  }
  LC_FSModule_DecActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_ReadFile(LC_FS_MODULE *fs,
			    LC_FS_NODE *node,
			    GWEN_TYPE_UINT32 offset,
			    GWEN_TYPE_UINT32 len,
			    GWEN_BUFFER *buf){
  LC_FSMEM_MODULE *modm;
  GWEN_TYPE_UINT32 nlen;
  GWEN_BUFFER *nbuf;
  const char *p;
  GWEN_TYPE_UINT32 bytes;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotDir;
  }

  nbuf=LC_FSMemNode_GetDataBuffer(node);
  assert(nbuf);
  nlen=LC_FSNode_GetFileSize(node);
  assert(nlen==GWEN_Buffer_GetUsedBytes(nbuf));
  if (nlen<offset) {
    GWEN_Buffer_FillWithBytes(buf, 0, len);
    return LC_FS_ErrorNone;
  }

  p=GWEN_Buffer_GetStart(nbuf)+offset;
  nlen-=offset;
  bytes=len;
  if (nlen<bytes)
    bytes=nlen;
  GWEN_Buffer_AppendBytes(buf, p, bytes);
  len-=bytes;
  if (len)
    GWEN_Buffer_FillWithBytes(buf, 0, len);
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_WriteFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node,
			     GWEN_TYPE_UINT32 offset,
			     GWEN_BUFFER *buf){
  LC_FSMEM_MODULE *modm;
  GWEN_BUFFER *nbuf;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotDir;
  }

  nbuf=LC_FSMemNode_GetDataBuffer(node);
  assert(nbuf);

  if (GWEN_Buffer_AllocRoom(nbuf, offset+GWEN_Buffer_GetUsedBytes(buf))) {
    DBG_ERROR(0, "Could not allocate memory");
    return LC_FS_ErrorFull;
  }
  GWEN_Buffer_SetPos(nbuf, offset);
  GWEN_Buffer_AppendBuffer(nbuf, buf);
  LC_FSNode_SetFileSize(node, GWEN_Buffer_GetUsedBytes(nbuf));
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_Lookup(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          const char *name,
			  LC_FS_NODE **nPtr){
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Searching for entry \"%s\"", name);

  n=LC_FSMemModule__FindNode(fs, node, name);
  if (!n) {
    DBG_INFO(0, "here");
    return LC_FS_ErrorNotFound;
  }
  *nPtr=n;
  return LC_FS_ErrorNone;
}



int LC_FSMemModule_Dump(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        FILE *f,
                        int indent) {
  LC_FSMEM_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSMEM_MODULE, fs);
  assert(modm);

  LC_FSMemNode_Dump(node, f, indent);

  n=LC_FSNode_List_First(LC_FSMemNode_GetChildren(node));
  while(n) {
    LC_FSMemModule_Dump(fs, n, f, indent+2);
    n=LC_FSNode_List_Next(n);
  } /* while */

  return LC_FS_ErrorNone;
}
















