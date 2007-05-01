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

#include "rfs_p.h"
#include "fsmodule_l.h"
#include "fsclient_l.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/path.h>



GWEN_INHERIT_FUNCTIONS(LC_RFS)


LC_RFS *LC_RFS_new() {
  LC_RFS *fs;

  GWEN_NEW_OBJECT(LC_RFS, fs);
  GWEN_INHERIT_INIT(LC_RFS, fs);

  return fs;
}



void LC_RFS_free(LC_RFS *fs){
  if (fs) {
    GWEN_FREE_OBJECT(fs);
  }
}



void LC_RFS_SetExchangeFn(LC_RFS *fs, LC_RFS_EXCHANGE_FN fn) {
  assert(fs);
  fs->exchangeFn=fn;
}



GWEN_TYPE_UINT32 LC_RFS_CreateClient(LC_RFS *fs) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "CreateClientRequest");
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "cid", 0, 0);
    if (i==0) {
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return 0;
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return i;
  }
}



void LC_RFS_DestroyClient(LC_RFS *fs, GWEN_TYPE_UINT32 clid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "DestroyClientRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return;
  }
  GWEN_DB_Group_free(dbResponse);
  GWEN_DB_Group_free(dbRequest);
}



int LC_RFS_ChangeWorkingDir(LC_RFS *fs,
                            GWEN_TYPE_UINT32 clid,
                            const char *path) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "ChangeWorkingDirRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_OpenDir(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   const char *path,
                   GWEN_TYPE_UINT32 *pHid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "OpenDirRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      GWEN_TYPE_UINT32 hid;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      hid=GWEN_DB_GetIntValue(dbResponse, "hid", 0, 0);
      *pHid=hid;
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_MkDir(LC_RFS *fs,
                 GWEN_TYPE_UINT32 clid,
                 const char *path,
                 GWEN_TYPE_UINT32 mode,
                 GWEN_TYPE_UINT32 *pHid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "MkDirRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "mode", mode);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      GWEN_TYPE_UINT32 hid;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      hid=GWEN_DB_GetIntValue(dbResponse, "hid", 0, 0);
      *pHid=hid;
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_ReadDir(LC_RFS *fs,
                   GWEN_TYPE_UINT32 clid,
                   GWEN_TYPE_UINT32 hid,
                   GWEN_STRINGLIST2 *sl) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "ReadDirRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "hid", hid);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      int i;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));

      for (i=0; ; i++) {
        const char *s;

        if (i>5000000) {
          DBG_ERROR(LC_LOGDOMAIN,
                    "Uuups, too many entries (%d), aborting",
                    i);
          abort();
        }
        s=GWEN_DB_GetCharValue(dbResponse, "entries", i, 0);
        if (!s)
          break;
        if (GWEN_StringList2_AppendString(sl, s, 0,
                                          GWEN_StringList2_IntertModeNoDouble)
            ==0) {
          DBG_WARN(LC_LOGDOMAIN, "Double entry suppressed");
        }
      } /* for */

    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }

}



int LC_RFS_CloseDir(LC_RFS *fs,
                    GWEN_TYPE_UINT32 clid,
                    GWEN_TYPE_UINT32 hid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "CloseDirRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "hid", hid);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_OpenFile(LC_RFS *fs,
                    GWEN_TYPE_UINT32 clid,
                    const char *path,
                    GWEN_TYPE_UINT32 mode,
                    GWEN_TYPE_UINT32 *pHid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "OpenFileRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "mode", mode);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      GWEN_TYPE_UINT32 hid;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      hid=GWEN_DB_GetIntValue(dbResponse, "hid", 0, 0);
      *pHid=hid;
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_CreateFile(LC_RFS *fs,
                      GWEN_TYPE_UINT32 clid,
                      const char *path,
                      GWEN_TYPE_UINT32 mode,
                      GWEN_TYPE_UINT32 *pHid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "CreateFileRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "mode", mode);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      GWEN_TYPE_UINT32 hid;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      hid=GWEN_DB_GetIntValue(dbResponse, "hid", 0, 0);
      *pHid=hid;
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_CloseFile(LC_RFS *fs,
                     GWEN_TYPE_UINT32 clid,
                     GWEN_TYPE_UINT32 hid) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "CloseFileRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "hid", hid);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }

}



int LC_RFS_ReadFile(LC_RFS *fs,
                    GWEN_TYPE_UINT32 clid,
                    GWEN_TYPE_UINT32 hid,
                    GWEN_TYPE_UINT32 offset,
                    GWEN_TYPE_UINT32 len,
                    GWEN_BUFFER *buf) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "ReadFileRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "hid", hid);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "offset", offset);
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "len", len);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      const void *p;
      unsigned int l;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      p=GWEN_DB_GetBinValue(dbResponse, "data", 0, 0, 0, &l);
      if (p && l) {
        if (buf)
          GWEN_Buffer_AppendBytes(buf, p, l);
      }
      else {
        DBG_WARN(LC_LOGDOMAIN, "No data returned");
      }
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return LC_FS_ErrorNone;
  }
}



int LC_RFS_Unlink(LC_RFS *fs,
                  GWEN_TYPE_UINT32 clid,
                  const char *path) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "UnlinkRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}



int LC_RFS_Stat(LC_RFS *fs,
                GWEN_TYPE_UINT32 clid,
                const char *path,
                LC_FS_STAT **pStat) {
  GWEN_DB_NODE *dbRequest;
  GWEN_DB_NODE *dbResponse;
  int rv;

  dbRequest=GWEN_DB_Group_new("RFS_Request");
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", "StatRequest");
  GWEN_DB_SetIntValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cid", clid);
  GWEN_DB_SetCharValue(dbRequest, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "path", path);
  dbResponse=GWEN_DB_Group_new("RFS_Response");
  rv=fs->exchangeFn(fs, dbRequest, dbResponse);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Error exchanging request/response (%d)", rv);
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return rv;
  }
  else {
    GWEN_TYPE_UINT32 i;

    i=GWEN_DB_GetIntValue(dbResponse, "resultCode", 0, -1);
    if (i) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Remote: Error %d (%s)", i,
                GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      GWEN_DB_Group_free(dbResponse);
      GWEN_DB_Group_free(dbRequest);
      return i;
    }
    else {
      GWEN_DB_NODE *dbT;

      DBG_INFO(LC_LOGDOMAIN,
               "Remote: Ok (%s)",
               GWEN_DB_GetCharValue(dbResponse, "resultText", 0, ""));
      dbT=GWEN_DB_GetGroup(dbResponse, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "stat");
      if (!dbT) {
        DBG_ERROR(LC_LOGDOMAIN, "No stat structure returned by remote");
        abort();
      }
      *pStat=LC_FSStat_fromDb(dbT);
      assert(*pStat);
    }
    GWEN_DB_Group_free(dbResponse);
    GWEN_DB_Group_free(dbRequest);
    return 0;
  }
}







