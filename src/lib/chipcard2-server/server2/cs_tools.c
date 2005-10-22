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


#include "server_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/buffer.h>




int LCS_Server_ReplaceVar(const char *path,
                          const char *var,
                          const char *value,
                          GWEN_BUFFER *nbuf) {
  unsigned int vlen;

  vlen=strlen(var);

  while(*path) {
    int handled;

    handled=0;
    if (*path=='@') {
      if (strncmp(path+1, var, vlen)==0) {
        if (path[vlen+1]=='@') {
          /* found variable, replace it */
          GWEN_Buffer_AppendString(nbuf, value);
          path+=vlen+2;
          handled=1;
        }
      }
    }
    if (!handled) {
      GWEN_Buffer_AppendByte(nbuf, *path);
      path++;
    }
  } /* while */

  return 0;
}



int LCS_Server_SendErrorResponse(LCS_SERVER *cs,
                                 GWEN_TYPE_UINT32 rid,
                                 int code,
                                 const char *text) {
  GWEN_DB_NODE *gr;

  gr=GWEN_DB_Group_new("Error");
  GWEN_DB_SetIntValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", code);
  if (text)
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", text);
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, gr)) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  return 0;
}



void LCS_Server_DumpState(const LCS_SERVER *cs) {
  if (!cs) {
    fprintf(stderr, "No Server.\n");
    return;
  }
  else {
    LCDM_DeviceManager_DumpState(cs->deviceManager);
  }
}


