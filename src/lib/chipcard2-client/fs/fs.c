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
                        LC_FS_NODE_MODE_FTYPE_DIR |
                        LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC |
                        LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE |
                        LC_FS_NODE_MODE_RIGHTS_OWNER_READ);

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
      if ((fmode & LC_FS_NODE_MODE_FTYPE_MASK) !=
          LC_FS_NODE_MODE_FTYPE_FILE) {
        DBG_INFO(LC_LOGDOMAIN, "%s not a regular file", s);
        return 0;
      }
    }
    else {
      if ((fmode & LC_FS_NODE_MODE_FTYPE_MASK) !=
          LC_FS_NODE_MODE_FTYPE_DIR) {
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
    if (path[1]==0)
      /* root wanted, finished */
      return LC_FS_ErrorNone;
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
  if (strcasecmp(p, "/")!=0) {
    rv=LC_FSModule_OpenDir(LC_FSNode_GetFileSystem(node),
                           node,
			   p,
			   &node);
    if (rv) {
      DBG_INFO(0, "here");
      LC_FSPathCtx_free(ctx);
      return rv;
    }
  }

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





