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
  fs->rootFsNode=LC_FSMemNode_new(fs->rootFsModule);

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
  int exists;
  LC_FS_PATH_CTX *ctx;
  LC_FS_NODE *node=0;
  int rv;
  GWEN_BUFFER *pbuf;
  LC_FS_NODE *pnode;

  ctx=(LC_FS_PATH_CTX*)data;
  pbuf=LC_FSPathCtx_GetPathBuffer(ctx);
  assert(pbuf);
  pnode=LC_FSPathCtx_GetNode(ctx);
  assert(pnode);

  if (strcmp(entry, ".")==0) {
    DBG_DEBUG(0, "Entry pointing to itself");
    return data;
  }
  else if (strcmp(entry, "..")==0) {
    char *p;

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
      GWEN_Buffer_AppendByte(pbuf, '/');
    }
    GWEN_Buffer_AppendString(pbuf, entry);
  }

  /* check for existence of the file/folder */
  p=GWEN_Buffer_GetStart(pbuf);
  if (*p=='/')
    p++;
  DBG_DEBUG(GWEN_LOGDOMAIN, "Checking entry \"%s\"", p);
  rv=LC_FSModule_Lookup(LC_FSNode_GetFileSystem(pnode),
                        pnode,
                        p, &node);
  if (rv==LC_FS_ErrorNotFound)
    exists=0;
  else if (rv==LC_FS_ErrorNone) {
    exists=1;
  }
  else {
    DBG_INFO(0, "Entry \"%s\" not available (%d)", entry, rv);
    return 0;
  }

  if (exists) {
    GWEN_TYPE_UINT32 fmode;

    fmode=LC_FSNode_GetFileMode(node);
    DBG_DEBUG(GWEN_LOGDOMAIN, "Checking for type");
    if (flags & GWEN_PATH_FLAGS_VARIABLE) {
      if ((fmode & LC_FS_NODE_MODE_FTYPE_MASK) !=
          LC_FS_NODE_MODE_FTYPE_FILE) {
        DBG_INFO(GWEN_LOGDOMAIN, "%s not a regular file", p);
        return 0;
      }
    }
    else {
      if ((fmode & LC_FS_NODE_MODE_FTYPE_MASK) !=
          LC_FS_NODE_MODE_FTYPE_DIR) {
        DBG_INFO(GWEN_LOGDOMAIN, "%s not a direcory", p);
        return 0;
      }
    }
    if ((flags & GWEN_PATH_FLAGS_PATHMUSTNOTEXIST) ||
        ((flags & GWEN_PATH_FLAGS_LAST) &&
         (flags & GWEN_PATH_FLAGS_NAMEMUSTNOTEXIST))) {
      DBG_INFO(GWEN_LOGDOMAIN, "Path \"%s\" does exists (it should not)", p);
      return 0;
    }
    DBG_DEBUG(0, "Entry \"%s\" exists", p);
    LC_FSPathCtx_SetNode(ctx, node);
    /* check if something is mounted there, select the mounted node in this
     * case. */
    node=LC_FSNode_GetMounted(node);
    if (node)
      LC_FSPathCtx_SetNode(ctx, node);
  } /* if entry exists */
  else {
    DBG_INFO(0, "Entry \"%s\" does not exist", p);
    return 0;
  }

  return data;
}




int LC_FS__GetNode(LC_FS *fs,
                   LC_FS_PATH_CTX *ctx,
		   const char *path,
		   GWEN_TYPE_UINT32 flags) {
  assert(path);

  if (*path=='/') {
    /* root, set context to root */
    LC_FSPathCtx_SetPath(ctx, path);
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
                           LC_FS_CLIENT *fcl,
                           const char *path) {
  LC_FS_PATH_CTX *ctx;
  int rv;

  assert(fs);
  assert(fcl);
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











