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


#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/path.h>

#include <chipcard2-client/fs/fsmodule.h>


void *LC_FS__HandlePathElement(const char *entry,
                               void *data,
                               unsigned int flags){
  char *p=0;
  int exists;
  LC_FS_PATH_CTX *ctx;
  LC_FS_NODE *node=0;
  int rv;

  ctx=(LC_FS_PATH_CTX*)data;

  if (strcmp(entry, ".")==0) {
    DBG_DEBUG(0, "Entry pointing to itself");
    return data;
  }
  else if (strcmp(entry, "..")==0) {
    char *p;

    p=strrchr(GWEN_Buffer_GetStart(ctx->path), '/');
    if (p)
      if (strcmp(GWEN_Buffer_GetStart(ctx->path), "/")==0)
        p=0;
    if (!p) {
      DBG_INFO(0, "Already at root of file system");
      return 0;
    }
    *p=0;
    GWEN_Buffer_Crop(ctx->path, 0, p-GWEN_Buffer_GetStart(ctx->path));
  }
  else {
    if (GWEN_Buffer_GetUsedBytes(ctx->path)) {
      GWEN_Buffer_AppendByte(ctx->path, '/');
    }
    GWEN_Buffer_AppendString(ctx->path, entry);
  }

  /* check for existence of the file/folder */
  assert(ctx->node);
  p=GWEN_Buffer_GetStart(ctx->path);
  if (*p=='/')
    p++;
  DBG_DEBUG(GWEN_LOGDOMAIN, "Checking entry \"%s\"", p);
  rv=LC_FSModule_Lookup(LC_FSNode_GetFileSystem(ctx->node),
                        ctx->node,
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
    ctx->node=node;
    /* check if something is mounted there, select the mounted node in this
     * case. */
    node=LC_FSNode_GetMounted(node);
    if (node)
      ctx->node=node;
  } /* if entry exists */
  else {
    DBG_INFO(0, "Entry \"%s\" does not exist", p);
    return 0;
  }

  return data;
}

















