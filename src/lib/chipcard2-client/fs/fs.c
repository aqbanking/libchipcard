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

#include "fs_p.h"
#include "fsmodule_l.h"
#include "fsclient_l.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/path.h>

#include <chipcard2-client/fs/fsmodule.h>
#include <chipcard2-client/fs/fsmem.h>


static GWEN_TYPE_UINT32 lc_fs__lastClientId=0;




LC_FS_PATH_CTX *LC_FSPathCtx_new(const char *path, LC_FS_NODE *node){
  LC_FS_PATH_CTX *ctx;

  assert(path);
  assert(node);
  GWEN_NEW_OBJECT(LC_FS_PATH_CTX, ctx);
  ctx->path=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(ctx->path, path);
  ctx->node=node;
  LC_FSNode_Attach(ctx->node);

  return ctx;
}



void LC_FSPathCtx_free(LC_FS_PATH_CTX *ctx){
  if (ctx) {
    LC_FSNode_free(ctx->node);
    GWEN_Buffer_free(ctx->path);
    GWEN_FREE_OBJECT(ctx);
  }
}



LC_FS_PATH_CTX *LC_FSPathCtx_dup(const LC_FS_PATH_CTX *octx){
  LC_FS_PATH_CTX *ctx;

  assert(octx);
  ctx=LC_FSPathCtx_new(GWEN_Buffer_GetStart(octx->path),
                       octx->node);
  return ctx;
}



GWEN_BUFFER *LC_FSPathCtx_GetPathBuffer(const LC_FS_PATH_CTX *ctx){
  assert(ctx);
  return ctx->path;
}



void LC_FSPathCtx_SetPath(LC_FS_PATH_CTX *ctx, const char *path){
  assert(ctx);
  assert(path);
  GWEN_Buffer_Reset(ctx->path);
  GWEN_Buffer_AppendString(ctx->path, path);
}



LC_FS_NODE *LC_FSPathCtx_GetNode(const LC_FS_PATH_CTX *ctx){
  assert(ctx);
  return ctx->node;
}



void LC_FSPathCtx_SetNode(LC_FS_PATH_CTX *ctx, LC_FS_NODE *node){
  assert(ctx);
  if (node)
    LC_FSNode_Attach(node);
  LC_FSNode_free(ctx->node);
  ctx->node=node;
}





LC_FS *LC_FS_new() {
  LC_FS *fs;

  GWEN_NEW_OBJECT(LC_FS, fs);
  fs->clients=LC_FSClient_List_new();

  fs->rootFsModule=LC_FSMemModule_new();
  fs->rootFsNode=LC_FSMemNode_new(fs->rootFsModule, "");
  LC_FSNode_SetFileMode(fs->rootFsNode,
                        LC_FS_MODE_FTYPE_DIR |
                        LC_FS_MODE_RIGHTS_OWNER_EXEC |
                        LC_FS_MODE_RIGHTS_OWNER_WRITE |
                        LC_FS_MODE_RIGHTS_OWNER_READ);

  return fs;
}



void LC_FS_free(LC_FS *fs) {
  if (fs) {
    LC_FSClient_List_free(fs->clients);
    LC_FSNode_free(fs->rootFsNode);
    LC_FSModule_free(fs->rootFsModule);
    GWEN_FREE_OBJECT(fs);
  }
}



void *LC_FS__HandlePathElement(const char *entry,
                               void *data,
                               unsigned int flags){
  char *p=0;
  const char *s;
  int exists;
  LC_FS_PATH_CTX *ctx;
  LC_FS_NODE *node=0;
  int rv;
  GWEN_BUFFER *pbuf;
  LC_FS_NODE *pnode;
  LC_FS_NODE *rnode;

  ctx=(LC_FS_PATH_CTX*)data;
  pbuf=LC_FSPathCtx_GetPathBuffer(ctx);
  assert(pbuf);
  pnode=LC_FSPathCtx_GetNode(ctx);
  assert(pnode);

  DBG_INFO(LC_LOGDOMAIN,
           "Handling path element \"%s\" (Path so far: \"%s\")",
           entry, GWEN_Buffer_GetStart(pbuf));

  s=entry;
  if (*s=='/')
    s++;

  if (strcmp(s, ".")==0) {
    DBG_INFO(0, "Entry pointing to itself");
    return data;
  }
  else if (strcmp(s, "..")==0) {
    char *p;

    DBG_INFO(0, "Entry pointing to parent");
    p=strrchr(GWEN_Buffer_GetStart(pbuf), '/');
    if (p)
      if (strcmp(GWEN_Buffer_GetStart(pbuf), "/")==0)
        p=0;
    if (!p) {
      DBG_INFO(0, "Already at root of file system");
      return 0;
    }
    *p=0;
    GWEN_Buffer_Crop(pbuf, 0, p-GWEN_Buffer_GetStart(pbuf));
  }
  else {
    if (GWEN_Buffer_GetUsedBytes(pbuf)) {
      char *p;

      p=GWEN_Buffer_GetStart(pbuf);
      if (p[GWEN_Buffer_GetUsedBytes(pbuf)-1]!='/')
        GWEN_Buffer_AppendByte(pbuf, '/');
    }
    GWEN_Buffer_AppendString(pbuf, s);
  }

  /* check for existence of the file/folder */
  rnode=LC_FSNode_GetMounted(pnode);
  if (!rnode)
    rnode=pnode;
  else {
    DBG_INFO(LC_LOGDOMAIN, "There something mounted at \"%s\"", s);
  }
  DBG_INFO(LC_LOGDOMAIN, "Checking entry \"%s\"", s);
  rv=LC_FSModule_Lookup(LC_FSNode_GetFileSystem(rnode),
                        rnode,
                        s, &node);
  if (rv==LC_FS_ErrorNotFound)
    exists=0;
  else if (rv==LC_FS_ErrorNone) {
    exists=1;
  }
  else {
    DBG_INFO(0, "Entry \"%s\" not available (%d)", entry, rv);
    return 0;
  }

  DBG_INFO(LC_LOGDOMAIN, "Result for entry \"%s\": %d (%s)",
           entry, rv, exists?"exists":"missing");

  if (exists) {
    GWEN_TYPE_UINT32 fmode;

    fmode=LC_FSNode_GetFileMode(node);
    DBG_INFO(LC_LOGDOMAIN, "Checking for type");
    if (flags & GWEN_PATH_FLAGS_VARIABLE) {
      if ((fmode & LC_FS_MODE_FTYPE_MASK) !=
          LC_FS_MODE_FTYPE_FILE) {
        DBG_INFO(LC_LOGDOMAIN, "%s not a regular file", s);
        return 0;
      }
    }
    else {
      if ((fmode & LC_FS_MODE_FTYPE_MASK) !=
          LC_FS_MODE_FTYPE_DIR) {
        DBG_INFO(LC_LOGDOMAIN, "%s not a direcory", s);
        return 0;
      }
    }
    if ((flags & GWEN_PATH_FLAGS_PATHMUSTNOTEXIST) ||
        ((flags & GWEN_PATH_FLAGS_LAST) &&
         (flags & GWEN_PATH_FLAGS_NAMEMUSTNOTEXIST))) {
      DBG_INFO(LC_LOGDOMAIN, "Path \"%s\" does exists (it should not)", p);
      return 0;
    }
    DBG_INFO(0, "Entry \"%s\" exists", s);
    LC_FSPathCtx_SetNode(ctx, node);
    /* check if something is mounted there, select the mounted node in this
     * case. */
    node=LC_FSNode_GetMounted(node);
    if (node)
      LC_FSPathCtx_SetNode(ctx, node);
  } /* if entry exists */
  else {
    DBG_INFO(0, "Entry \"%s\" does not exist", s);
    return 0;
  }

  return data;
}




int LC_FS__GetNode(LC_FS *fs,
                   LC_FS_PATH_CTX *ctx,
		   const char *path,
		   GWEN_TYPE_UINT32 flags) {
  assert(path);

  DBG_INFO(LC_LOGDOMAIN, "Searching for path \"%s\"", path);

  if (*path=='/') {
    /* root, set context to root */
    LC_FSPathCtx_SetPath(ctx, "/");
    LC_FSPathCtx_SetNode(ctx, fs->rootFsNode);
    if (path[1]==0) {
      /* root wanted, finished */
      DBG_ERROR(LC_LOGDOMAIN, "Root wanted");
      return LC_FS_ErrorNone;
    }
  }

  if (0==GWEN_Path_Handle(path, (void*)ctx,
			  flags,
			  LC_FS__HandlePathElement)) {
    DBG_INFO(0, "Path \"%s\" not found", path);
    return LC_FS_ErrorNotFound;
  }

  return LC_FS_ErrorNone;
}



LC_FS_CLIENT *LC_FS__FindClient(LC_FS *fs, GWEN_TYPE_UINT32 id){
  LC_FS_CLIENT *fcl;

  assert(fs);
  fcl=LC_FSClient_List_First(fs->clients);
  while(fcl) {
    if (LC_FSClient_GetId(fcl)==id)
      break;
    fcl=LC_FSClient_List_Next(fcl);
  } /* while */

  return fcl;
}



GWEN_TYPE_UINT32 LC_FS_CreateClient(LC_FS *fs){
  LC_FS_CLIENT *fcl;
  LC_FS_PATH_CTX *ctx;

  fcl=LC_FSClient_new(fs, ++lc_fs__lastClientId);
  ctx=LC_FSPathCtx_new("/", fs->rootFsNode);
  LC_FSClient_SetWorkingCtx(fcl, ctx);
  LC_FSClient_List_Add(fcl, fs->clients);
  return LC_FSClient_GetId(fcl);
}



void LC_FS_DestroyClient(LC_FS *fs, GWEN_TYPE_UINT32 clid){
  LC_FS_CLIENT *fcl;

  assert(fs);
  fcl=LC_FS__FindClient(fs, clid);
  if (fcl) {
    LC_FSClient_List_Del(fcl);
    LC_FSClient_free(fcl);
  }
}



int LC_FS_ChangeWorkingDir(LC_FS *fs,
                           GWEN_TYPE_UINT32 clid,
                           const char *path) {
  LC_FS_PATH_CTX *ctx;
  LC_FS_CLIENT *fcl;
  int rv;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }
  ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));

  rv=LC_FS__GetNode(fs, ctx, path,
                    GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                    GWEN_PATH_FLAGS_CHECKROOT);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
  }

  LC_FSClient_SetWorkingCtx(fcl, ctx);
  return 0;
}



int LC_FS_OpenDir(LC_FS *fs,
		  GWEN_TYPE_UINT32 clid,
		  const char *path,
		  GWEN_TYPE_UINT32 *pHid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  LC_FS_NODE *realNode;
  LC_FS_NODE_HANDLE *hdl;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));

    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
		      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
  }

  /* open folder (if not root) */
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);
  realNode=LC_FSNode_GetMounted(node);
  if (realNode)
    node=realNode;

  if (strcasecmp(path, "/")==0)
    rv=LC_FSModule_OpenDir(LC_FSNode_GetFileSystem(node),
                           node,
                           0,
                           &node);
  else
    rv=LC_FSModule_OpenDir(LC_FSNode_GetFileSystem(node),
                           node,
                           p,
                           &node);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
  }

  realNode=LC_FSNode_GetMounted(node);
  if (realNode)
    node=realNode;

  /* create file handle */
  hdl=LC_FSNodeHandle_new(path, node, LC_FSClient_GetNextHandleId(fcl));
  LC_FSClient_AddNodeHandle(fcl, hdl);
  *pHid=LC_FSNodeHandle_GetId(hdl);
  LC_FSPathCtx_free(ctx);
  return 0;
}



int LC_FS_MkDir(LC_FS *fs,
		GWEN_TYPE_UINT32 clid,
		const char *path,
		GWEN_TYPE_UINT32 mode,
		GWEN_TYPE_UINT32 *pHid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  LC_FS_NODE_HANDLE *hdl;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  if (strcasecmp(path, "/")==0) {
    DBG_ERROR(0, "Unable to create root node");
    return LC_FS_ErrorExists;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in \"%s\"",
             p, folder);
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    DBG_INFO(LC_LOGDOMAIN,
             "Current working dir is: %s\n",
             GWEN_Buffer_GetStart(LC_FSPathCtx_GetPathBuffer(ctx)));
    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
		      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in current working dir", p);
  }

  /* open folder (if not root) */
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);
  DBG_INFO(LC_LOGDOMAIN, "Creating folder \"%s\"", p);
  rv=LC_FSModule_MkDir(LC_FSNode_GetFileSystem(node),
                       node,
                       p,
                       mode & LC_FS_MODE_MASK_NODE,
                       &node);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
    }

  /* create file handle */
  DBG_INFO(LC_LOGDOMAIN, "Creating handle");
  hdl=LC_FSNodeHandle_new(path, node, LC_FSClient_GetNextHandleId(fcl));
  LC_FSNodeHandle_SetFlags(hdl, mode & LC_FS_MODE_MASK_HANDLE);
  LC_FSClient_AddNodeHandle(fcl, hdl);
  *pHid=LC_FSNodeHandle_GetId(hdl);
  LC_FSPathCtx_free(ctx);
  DBG_INFO(LC_LOGDOMAIN, "Created handle %08x", *pHid);
  return 0;
}



int LC_FS_ReadDir(LC_FS *fs,
                  GWEN_TYPE_UINT32 clid,
                  GWEN_TYPE_UINT32 hid,
                  GWEN_STRINGLIST2 *sl) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE_HANDLE *hdl;
  LC_FS_NODE *node;
  int rv;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  hdl=LC_FSClient_FindHandle(fcl, hid);
  if (!hdl) {
    DBG_ERROR(0, "Handle %08x not found", hid);
    return LC_FS_ErrorInvalid;
  }

  node=LC_FSNodeHandle_GetNode(hdl);
  assert(node);
  if ((LC_FSNode_GetFileMode(node) & LC_FS_MODE_FTYPE_MASK) !=
      LC_FS_MODE_FTYPE_DIR) {
    DBG_ERROR(LC_LOGDOMAIN, "Not a folder");
    return LC_FS_ErrorNotDir;
  }
  rv=LC_FSModule_ReadDir(LC_FSNode_GetFileSystem(node), node, sl);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "Error: %d", rv);
  }
  return rv;
}



int LC_FS_CloseDir(LC_FS *fs,
		   GWEN_TYPE_UINT32 clid,
		   GWEN_TYPE_UINT32 hid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE_HANDLE *hdl;
  LC_FS_NODE *node;
  int rv;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  hdl=LC_FSClient_FindHandle(fcl, hid);
  if (!hdl) {
    DBG_ERROR(0, "Handle %08x not found", hid);
    return LC_FS_ErrorInvalid;
  }

  node=LC_FSNodeHandle_GetNode(hdl);
  assert(node);
  if ((LC_FSNode_GetFileMode(node) & LC_FS_MODE_FTYPE_MASK) !=
      LC_FS_MODE_FTYPE_DIR) {
    DBG_ERROR(LC_LOGDOMAIN, "Not a folder");
    return LC_FS_ErrorNotDir;
  }
  rv=LC_FSModule_CloseDir(LC_FSNode_GetFileSystem(node), node);
  LC_FSNodeHandle_List_Del(hdl);
  LC_FSNodeHandle_free(hdl);

  return rv;
}



int LC_FS_CreateFile(LC_FS *fs,
                     GWEN_TYPE_UINT32 clid,
                     const char *path,
                     GWEN_TYPE_UINT32 mode,
                     GWEN_TYPE_UINT32 *pHid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  LC_FS_NODE_HANDLE *hdl;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  if (strcasecmp(path, "/")==0) {
    DBG_ERROR(0, "Unable to create root node");
    return LC_FS_ErrorExists;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in \"%s\"",
             p, folder);
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    DBG_INFO(LC_LOGDOMAIN,
             "Current working dir is: %s\n",
             GWEN_Buffer_GetStart(LC_FSPathCtx_GetPathBuffer(ctx)));
    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in current working dir", p);
  }

  /* open folder (if not root) */
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);
  DBG_INFO(LC_LOGDOMAIN, "Creating file \"%s\" here:", p);
  LC_FSModule_Dump(LC_FSNode_GetFileSystem(node), node, stderr, 2);
  rv=LC_FSModule_CreateFile(LC_FSNode_GetFileSystem(node),
                            node,
                            p,
                            mode & LC_FS_MODE_MASK_NODE,
                            &node);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
    }

  /* create file handle */
  DBG_INFO(LC_LOGDOMAIN, "Creating handle");
  hdl=LC_FSNodeHandle_new(path, node, LC_FSClient_GetNextHandleId(fcl));
  LC_FSNodeHandle_SetFlags(hdl, mode & LC_FS_MODE_MASK_HANDLE);
  LC_FSClient_AddNodeHandle(fcl, hdl);
  *pHid=LC_FSNodeHandle_GetId(hdl);
  LC_FSPathCtx_free(ctx);
  DBG_INFO(LC_LOGDOMAIN, "Created handle %08x", *pHid);
  return 0;
}



int LC_FS_OpenFile(LC_FS *fs,
                   GWEN_TYPE_UINT32 clid,
                   const char *path,
                   GWEN_TYPE_UINT32 mode,
                   GWEN_TYPE_UINT32 *pHid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  LC_FS_NODE_HANDLE *hdl;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  if (strcasecmp(path, "/")==0) {
    DBG_ERROR(0, "Unable to open root node");
    return LC_FS_ErrorExists;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in \"%s\"",
             p, folder);
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    DBG_INFO(LC_LOGDOMAIN,
             "Current working dir is: %s\n",
             GWEN_Buffer_GetStart(LC_FSPathCtx_GetPathBuffer(ctx)));
    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
    DBG_INFO(LC_LOGDOMAIN, "Searching for \"%s\" in current working dir", p);
  }

  /* open folder (if not root) */
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);
  DBG_INFO(LC_LOGDOMAIN, "Opening file \"%s\"", p);
  rv=LC_FSModule_OpenFile(LC_FSNode_GetFileSystem(node),
                          node,
                          p,
                          &node);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
    }

  /* create file handle */
  DBG_INFO(LC_LOGDOMAIN, "Creating handle");
  hdl=LC_FSNodeHandle_new(path, node, LC_FSClient_GetNextHandleId(fcl));
  LC_FSNodeHandle_SetFlags(hdl, mode & LC_FS_MODE_MASK_HANDLE);
  LC_FSClient_AddNodeHandle(fcl, hdl);
  *pHid=LC_FSNodeHandle_GetId(hdl);
  LC_FSPathCtx_free(ctx);
  DBG_INFO(LC_LOGDOMAIN, "Created handle %08x", *pHid);
  return 0;
}



int LC_FS_CloseFile(LC_FS *fs,
                    GWEN_TYPE_UINT32 clid,
                    GWEN_TYPE_UINT32 hid) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE_HANDLE *hdl;
  LC_FS_NODE *node;
  int rv;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  hdl=LC_FSClient_FindHandle(fcl, hid);
  if (!hdl) {
    DBG_ERROR(0, "Handle %08x not found", hid);
    return LC_FS_ErrorInvalid;
  }

  node=LC_FSNodeHandle_GetNode(hdl);
  assert(node);
  if ((LC_FSNode_GetFileMode(node) & LC_FS_MODE_FTYPE_MASK) !=
      LC_FS_MODE_FTYPE_FILE) {
    DBG_ERROR(LC_LOGDOMAIN, "Not a regular file");
    return LC_FS_ErrorNotFile;
  }

  rv=LC_FSModule_CloseFile(LC_FSNode_GetFileSystem(node), node);
  LC_FSNodeHandle_List_Del(hdl);
  LC_FSNodeHandle_free(hdl);

  return rv;
}



int LC_FS_ReadFile(LC_FS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid,
                   GWEN_TYPE_UINT32 offset,
                   GWEN_TYPE_UINT32 len,
                   GWEN_BUFFER *buf){
  LC_FS_CLIENT *fcl;
  LC_FS_NODE_HANDLE *hdl;
  LC_FS_NODE *node;
  int rv;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  hdl=LC_FSClient_FindHandle(fcl, hid);
  if (!hdl) {
    DBG_ERROR(0, "Handle %08x not found", hid);
    return LC_FS_ErrorInvalid;
  }

  node=LC_FSNodeHandle_GetNode(hdl);
  rv=LC_FSModule_ReadFile(LC_FSNode_GetFileSystem(node), node,
                          LC_FSNodeHandle_GetFlags(hdl),
                          offset,
                          len,
                          buf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }
  return rv;
}



int LC_FS_Unlink(LC_FS *fs,
                 GWEN_TYPE_UINT32 clid,
                 const char *path) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));

    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
		      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
  }

  /* unlink (if not root) */
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);
  if (strcasecmp(p, "/")!=0) {
    /* TODO: Check for file or empty folder */
    rv=LC_FSModule_Unlink(LC_FSNode_GetFileSystem(node),
                          node,
                          p);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      return rv;
    }
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Can not unlink root");
    return LC_FS_ErrorInvalid;
  }

  LC_FSPathCtx_free(ctx);
  return 0;
}



int LC_FS_Stat(LC_FS *fs,
               GWEN_TYPE_UINT32 clid,
               const char *path,
               LC_FS_STAT **pStat) {
  LC_FS_CLIENT *fcl;
  LC_FS_NODE *node;
  LC_FS_NODE *realNode;
  int rv;
  const char *p;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }

  /* get context of folder which contains the wanted dir */
  p=strrchr(path, '/');
  if (p) {
    char *folder;

    if (p==path)
      folder=strdup("/");
    else {
      folder=(char*)malloc(p-path+1);
      assert(folder);
      memmove(folder, path, p-path);
      folder[p-path]=0;
    }
    p++;
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));

    rv=LC_FS__GetNode(fs, ctx, folder,
		      GWEN_PATH_FLAGS_NAMEMUSTEXIST |
		      GWEN_PATH_FLAGS_CHECKROOT);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      free(folder);
      return rv;
    }
    free(folder);
  }
  else {
    ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));
    p=path;
  }

  /* return node */
  node=LC_FSPathCtx_GetNode(ctx);
  realNode=LC_FSNode_GetMounted(node);
  if (realNode)
    node=realNode;
  rv=LC_FSModule_Lookup(LC_FSNode_GetFileSystem(node), node,
                        p, &node);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error lookin up node \"%s\": %d",
              p, rv);
    LC_FSPathCtx_free(ctx);
    return rv;
  }
  assert(node);
  realNode=LC_FSNode_GetMounted(node);
  if (realNode)
    node=realNode;
  *pStat=LC_FSStat_fromNode(node);
  assert(*pStat);
  LC_FSPathCtx_free(ctx);
  return 0;
}














int LC_FS_Dump(LC_FS *fs,
               GWEN_TYPE_UINT32 clid,
               const char *path,
               FILE *f,
               int indent) {
  LC_FS_PATH_CTX *ctx;
  LC_FS_CLIENT *fcl;
  int rv;
  LC_FS_NODE *node;

  assert(fs);
  assert(clid);

  fcl=LC_FS__FindClient(fs, clid);
  if (!fcl) {
    DBG_ERROR(0, "Client %08x not found", clid);
    return LC_FS_ErrorInvalid;
  }
  ctx=LC_FSPathCtx_dup(LC_FSClient_GetWorkingCtx(fcl));

  rv=LC_FS__GetNode(fs, ctx, path,
                    GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                    GWEN_PATH_FLAGS_CHECKROOT);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
  }

  node=LC_FSPathCtx_GetNode(ctx);
  rv=LC_FSModule_Dump(LC_FSNode_GetFileSystem(node), node, f, indent);
  LC_FSPathCtx_free(ctx);
  return rv;
}



int LC_FS_Mount(LC_FS *fs,
                LC_FS_MODULE *fsm,
                const char *path) {
  LC_FS_NODE *node;
  LC_FS_NODE *modNode=0;
  int rv;
  LC_FS_PATH_CTX *ctx;

  assert(fs);
  assert(fsm);
  assert(path);

  if (strcasecmp(path, "/")==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Can not mount at root");
    return LC_FS_ErrorInvalid;
  }

  if (*path!='/') {
    DBG_ERROR(LC_LOGDOMAIN, "Absolute path needed");
    return LC_FS_ErrorInvalid;
  }

  /* get context of folder which contains the wanted dir */
  ctx=LC_FSPathCtx_new("/", fs->rootFsNode);

  /* get folder node */
  rv=LC_FS__GetNode(fs, ctx, path,
                    GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                    GWEN_PATH_FLAGS_CHECKROOT);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
  }
  node=LC_FSPathCtx_GetNode(ctx);
  assert(node);

  if (LC_FSNode_GetMounted(node)) {
    DBG_ERROR(LC_LOGDOMAIN,
              "There already is something mounted here");
    LC_FSPathCtx_free(ctx);
    return LC_FS_ErrorInvalid;
  }

  rv=LC_FSModule_Mount(fsm, &modNode);
  if (rv) {
    DBG_INFO(0, "here");
    LC_FSPathCtx_free(ctx);
    return rv;
  }

  LC_FSNode_SetMounted(node, modNode);
  DBG_NOTICE(LC_LOGDOMAIN, "Mounted fs at \"%s\"", path);

  LC_FSPathCtx_free(ctx);
  return 0;
}







int LC_FS_HandleCreateClient(LC_FS *fs,
                             GWEN_DB_NODE *dbRequest,
                             GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;

  cid=LC_FS_CreateClient(fs);
  GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "resultCode", 0);
  GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "resultText", "Client created");
  GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", cid);
  return 0;
}



int LC_FS_HandleDestroyClient(LC_FS *fs,
                              GWEN_DB_NODE *dbRequest,
                              GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  LC_FS_DestroyClient(fs, cid);
  GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "resultCode", 0);
  GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "resultText", "Client destroyed");
  return 0;
}



int LC_FS_HandleChangeWorkingDir(LC_FS *fs,
                                 GWEN_DB_NODE *dbRequest,
                                 GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_ChangeWorkingDir(fs, cid, path);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Working directory changed");
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleOpenDir(LC_FS *fs,
                        GWEN_DB_NODE *dbRequest,
                        GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_OpenDir(fs, cid, path, &hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Folder opened");
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "hid", hid);
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleMkDir(LC_FS *fs,
                      GWEN_DB_NODE *dbRequest,
                      GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 mode;
  GWEN_TYPE_UINT32 hid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  mode=GWEN_DB_GetIntValue(dbRequest, "mode", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_MkDir(fs, cid, path, mode, &hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Folder created");
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "hid", hid);
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleReadDir(LC_FS *fs,
                        GWEN_DB_NODE *dbRequest,
                        GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  int rv;
  GWEN_STRINGLIST2 *sl;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  hid=GWEN_DB_GetIntValue(dbRequest, "hid", 0, 0);
  if (hid==0) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  sl=GWEN_StringList2_new();
  rv=LC_FS_ReadDir(fs, cid, hid, sl);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    GWEN_StringList2_free(sl);
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_STRINGLIST2_ITERATOR *sit;

    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Folder read");
    GWEN_DB_DeleteVar(dbResponse, "entries");
    sit=GWEN_StringList2_First(sl);
    if (sit) {
      const char *s;

      s=GWEN_StringList2Iterator_Data(sit);
      while(s) {
        GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_DEFAULT,
                             "entries", s);
        s=GWEN_StringList2Iterator_Next(sit);
      }
      GWEN_StringList2Iterator_free(sit);
    }
  }
  GWEN_StringList2_free(sl);
  return LC_FS_ErrorNone;
}



int LC_FS_HandleCloseDir(LC_FS *fs,
                         GWEN_DB_NODE *dbRequest,
                         GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  int rv;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  hid=GWEN_DB_GetIntValue(dbRequest, "hid", 0, 0);
  if (hid==0) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_CloseDir(fs, cid, hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Folder closed");
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleOpenFile(LC_FS *fs,
                         GWEN_DB_NODE *dbRequest,
                         GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_OpenDir(fs, cid, path, &hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Working directory changed");
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "hid", hid);
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleCreateFile(LC_FS *fs,
                           GWEN_DB_NODE *dbRequest,
                           GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 mode;
  GWEN_TYPE_UINT32 hid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  mode=GWEN_DB_GetIntValue(dbRequest, "mode", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_CreateFile(fs, cid, path, mode, &hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Folder created");
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "hid", hid);
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleCloseFile(LC_FS *fs,
                          GWEN_DB_NODE *dbRequest,
                          GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  int rv;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  hid=GWEN_DB_GetIntValue(dbRequest, "hid", 0, 0);
  if (hid==0) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_CloseFile(fs, cid, hid);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "File closed");
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleReadFile(LC_FS *fs,
                         GWEN_DB_NODE *dbRequest,
                         GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  GWEN_TYPE_UINT32 hid;
  GWEN_TYPE_UINT32 offset;
  GWEN_TYPE_UINT32 len;
  GWEN_BUFFER *buf;
  int rv;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  hid=GWEN_DB_GetIntValue(dbRequest, "hid", 0, 0);
  offset=GWEN_DB_GetIntValue(dbRequest, "offset", 0, 0);
  len=GWEN_DB_GetIntValue(dbRequest, "len", 0, 0);
  if (hid==0 || len==0) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  buf=GWEN_Buffer_new(0, len, 0, 1);
  rv=LC_FS_ReadFile(fs, cid, hid, offset, len, buf);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    GWEN_Buffer_free(buf);
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "File closed");
    if (GWEN_Buffer_GetUsedBytes(buf))
      GWEN_DB_SetBinValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "data",
                          GWEN_Buffer_GetStart(buf),
                          GWEN_Buffer_GetUsedBytes(buf));
  }
  GWEN_Buffer_free(buf);
  return LC_FS_ErrorNone;
}



int LC_FS_HandleUnlink(LC_FS *fs,
                       GWEN_DB_NODE *dbRequest,
                       GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  int rv;
  const char *path;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_Unlink(fs, cid, path);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Object unlinked");
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleStat(LC_FS *fs,
                     GWEN_DB_NODE *dbRequest,
                     GWEN_DB_NODE *dbResponse) {
  GWEN_TYPE_UINT32 cid;
  int rv;
  const char *path;
  LC_FS_STAT *st=0;

  cid=GWEN_DB_GetIntValue(dbRequest, "cid", 0, 0);
  path=GWEN_DB_GetCharValue(dbRequest, "path", 0, 0);
  if (!path) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing arguments");
    return LC_FS_ErrorNone;
  }
  rv=LC_FS_Stat(fs, cid, path, &st);
  if (rv) {
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", rv);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Error returned by function");
    return LC_FS_ErrorNone;
  }
  else {
    GWEN_DB_NODE *dbT;

    assert(st);
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorNone);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Stat performed.");
    dbT=GWEN_DB_GetGroup(dbResponse, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "stat");
    assert(dbT);
    LC_FSStat_toDb(st, dbT);
    LC_FSStat_free(st);
  }
  return LC_FS_ErrorNone;
}



int LC_FS_HandleRequest(LC_FS *fs,
                        GWEN_DB_NODE *dbRequest,
                        GWEN_DB_NODE *dbResponse) {
  const char *cmd;
  int rv;

  cmd=GWEN_DB_GetCharValue(dbRequest, "name", 0, 0);
  if (!cmd) {
    DBG_ERROR(LC_LOGDOMAIN, "No command in request");
    GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "resultCode", LC_FS_ErrorMissingArgs);
    GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "resultText", "Missing command");
    rv=LC_FS_ErrorNone;
  }
  else {
    DBG_NOTICE(LC_LOGDOMAIN, "Command \"%s\"", cmd);
    if (strcasecmp(cmd, "CreateClientRequest")==0)
      rv=LC_FS_HandleCreateClient(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "DestroyClientRequest")==0)
      rv=LC_FS_HandleDestroyClient(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "ChangeWorkingDirRequest")==0)
      rv=LC_FS_HandleChangeWorkingDir(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "OpenDirRequest")==0)
      rv=LC_FS_HandleOpenDir(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "MkDirRequest")==0)
      rv=LC_FS_HandleMkDir(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "ReadDirRequest")==0)
      rv=LC_FS_HandleReadDir(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "CloseDirRequest")==0)
      rv=LC_FS_HandleCloseDir(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "OpenFileRequest")==0)
      rv=LC_FS_HandleOpenFile(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "CreateFileRequest")==0)
      rv=LC_FS_HandleCreateFile(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "CloseFileRequest")==0)
      rv=LC_FS_HandleCloseFile(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "ReadFileRequest")==0)
      rv=LC_FS_HandleReadFile(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "UnlinkRequest")==0)
      rv=LC_FS_HandleUnlink(fs, dbRequest, dbResponse);
    else if (strcasecmp(cmd, "StatRequest")==0)
      rv=LC_FS_HandleStat(fs, dbRequest, dbResponse);
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command \"%s\" not supported", cmd);
      GWEN_DB_SetIntValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "resultCode", LC_FS_ErrorNotSupported);
      GWEN_DB_SetCharValue(dbResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "resultText", "Command not supported");
      rv=LC_FS_ErrorNone;
    }
  }

  return rv;
}







LC_FS_STAT *LC_FSStat_new() {
  LC_FS_STAT *st;

  GWEN_NEW_OBJECT(LC_FS_STAT, st);
  return st;
}



void LC_FSStat_free(LC_FS_STAT *st) {
  if (st) {
    GWEN_FREE_OBJECT(st);
  }
}



LC_FS_STAT *LC_FSStat_dup(const LC_FS_STAT *ost) {
  LC_FS_STAT *st;

  st=LC_FSStat_new();
  st->fileMode=ost->fileMode;
  st->fileSize=ost->fileSize;
  st->ctime=ost->ctime;
  st->mtime=ost->mtime;
  st->atime=ost->atime;

  return st;
}



LC_FS_STAT *LC_FSStat_fromNode(const LC_FS_NODE *n) {
  LC_FS_STAT *st;

  st=LC_FSStat_new();
  st->fileMode=LC_FSNode_GetFileMode(n);
  st->fileSize=LC_FSNode_GetFileSize(n);
  st->ctime=LC_FSNode_GetCTime(n);
  st->mtime=LC_FSNode_GetMTime(n);
  st->atime=LC_FSNode_GetATime(n);

  return st;
}



LC_FS_STAT *LC_FSStat_fromDb(GWEN_DB_NODE *db) {
  LC_FS_STAT *st;

  st=LC_FSStat_new();
  st->fileMode=(GWEN_TYPE_UINT32)GWEN_DB_GetIntValue(db, "fileMode", 0, 0);
  st->fileSize=(GWEN_TYPE_UINT32)GWEN_DB_GetIntValue(db, "fileSize", 0, 0);
  st->ctime=(time_t)GWEN_DB_GetIntValue(db, "ctime", 0, 0);
  st->mtime=(time_t)GWEN_DB_GetIntValue(db, "mtime", 0, 0);
  st->atime=(time_t)GWEN_DB_GetIntValue(db, "atime", 0, 0);

  return st;
}



int LC_FSStat_toDb(const LC_FS_STAT *st, GWEN_DB_NODE *db) {
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "fileMode", st->fileMode);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "fileSize", st->fileSize);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "ctime", st->ctime);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "mtime", st->mtime);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "atime", st->atime);

  return 0;
}




GWEN_TYPE_UINT32 LC_FSStat_GetFileMode(const LC_FS_STAT *st) {
  assert(st);
  return st->fileMode;
}



void LC_FSStat_SetFileMode(LC_FS_STAT *st, GWEN_TYPE_UINT32 m){
}



GWEN_TYPE_UINT32 LC_FSStat_GetFileSize(const LC_FS_STAT *st) {
  assert(st);
  return st->fileSize;
}



void LC_FSStat_SetFileSize(LC_FS_STAT *st, GWEN_TYPE_UINT32 s) {
}



time_t LC_FSStat_GetCTime(const LC_FS_STAT *st) {
  assert(st);
  return st->ctime;
}



void LC_FSStat_SetCTime(LC_FS_STAT *st, time_t ti) {
  assert(st);
  st->ctime=ti;
}



time_t LC_FSStat_GetATime(const LC_FS_STAT *st) {
  assert(st);
  return st->atime;
}



void LC_FSStat_SetATime(LC_FS_STAT *st, time_t ti) {
  assert(st);
  st->atime=ti;
}



time_t LC_FSStat_GetMTime(const LC_FS_STAT *st) {
  assert(st);
  return st->mtime;
}



void LC_FSStat_SetMTime(LC_FS_STAT *st, time_t ti) {
  assert(st);
  st->mtime=ti;
}









