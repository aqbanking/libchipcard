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

#include "fsfile_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/directory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>



GWEN_INHERIT(LC_FS_NODE, LC_FSFILE_NODE);
GWEN_INHERIT(LC_FS_MODULE, LC_FSFILE_MODULE);




LC_FS_NODE *LC_FSFileNode_new(LC_FS_MODULE *fs, const char *name){
  LC_FS_NODE *n;
  LC_FSFILE_NODE *mn;

  assert(fs);
  assert(name);
  n=LC_FSNode_new(fs);
  GWEN_NEW_OBJECT(LC_FSFILE_NODE, mn);
  GWEN_INHERIT_SETDATA(LC_FS_NODE, LC_FSFILE_NODE,
		      n, mn, LC_FSFileNode_FreeData);
  mn->children=LC_FSNode_List_new();
  mn->name=strdup(name);

  return n;
}



void LC_FSFileNode_FreeData(void *bp, void *p) {
  LC_FSFILE_NODE *mn;
  LC_FS_NODE *n;
  LC_FS_NODE *nn;

  mn=(LC_FSFILE_NODE*)p;
  n=(LC_FS_NODE*)bp;

  /* unlink from all children */
  nn=LC_FSNode_List_First(mn->children);
  while(nn) {
    LC_FSFileNode_SetParent(nn, 0);
    nn=LC_FSNode_List_Next(nn);
  }
  free(mn->name);
  LC_FSNode_List_free(mn->children);
  GWEN_FREE_OBJECT(mn);
}



LC_FS_NODE_LIST *LC_FSFileNode_GetChildren(const LC_FS_NODE *n) {
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  return mn->children;
}



void LC_FSFileNode_AddChild(LC_FS_NODE *n, LC_FS_NODE *nchild){
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  LC_FSNode_List_Add(nchild, mn->children);
}



LC_FS_NODE *LC_FSFileNode_GetParent(const LC_FS_NODE *n) {
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  return mn->parent;
}



void LC_FSFileNode_SetParent(LC_FS_NODE *n, LC_FS_NODE *p) {
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  mn->parent=p;
}



const char *LC_FSFileNode_GetName(const LC_FS_NODE *n) {
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  return mn->name;
}



void LC_FSFileNode_SetName(LC_FS_NODE *n, const char *name){
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  free(mn->name);
  if (name)
    mn->name=strdup(name);
  else
    mn->name=0;
}



int LC_FSFileNode_GetSampled(const LC_FS_NODE *n) {
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  return mn->sampled;
}



void LC_FSFileNode_SetSampled(LC_FS_NODE *n, int b){
  LC_FSFILE_NODE *mn;

  assert(n);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, n);
  assert(mn);

  mn->sampled=b;
}



void LC_FSFileNode_Dump(LC_FS_NODE *node, FILE *f, int indent){
  LC_FSFILE_NODE *mn;
  int i;
  GWEN_TYPE_UINT32 fl;
  LC_FS_NODE *realNode;

  assert(node);
  mn=GWEN_INHERIT_GETDATA(LC_FS_NODE, LC_FSFILE_NODE, node);
  assert(mn);

  realNode=LC_FSNode_GetMounted(node);
  if (!realNode)
    realNode=node;

  for (i=0; i<indent; i++)
    fprintf(f, " ");

  fprintf(f, "%s ", mn->name);
  fl=LC_FSNode_GetFileMode(realNode);
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

  fl=LC_FSNode_GetFileSize(realNode);
  fprintf(f, GWEN_TYPE_TMPL_UINT32, fl);

  fprintf(f, " [fsfile]\n");
}











LC_FS_MODULE *LC_FSFileModule_new(const char *path){
  LC_FS_MODULE *mod;
  LC_FSFILE_MODULE *modm;

  assert(path);
  mod=LC_FSModule_new();
  GWEN_NEW_OBJECT(LC_FSFILE_MODULE, modm);
  GWEN_INHERIT_SETDATA(LC_FS_MODULE, LC_FSFILE_MODULE,
		       mod, modm, LC_FSFileModule_FreeData);

  modm->path=strdup(path);

  LC_FSModule_SetMountFn(mod, LC_FSFileModule_Mount);
  LC_FSModule_SetUnmountFn(mod, LC_FSFileModule_Unmount);
  LC_FSModule_SetOpenDirFn(mod, LC_FSFileModule_OpenDir);
  LC_FSModule_SetMkDirFn(mod, LC_FSFileModule_MkDir);
  LC_FSModule_SetReadDirFn(mod, LC_FSFileModule_ReadDir);
  LC_FSModule_SetCloseDirFn(mod, LC_FSFileModule_CloseDir);
  LC_FSModule_SetOpenFileFn(mod, LC_FSFileModule_OpenFile);
  LC_FSModule_SetCreateFileFn(mod, LC_FSFileModule_CreateFile);
  LC_FSModule_SetCloseFileFn(mod, LC_FSFileModule_CloseFile);
  LC_FSModule_SetReadFileFn(mod, LC_FSFileModule_ReadFile);
  LC_FSModule_SetWriteFileFileFn(mod, LC_FSFileModule_WriteFile);
  LC_FSModule_SetLookupFn(mod, LC_FSFileModule_Lookup);
  LC_FSModule_SetDumpFn(mod, LC_FSFileModule_Dump);

  return mod;
}



void LC_FSFileModule_FreeData(void *bp, void *p){
  LC_FSFILE_MODULE *modm;

  modm=(LC_FSFILE_MODULE*)p;
  free(modm->path);
  GWEN_FREE_OBJECT(modm);
}




LC_FS_NODE *LC_FSFileModule__FindNode(LC_FS_MODULE *fs,
                                      LC_FS_NODE *node,
                                      const char *name){
  LC_FS_NODE *nn;

  if (!LC_FSFileNode_GetSampled(node)) {
    int rv;
    GWEN_BUFFER *pbuf;

    DBG_INFO(LC_LOGDOMAIN,
             "Node \"%s\" has not been sampled, doing it now",
             LC_FSFileNode_GetName(node));
    pbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_ReserveBytes(pbuf, 128);
    rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
    if (rv) {
      GWEN_Buffer_free(pbuf);
      DBG_INFO(LC_LOGDOMAIN, "here");
      return 0;
    }
    rv=LC_FSFileModule__Dir2Node2(fs, node, GWEN_Buffer_GetStart(pbuf));
    if (rv) {
      GWEN_Buffer_free(pbuf);
      DBG_INFO(LC_LOGDOMAIN, "here");
      return 0;
    }
    GWEN_Buffer_free(pbuf);
    LC_FSFileNode_SetSampled(node, 1);
  }

  DBG_INFO(LC_LOGDOMAIN, "Searching for entry \"%s\"", name);
  nn=LC_FSNode_List_First(LC_FSFileNode_GetChildren(node));
  while(nn) {
    const char *s;

    s=LC_FSFileNode_GetName(nn);
    assert(s);
    if (strcmp(name, s)==0)
      break;
    nn=LC_FSNode_List_Next(nn);
  }

  return nn;
}



int LC_FSFileModule__Dir2Node2(LC_FS_MODULE *fs,
                               LC_FS_NODE *node,
                               const char *path){
  GWEN_DIRECTORYDATA *dir;
  LC_FSFILE_MODULE *modm;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Scanning folder \"%s\"", path);
  dir=GWEN_Directory_new();
  if (!GWEN_Directory_Open(dir, path)) {
    LC_FS_NODE_LIST *nl=0;
    GWEN_BUFFER *pbuf;
    GWEN_TYPE_UINT32 pos;
    char entry[256];
    LC_FS_NODE *n=0;

    pbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(pbuf, path);
#ifdef OS_WIN32
    GWEN_Buffer_AppendByte(pbuf, '\\');
#else
    GWEN_Buffer_AppendByte(pbuf, '/');
#endif
    pos=GWEN_Buffer_GetPos(pbuf);
    nl=LC_FSNode_List_new();
    while(!GWEN_Directory_Read(dir, entry, sizeof(entry))) {
      struct stat st;

      GWEN_Buffer_Crop(pbuf, 0, pos);
      GWEN_Buffer_AppendString(pbuf, entry);
      if (stat(GWEN_Buffer_GetStart(pbuf), &st)) {
      }
      else {
        if (S_ISDIR(st.st_mode) || S_ISREG(st.st_mode)) {
          GWEN_TYPE_UINT32 fmode;

          fmode=0;

          /* owner rights */
          if (st.st_mode & S_IRUSR)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OWNER_READ;
          if (st.st_mode & S_IWUSR)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE;
          if (st.st_mode & S_IXUSR)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC;

          /* group rights */
          if (st.st_mode & S_IRGRP)
            fmode|=LC_FS_NODE_MODE_RIGHTS_GROUP_READ;
          if (st.st_mode & S_IWGRP)
            fmode|=LC_FS_NODE_MODE_RIGHTS_GROUP_WRITE;
          if (st.st_mode & S_IXGRP)
            fmode|=LC_FS_NODE_MODE_RIGHTS_GROUP_EXEC;

          /* other rights */
          if (st.st_mode & S_IROTH)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OTHER_READ;
          if (st.st_mode & S_IWOTH)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OTHER_WRITE;
          if (st.st_mode & S_IXOTH)
            fmode|=LC_FS_NODE_MODE_RIGHTS_OTHER_EXEC;

          /* file type */
          if (S_ISDIR(st.st_mode))
            fmode|=LC_FS_NODE_MODE_FTYPE_DIR;
          else if (S_ISREG(st.st_mode))
            fmode|=LC_FS_NODE_MODE_FTYPE_FILE;

          n=LC_FSFileNode_new(fs, entry);
          LC_FSNode_SetFileSize(n, st.st_size);
          LC_FSNode_SetFileMode(n, fmode);

          /* add to list */
          DBG_INFO(LC_LOGDOMAIN, "Adding entry \"%s\" to node %s",
                   entry,
                   LC_FSFileNode_GetName(node));
          LC_FSNode_List_Add(n, nl);

        } /* if folder or regular file */
        else {
          DBG_INFO(LC_LOGDOMAIN,
                   "Not a folder or regular file, ignoring");
        }
      } /* if stat succeeds */
    } /* if read succeeds */

    GWEN_Buffer_free(pbuf);
    GWEN_Directory_Close(dir);

    n=LC_FSNode_List_First(nl);
    while (n) {
      LC_FS_NODE *n2;

      n2=LC_FSNode_List_Next(n);
      LC_FSNode_List_Del(n);
      LC_FSFileNode_AddChild(node, n);
      n=n2;
    }

    LC_FSNode_List_free(nl);
  }
  else {
    DBG_INFO(LC_LOGDOMAIN, "Could not open dir \"%s\"", path);
    return LC_FS_ErrorNotFound;
  }
  GWEN_Directory_free(dir);

  return LC_FS_ErrorNone;
}



int LC_FSFileModule__GetNodePath(LC_FS_MODULE *fs,
                                 LC_FS_NODE *node,
                                 GWEN_BUFFER *pbuf) {
  LC_FSFILE_MODULE *modm;
  const char *s;

  assert(node);
  assert(pbuf);
  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  while(node) {
    LC_FS_NODE *parentNode;

    parentNode=LC_FSFileNode_GetParent(node);
    if (GWEN_Buffer_GetUsedBytes(pbuf))
      GWEN_Buffer_InsertByte(pbuf, '/');
    s=LC_FSFileNode_GetName(node);
    assert(s);
    DBG_INFO(LC_LOGDOMAIN, "Inserting entry \"%s\"", s);
    GWEN_Buffer_InsertString(pbuf, s);
    node=parentNode;
  }
  if (GWEN_Buffer_GetUsedBytes(pbuf))
    GWEN_Buffer_InsertByte(pbuf, '/');
  GWEN_Buffer_InsertString(pbuf, modm->path);
  DBG_INFO(LC_LOGDOMAIN, "Node path is \"%s\"", GWEN_Buffer_GetStart(pbuf));
  return 0;
}




int LC_FSFileModule_Mount(LC_FS_MODULE *fs,
                          LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *node;
  int rv;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  node=LC_FSFileNode_new(fs, "");
  LC_FSNode_SetFileMode(node,
                        LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC |
                        LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE |
                        LC_FS_NODE_MODE_RIGHTS_OWNER_READ |
                        LC_FS_NODE_MODE_FTYPE_DIR);

  rv=LC_FSFileModule__Dir2Node2(fs, node, modm->path);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    LC_FSNode_free(node);
    return rv;
  }
  LC_FSFileNode_SetSampled(node, 1);

  LC_FSNode_Attach(node);
  *nPtr=node;

  return LC_FS_ErrorNone;
}



int LC_FSFileModule_Unmount(LC_FS_MODULE *fs,
                            LC_FS_NODE *node){
  LC_FSNode_free(node);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_OpenDir(LC_FS_MODULE *fs,
			   LC_FS_NODE *node,
			   const char *name,
			   LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Opening folder \"%s\"", name);
  n=LC_FSFileModule__FindNode(fs, node, name);
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



int LC_FSFileModule_MkDir(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          const char *name,
                          GWEN_TYPE_UINT32 mode,
                          LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;
  int rv;
  GWEN_BUFFER *pbuf;
  int flags;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Creating folder \"%s\"", name);

  n=LC_FSFileModule__FindNode(fs, node, name);
  if (n) {
    DBG_INFO(0, "Entry \"%s\" already exists", name);
    return LC_FS_ErrorExists;
  }

  /* create real folder */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_ReserveBytes(pbuf, 128);
  rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
  if (rv) {
    GWEN_Buffer_free(pbuf);
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

#ifdef OS_WIN32
  GWEN_Buffer_AppendByte(pbuf, '\\');
#else
  GWEN_Buffer_AppendByte(pbuf, '/');
#endif
  GWEN_Buffer_AppendString(pbuf, name);

  DBG_INFO(LC_LOGDOMAIN, "Creating real folder \"%s\"",
           GWEN_Buffer_GetStart(pbuf));

  flags=LC_FSFileModule__FileModeToSys(mode);
  if (mkdir(GWEN_Buffer_GetStart(pbuf), flags)) {
    DBG_ERROR(LC_LOGDOMAIN, "mkdir(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }
  GWEN_Buffer_free(pbuf);

  /* create file node */
  n=LC_FSFileNode_new(fs, name);
  mode&=~LC_FS_NODE_MODE_FTYPE_MASK;
  mode|=LC_FS_NODE_MODE_FTYPE_DIR;
  LC_FSNode_SetFileMode(n, mode);
  LC_FSFileNode_AddChild(node, n);
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_ReadDir(LC_FS_MODULE *fs,
			   LC_FS_NODE *node,
			   GWEN_STRINGLIST2 *sl){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  if (!LC_FSFileNode_GetSampled(node)) {
    int rv;
    GWEN_BUFFER *pbuf;

    DBG_INFO(LC_LOGDOMAIN,
             "Node \"%s\" has not been sampled, doing it now",
             LC_FSFileNode_GetName(node));
    pbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_ReserveBytes(pbuf, 128);
    rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
    if (rv) {
      GWEN_Buffer_free(pbuf);
      DBG_INFO(LC_LOGDOMAIN, "here");
      return 0;
    }
    rv=LC_FSFileModule__Dir2Node2(fs, node, GWEN_Buffer_GetStart(pbuf));
    if (rv) {
      GWEN_Buffer_free(pbuf);
      DBG_INFO(LC_LOGDOMAIN, "here");
      return 0;
    }
    GWEN_Buffer_free(pbuf);
    LC_FSFileNode_SetSampled(node, 1);
  }

  n=LC_FSNode_List_First(LC_FSFileNode_GetChildren(node));
  while(n) {
    const char *s;

    s=LC_FSFileNode_GetName(n);
    assert(s);
    GWEN_StringList2_AppendString(sl, s, 0,
				  GWEN_StringList2_IntertModeAlwaysAdd);
    n=LC_FSNode_List_Next(n);
  } /* while */

  return LC_FS_ErrorNone;
}



int LC_FSFileModule_CloseDir(LC_FS_MODULE *fs, LC_FS_NODE *node){
  LC_FSFILE_MODULE *modm;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_DIR) {
    DBG_ERROR(0, "Node is not a folder");
    return LC_FS_ErrorNotDir;
  }
  LC_FSModule_DecActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_OpenFile(LC_FS_MODULE *fs,
			    LC_FS_NODE *node,
			    const char *name,
			    LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Opening file \"%s\"", name);

  n=LC_FSFileModule__FindNode(fs, node, name);
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



int LC_FSFileModule__FileModeToSys(GWEN_TYPE_UINT32 fm) {
  int res;

  res=0;

  /* owner rights */
  if (fm & LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC)
    res|=S_IXUSR;
  if (fm & LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE)
    res|=S_IWUSR;
  if (fm & LC_FS_NODE_MODE_RIGHTS_OWNER_READ)
    res|=S_IRUSR;

  /* group rights */
  if (fm & LC_FS_NODE_MODE_RIGHTS_GROUP_EXEC)
    res|=S_IXGRP;
  if (fm & LC_FS_NODE_MODE_RIGHTS_GROUP_WRITE)
    res|=S_IWGRP;
  if (fm & LC_FS_NODE_MODE_RIGHTS_GROUP_READ)
    res|=S_IRGRP;

  /* other rights */
  if (fm & LC_FS_NODE_MODE_RIGHTS_OTHER_EXEC)
    res|=S_IXOTH;
  if (fm & LC_FS_NODE_MODE_RIGHTS_OTHER_WRITE)
    res|=S_IWOTH;
  if (fm & LC_FS_NODE_MODE_RIGHTS_OTHER_READ)
    res|=S_IROTH;

  return res;
}


int LC_FSFileModule_CreateFile(LC_FS_MODULE *fs,
                               LC_FS_NODE *node,
                               const char *name,
                               GWEN_TYPE_UINT32 mode,
                               LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;
  int fd;
  int rv;
  GWEN_BUFFER *pbuf;
  int flags;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Creating file \"%s\"", name);

  n=LC_FSFileModule__FindNode(fs, node, name);
  if (n) {
    DBG_INFO(0, "Entry \"%s\" already exists", name);
    return LC_FS_ErrorExists;
  }

  /* create real file */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_ReserveBytes(pbuf, 128);
  rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
  if (rv) {
    GWEN_Buffer_free(pbuf);
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

#ifdef OS_WIN32
  GWEN_Buffer_AppendByte(pbuf, '\\');
#else
  GWEN_Buffer_AppendByte(pbuf, '/');
#endif
  GWEN_Buffer_AppendString(pbuf, name);

  DBG_INFO(LC_LOGDOMAIN, "Creating real file \"%s\"",
           GWEN_Buffer_GetStart(pbuf));

  flags=LC_FSFileModule__FileModeToSys(mode);
  fd=open(GWEN_Buffer_GetStart(pbuf),
          O_RDWR | O_CREAT | O_EXCL,
          flags);
  if (fd==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "open(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }
  GWEN_Buffer_free(pbuf);
  close(fd);

  /* create file node */
  n=LC_FSFileNode_new(fs, name);
  mode&=~LC_FS_NODE_MODE_FTYPE_MASK;
  mode|=LC_FS_NODE_MODE_FTYPE_FILE;
  LC_FSNode_SetFileMode(n, mode);
  LC_FSFileNode_AddChild(node, n);
  *nPtr=n;
  LC_FSModule_IncActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_CloseFile(LC_FS_MODULE *fs, LC_FS_NODE *node){
  LC_FSFILE_MODULE *modm;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotDir;
  }
  LC_FSModule_DecActiveNodes(fs);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_ReadFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node,
                             GWEN_TYPE_UINT32 mode,
                             GWEN_TYPE_UINT32 offset,
                             GWEN_TYPE_UINT32 len,
                             GWEN_BUFFER *buf){
  LC_FSFILE_MODULE *modm;
  int fd;
  int rv;
  GWEN_BUFFER *pbuf;
  int flags;
  struct stat st;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotFile;
  }

  if (!(mode & LC_FS_HANDLE_MODE_READ)) {
    DBG_ERROR(0, "Node is not open for reading");
    return LC_FS_ErrorInvalid;
  }

  flags=0;
  if ((mode & LC_FS_HANDLE_MODE_READ) &&
      (mode & LC_FS_HANDLE_MODE_WRITE))
    flags|=O_RDWR;
  else {
    if (mode & LC_FS_HANDLE_MODE_READ)
      flags|=O_RDONLY;
    if (mode & LC_FS_HANDLE_MODE_WRITE)
      flags|=O_WRONLY;
  }

  /* open real file */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_ReserveBytes(pbuf, 128);
  rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
  if (rv) {
    GWEN_Buffer_free(pbuf);
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

  DBG_INFO(LC_LOGDOMAIN, "Opening real file \"%s\"",
           GWEN_Buffer_GetStart(pbuf));

  fd=open(GWEN_Buffer_GetStart(pbuf), flags);
  if (fd==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "open(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }

  if (((off_t)-1)==lseek(fd, offset, SEEK_SET)) {
    DBG_ERROR(LC_LOGDOMAIN, "lseek(%s, "GWEN_TYPE_TMPL_UINT32"): %s",
              GWEN_Buffer_GetStart(pbuf),
              offset,
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    close(fd);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }

  /* actually read */
  while(len) {
    char buffer[512];
    int size;
    int i;

    size=len;
    if (size>sizeof(buffer))
      size=sizeof(buffer);
    i=read(fd, buffer, size);
    if (i<0) {
      DBG_ERROR(LC_LOGDOMAIN, "read(%s): %s",
                GWEN_Buffer_GetStart(pbuf),
                strerror(errno));
      GWEN_Buffer_free(pbuf);
      close(fd);
      /* TODO: check errno, return matching error code */
      return LC_FS_ErrorGeneric;
    }
    else if (i==0) {
      break;
    }
    GWEN_Buffer_AppendBytes(buf, buffer, i);
    len-=i;
  } /* while */

  close(fd);

  /* update size */
  if (!stat(GWEN_Buffer_GetStart(pbuf), &st)) {
    LC_FSNode_SetFileSize(node, st.st_size);
  }
  GWEN_Buffer_free(pbuf);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_WriteFile(LC_FS_MODULE *fs,
                              LC_FS_NODE *node,
                              GWEN_TYPE_UINT32 mode,
                              GWEN_TYPE_UINT32 offset,
                              GWEN_BUFFER *buf){
  LC_FSFILE_MODULE *modm;
  int fd;
  int rv;
  GWEN_BUFFER *pbuf;
  int flags;
  struct stat st;
  GWEN_TYPE_UINT32 len;
  const char *p;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  if ((LC_FSNode_GetFileMode(node) & LC_FS_NODE_MODE_FTYPE_MASK) !=
      LC_FS_NODE_MODE_FTYPE_FILE) {
    DBG_ERROR(0, "Node is not a file");
    return LC_FS_ErrorNotFile;
  }

  if (!(mode & LC_FS_HANDLE_MODE_WRITE)) {
    DBG_ERROR(0, "Node is not open for writing");
    return LC_FS_ErrorInvalid;
  }

  flags=0;
  if ((mode & LC_FS_HANDLE_MODE_READ) &&
      (mode & LC_FS_HANDLE_MODE_WRITE))
    flags|=O_RDWR;
  else {
    if (mode & LC_FS_HANDLE_MODE_READ)
      flags|=O_RDONLY;
    if (mode & LC_FS_HANDLE_MODE_WRITE)
      flags|=O_WRONLY;
  }

  /* open real file */
  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_ReserveBytes(pbuf, 128);
  rv=LC_FSFileModule__GetNodePath(fs, node, pbuf);
  if (rv) {
    GWEN_Buffer_free(pbuf);
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

  DBG_INFO(LC_LOGDOMAIN, "Opening real file \"%s\"",
           GWEN_Buffer_GetStart(pbuf));

  fd=open(GWEN_Buffer_GetStart(pbuf), flags);
  if (fd==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "open(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }

  if (((off_t)-1)==lseek(fd, offset, SEEK_SET)) {
    DBG_ERROR(LC_LOGDOMAIN, "lseek(%s, "GWEN_TYPE_TMPL_UINT32"): %s",
              GWEN_Buffer_GetStart(pbuf),
              offset,
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    close(fd);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }

  /* actually read */
  len=GWEN_Buffer_GetUsedBytes(pbuf);
  p=GWEN_Buffer_GetStart(pbuf);
  while(len) {
    int size;
    int i;

    size=len;
    i=write(fd, p, size);
    if (i<0) {
      DBG_ERROR(LC_LOGDOMAIN, "write(%s): %s",
                GWEN_Buffer_GetStart(pbuf),
                strerror(errno));
      GWEN_Buffer_free(pbuf);
      close(fd);
      /* TODO: check errno, return matching error code */
      return LC_FS_ErrorGeneric;
    }
    else if (i==0) {
      DBG_ERROR(LC_LOGDOMAIN, "write(%s): Broken pipe",
                GWEN_Buffer_GetStart(pbuf));
      GWEN_Buffer_free(pbuf);
      close(fd);
      return LC_FS_ErrorBrokenPipe;
    }
    p+=i;
    len-=i;
  } /* while */

  rv=close(fd);

  /* update size */
  if (!stat(GWEN_Buffer_GetStart(pbuf), &st)) {
    LC_FSNode_SetFileSize(node, st.st_size);
  }

  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "close(%s): %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    close(fd);
    /* TODO: check errno, return matching error code */
    return LC_FS_ErrorGeneric;
  }

  GWEN_Buffer_free(pbuf);
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_Lookup(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           const char *name,
                           LC_FS_NODE **nPtr){
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  DBG_INFO(LC_LOGDOMAIN, "Searching for entry \"%s\"", name);

  n=LC_FSFileModule__FindNode(fs, node, name);
  if (!n) {
    DBG_INFO(0, "here");
    return LC_FS_ErrorNotFound;
  }
  *nPtr=n;
  return LC_FS_ErrorNone;
}



int LC_FSFileModule_Dump(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         FILE *f,
                         int indent) {
  LC_FSFILE_MODULE *modm;
  LC_FS_NODE *n;
  LC_FS_NODE *realn;

  assert(fs);
  modm=GWEN_INHERIT_GETDATA(LC_FS_MODULE, LC_FSFILE_MODULE, fs);
  assert(modm);

  LC_FSFileNode_Dump(node, f, indent);
  realn=LC_FSNode_GetMounted(node);
  if (realn)
    LC_FSModule_Dump(LC_FSNode_GetFileSystem(realn), realn, f, indent+2);
  else {
    n=LC_FSNode_List_First(LC_FSFileNode_GetChildren(node));
    while(n) {
      LC_FSFileModule_Dump(fs, n, f, indent+2);
      n=LC_FSNode_List_Next(n);
    } /* while */
  }

  return LC_FS_ErrorNone;
}
















