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

#include "cardcontext_p.h"
#include "cardmgr_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>

#include <chipcard2-client/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CARDCONTEXT, LC_CardContext);
GWEN_INHERIT_FUNCTIONS(LC_CARDCONTEXT);




LC_CARDCONTEXT *LC_CardContext_new(LC_CARDMGR *mgr){
  LC_CARDCONTEXT *ctx;

  assert(mgr);
  GWEN_NEW_OBJECT(LC_CARDCONTEXT, ctx);
  GWEN_INHERIT_INIT(LC_CARDCONTEXT, ctx);
  GWEN_LIST_INIT(LC_CARDCONTEXT, ctx);
  ctx->mgr=mgr;
  LC_CardMgr_Attach(ctx->mgr);
  return ctx;
}




void LC_CardContext_free(LC_CARDCONTEXT *ctx){
  if (ctx) {
    GWEN_INHERIT_FINI(LC_CARDCONTEXT, ctx);
    LC_CardMgr_free(ctx->mgr);
    GWEN_LIST_FINI(LC_CARDCONTEXT, ctx);
    GWEN_FREE_OBJECT(ctx);
  }
}



LC_CARDMGR *LC_CardContext_GetManager(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->mgr;
}



GWEN_XMLNODE *LC_CardContext_GetAppNode(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->appNode;
}



void LC_CardContext_SetAppNode(LC_CARDCONTEXT *ctx,
                               GWEN_XMLNODE *n){
  assert(ctx);
  ctx->appNode=n;
}



GWEN_XMLNODE *LC_CardContext_GetDfNode(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->dfNode;
}



void LC_CardContext_SetDfNode(LC_CARDCONTEXT *ctx, GWEN_XMLNODE *n){
  assert(ctx);
  ctx->dfNode=n;
}



GWEN_XMLNODE *LC_CardContext_GetEfNode(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->efNode;
}



void LC_CardContext_SetEfNode(LC_CARDCONTEXT *ctx, GWEN_XMLNODE *n){
  assert(ctx);
  ctx->efNode=n;
}



GWEN_XMLNODE *LC_CardContext_FindFile(LC_CARDCONTEXT *ctx,
                                      const char *type,
                                      const char *fname) {
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *currDF;
  int isSameLevel;

  currDF=ctx->dfNode;
  if (!currDF)
    currDF=ctx->appNode;

  isSameLevel=1;
  while(currDF) {
    n=GWEN_XMLNode_FindNode(currDF, GWEN_XMLNodeTypeTag, "files");
    if (n) {
      n=GWEN_XMLNode_FindFirstTag(n, type, "name", fname);
      if (n) {
        if (isSameLevel) {
          return n;
        }
        if (atoi(GWEN_XMLNode_GetProperty(n, "inAnyDF", "0"))!=0) {
          DBG_DEBUG(LC_LOGDOMAIN, "Returning file from level above");
          return n;
        }
      }
    }
    currDF=GWEN_XMLNode_GetParent(currDF);
    isSameLevel=0;
  }
  DBG_DEBUG(LC_LOGDOMAIN, "%s \"%s\" not found", type, fname);
  return 0;
}








LC_CARDMGR_RESULT LC_CardContext_HandleSelectDF(LC_CARDCONTEXT *ctx,
                                                GWEN_DB_NODE *dbReq){
  GWEN_XMLNODE *n;
  const char *fname;
  int fid;

  assert(ctx);

  fname=GWEN_DB_GetCharValue(dbReq, "fname", 0, 0);
  if (!fname) {
    DBG_ERROR(LC_LOGDOMAIN, "File name missing");
    return LC_CardMgr_ResultCmdError;
  }

  n=LC_CardContext_FindFile(ctx, "DF", fname);
  if (!n) {
    DBG_ERROR(LC_LOGDOMAIN, "DF \"%s\" not found",
              fname);
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(n, "sid", "-1"), "%i",&fid)){
    DBG_ERROR(LC_LOGDOMAIN, "Bad id for DF \"%s\"",
              fname);
    return LC_CardMgr_ResultCmdError;
  }

  if (fid==-1) {
    GWEN_BUFFER *buf;
    const char *lid;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    lid=GWEN_XMLNode_GetProperty(n, "lid", 0);
    if (!lid) {
      DBG_ERROR(LC_LOGDOMAIN, "No long id given in XML file");
      GWEN_Buffer_free(buf);
      return LC_CardMgr_ResultError;
    }
    if (GWEN_Text_FromHexBuffer(lid, buf)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad long id given in XML file");
      GWEN_Buffer_free(buf);
      return LC_CardMgr_ResultError;
    }

    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId",
                        GWEN_Buffer_GetStart(buf),
                        GWEN_Buffer_GetUsedBytes(buf));
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", "SelectDFL");

  }
  else {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId", fid);
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", "SelectDFS");
  }

  ctx->tmpFileNode=n;
  return LC_CardMgr_ResultOk;
}



LC_CARDMGR_RESULT LC_CardContext_HandleSelectEF(LC_CARDCONTEXT *ctx,
                                                GWEN_DB_NODE *dbReq){
  GWEN_XMLNODE *n;
  const char *fname;
  int fid;

  assert(ctx);

  fname=GWEN_DB_GetCharValue(dbReq, "fname", 0, 0);
  if (!fname) {
    DBG_ERROR(LC_LOGDOMAIN, "File name missing");
    return LC_CardMgr_ResultCmdError;
  }

  n=LC_CardContext_FindFile(ctx, "EF", fname);
  if (!n) {
    DBG_ERROR(LC_LOGDOMAIN, "EF \"%s\" not found",
              fname);
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(n, "sid", "-1"), "%i",&fid)){
    DBG_ERROR(LC_LOGDOMAIN, "Bad id for EF \"%s\"",
              fname);
    return LC_CardMgr_ResultCmdError;
  }

  if (fid==-1) {
    GWEN_BUFFER *buf;
    const char *lid;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    lid=GWEN_XMLNode_GetProperty(n, "lid", 0);
    if (!lid) {
      DBG_ERROR(LC_LOGDOMAIN, "No long id given in XML file");
      GWEN_Buffer_free(buf);
      return LC_CardMgr_ResultError;
    }
    if (GWEN_Text_FromHexBuffer(lid, buf)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad long id given in XML file");
      GWEN_Buffer_free(buf);
      return LC_CardMgr_ResultError;
    }

    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId",
                        GWEN_Buffer_GetStart(buf),
                        GWEN_Buffer_GetUsedBytes(buf));
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", "SelectEFL");
  }
  else {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId", fid);
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", "SelectEFS");
  }

  ctx->tmpFileNode=n;
  return LC_CardMgr_ResultOk;
}



LC_CARDMGR_RESULT LC_CardContext_HandleReadRecord(LC_CARDCONTEXT *ctx,
                                                  GWEN_DB_NODE *dbReq){
  int recNum;
  int maxRecNum;
  int minRecNum;
  const char *recName;
  int size;

  assert(ctx);

  recNum=GWEN_DB_GetIntValue(dbReq, "recnum", 0, -1);
  recName=GWEN_DB_GetCharValue(dbReq, "recName", 0, 0);

  if (!ctx->efNode) {
    DBG_ERROR(LC_LOGDOMAIN, "No EF selected");
    return LC_CardMgr_ResultCmdError;
  }

  if (strcasecmp(GWEN_XMLNode_GetProperty(ctx->efNode, "type", ""),
                 "record")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "EF is not of type \"record\"");
    return LC_CardMgr_ResultCmdError;
  }

  /* find record number */
  if (recNum==-1) {
    if (!recName) {
      DBG_ERROR(LC_LOGDOMAIN, "Neither \"recNum\" nor \"recName\" given");
      return LC_CardMgr_ResultCmdError;
    }
    recNum=LC_CardContext_GetRecordNumber(ctx, recName);
  }
  if (recNum==-1) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "minnum", "-1"),
                "%i",
                &minRecNum) ||
      1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "maxnum", "-1"),
                "%i",
                &maxRecNum)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad property \"minnum\" or \"maxnum\"");
    return LC_CardMgr_ResultCmdError;
  }

  DBG_DEBUG(LC_LOGDOMAIN, "Read Record: %d (min: %d, max: %d)",
            recNum, minRecNum, maxRecNum);

  if (minRecNum!=-1 && recNum<minRecNum) {
    DBG_ERROR(LC_LOGDOMAIN, "Record number %d too small (minimum is %d)",
              recNum, minRecNum);
    return LC_CardMgr_ResultCmdError;
  }

  if (maxRecNum!=-1 && recNum>maxRecNum) {
    DBG_ERROR(LC_LOGDOMAIN, "Record number %d too high (maximum is %d)",
              recNum, minRecNum);
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "size", "0"),
                "%i",
                &size)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad property \"size\"");
    return LC_CardMgr_ResultCmdError;
  }
  if (size!=-1) {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "lr", size);
  }

  return LC_CardMgr_ResultOk;
}



LC_CARDMGR_RESULT LC_CardContext_HandleWriteRecord(LC_CARDCONTEXT *ctx,
                                                   GWEN_DB_NODE *dbReq){
  int recNum;
  int maxRecNum;
  int minRecNum;
  const char *recName;
  int size;

  assert(ctx);

  recNum=GWEN_DB_GetIntValue(dbReq, "recnum", 0, -1);
  recName=GWEN_DB_GetCharValue(dbReq, "recName", 0, 0);

  if (!ctx->efNode) {
    DBG_ERROR(LC_LOGDOMAIN, "No EF selected");
    return LC_CardMgr_ResultCmdError;
  }

  if (strcasecmp(GWEN_XMLNode_GetProperty(ctx->efNode, "type", ""),
                 "record")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "EF is not of type \"record\"");
    return LC_CardMgr_ResultCmdError;
  }

  /* find record number */
  if (recNum==-1) {
    if (!recName) {
      DBG_ERROR(LC_LOGDOMAIN, "Neither \"recNum\" nor \"recName\" given");
      return LC_CardMgr_ResultCmdError;
    }
    recNum=LC_CardContext_GetRecordNumber(ctx, recName);
  }
  if (recNum==-1) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "minnum", "-1"),
                "%i",
                &minRecNum) ||
      1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "maxnum", "-1"),
                "%i",
                &maxRecNum)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad property \"minnum\" or \"maxnum\"");
    return LC_CardMgr_ResultCmdError;
  }

  DBG_DEBUG(LC_LOGDOMAIN, "Write Record: %d (min: %d, max: %d)",
            recNum, minRecNum, maxRecNum);

  if (minRecNum!=-1 && recNum<minRecNum) {
    DBG_ERROR(LC_LOGDOMAIN, "Record number %d too small (minimum is %d)",
              recNum, minRecNum);
    return LC_CardMgr_ResultCmdError;
  }

  if (maxRecNum!=-1 && recNum>maxRecNum) {
    DBG_ERROR(LC_LOGDOMAIN, "Record number %d too high (maximum is %d)",
              recNum, minRecNum);
    return LC_CardMgr_ResultCmdError;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(ctx->efNode, "size", "-1"),
                "%i",
                &size)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad property \"size\"");
    return LC_CardMgr_ResultCmdError;
  }

  if (size!=-1) {
    const void *p;
    unsigned int bs;

    p=GWEN_DB_GetBinValue(dbReq, "data", 0, 0, 0, &bs);
    if (p && bs) {
      if (bs>size) {
        DBG_ERROR(LC_LOGDOMAIN, "Too much data for this record (maximum is %d)", size);
        return LC_CardMgr_ResultCmdError;
      }
    }
  }

  return LC_CardMgr_ResultOk;
}





LC_CARDMGR_RESULT LC_CardContext_Translate(LC_CARDCONTEXT *ctx,
                                           GWEN_DB_NODE *dbReq){
  const char *cmdName;

  assert(ctx);

  DBG_VERBOUS(LC_LOGDOMAIN, "Building command from this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbReq, stderr, 2);

  cmdName=GWEN_DB_GetCharValue(dbReq, "name", 0, 0);
  if (!cmdName) {
    DBG_ERROR(LC_LOGDOMAIN, "Command name missing");
    return LC_CardMgr_ResultCmdError;
  }

  DBG_INFO(LC_LOGDOMAIN, "Command: \"%s\"", cmdName);

  if (strcasecmp(cmdName, "SelectDF")==0)
    return LC_CardContext_HandleSelectDF(ctx, dbReq);
  else if (strcasecmp(cmdName, "SelectEF")==0)
    return LC_CardContext_HandleSelectEF(ctx, dbReq);
  else if (strcasecmp(cmdName, "ReadRecord")==0)
    return LC_CardContext_HandleReadRecord(ctx, dbReq);
  else if (strcasecmp(cmdName, "WriteRecord")==0)
    return LC_CardContext_HandleWriteRecord(ctx, dbReq);
  else {
    DBG_DEBUG(LC_LOGDOMAIN, "No translation needed");
    return LC_CardMgr_ResultOk;
  }
}



LC_CARDMGR_RESULT LC_CardContext_CheckResponse(LC_CARDCONTEXT *ctx,
                                               GWEN_DB_NODE *dbReq,
                                               GWEN_DB_NODE *dbServerRsp){
  const char *cmdName;

  assert(ctx);
  assert(dbReq);
  cmdName=GWEN_DB_GetCharValue(dbReq, "name", 0, 0);
  if (!cmdName) {
    DBG_ERROR(LC_LOGDOMAIN, "Command name missing");
    return LC_CardMgr_ResultCmdError;
  }

  DBG_VERBOUS(LC_LOGDOMAIN, "Server response was this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbServerRsp, stderr, 2);

  /* catch DF/EF selection changes, post-process commands */
  DBG_DEBUG(LC_LOGDOMAIN, "Post processing command \"%s\"", cmdName);
  if (strcasecmp(cmdName, "SelectDFS")==0 ||
      strcasecmp(cmdName, "SelectDFL")==0) {
    assert(ctx->tmpFileNode);
    if (strcasecmp(GWEN_DB_GetCharValue(dbServerRsp, "result/type", 0, "error"),
                   "success")==0) {
      ctx->dfNode=ctx->tmpFileNode;
      ctx->efNode=0;
    }
    ctx->tmpFileNode=0;
  }
  else if (strcasecmp(cmdName, "SelectEFS")==0){
    assert(ctx->tmpFileNode);
    if (strcasecmp(GWEN_DB_GetCharValue(dbServerRsp, "result/type", 0, "error"),
                   "success")==0) {
      DBG_NOTICE(LC_LOGDOMAIN, "EF selected");
      ctx->efNode=ctx->tmpFileNode;
    }
    ctx->tmpFileNode=0;
  }
  else if (strcasecmp(cmdName, "SelectMF")==0){
    if (strcasecmp(GWEN_DB_GetCharValue(dbServerRsp,
                                        "result/type", 0, "error"),
                   "success")==0) {
      ctx->dfNode=0;
      ctx->efNode=0;
    }
    ctx->tmpFileNode=0;
  }
  return LC_CardMgr_ResultOk;
}



int LC_CardContext_GetRecordNumber(LC_CARDCONTEXT *ctx,
                                   const char *recName) {
  int recNum;
  GWEN_XMLNODE *rtag;

  assert(ctx);

  if (!ctx->efNode) {
    DBG_ERROR(LC_LOGDOMAIN, "No EF selected");
    return LC_CardMgr_ResultCmdError;
  }

  if (strcasecmp(GWEN_XMLNode_GetProperty(ctx->efNode, "type", ""),
                 "record")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "EF is not of type \"record\"");
    return -1;
  }

  /* find record number */
  rtag=GWEN_XMLNode_FindFirstTag(ctx->efNode,
                                 "record",
                                 "name",
                                 recName);
  if (!rtag) {
    DBG_ERROR(LC_LOGDOMAIN, "Record with given name not found");
    return -1;
  }
  if (1!=sscanf(GWEN_XMLNode_GetProperty(rtag, "recnum", "-1"),
                "%i",
                &recNum)) {
    DBG_ERROR(LC_LOGDOMAIN, "\"recnum\" needed in XML file");
    return -1;
  }
  DBG_NOTICE(LC_LOGDOMAIN, "Record number is %d (from name=\"%s\")",
             recNum, recName);
  return recNum;
}



int LC_CardContext_ParseRecord(LC_CARDCONTEXT *ctx,
                               int recNum,
                               GWEN_BUFFER *buf,
                               GWEN_DB_NODE *dbRecord){
  GWEN_XMLNODE *recordNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(ctx->efNode);
  e=LC_CardMgr_GetMsgEngine(ctx->mgr);
  assert(e);
  if (!GWEN_Buffer_GetBytesLeft(buf)) {
    DBG_ERROR(LC_LOGDOMAIN, "End of buffer reached");
    return -1;
  }
  recordNode=GWEN_XMLNode_FindFirstTag(ctx->efNode, "record", 0, 0);
  while(recordNode) {
    int lrecNum;

    if (1==sscanf(GWEN_XMLNode_GetProperty(recordNode,
                                           "recnum", "-1"),
                  "%i", &lrecNum)) {
      if (lrecNum!=-1 && recNum==lrecNum)
        break;
    }
    recordNode=GWEN_XMLNode_FindNextTag(recordNode, "record", 0, 0);
  } /* while */
  if (!recordNode)
    recordNode=GWEN_XMLNode_FindFirstTag(ctx->efNode,"record", 0, 0);

  if (recordNode) {
    /* node found, parse data */
    DBG_DEBUG(LC_LOGDOMAIN, "Parsing record data");
    if (GWEN_MsgEngine_ParseMessage(e,
                                    recordNode,
                                    buf,
                                    dbRecord,
                                    GWEN_MSGENGINE_READ_FLAGS_DEFAULT)){
      DBG_ERROR(LC_LOGDOMAIN, "Error parsing response");
      return -1;
    }
  } /* if record found */
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Record not found");
    return -1;
  }
  return 0;
}



int LC_CardContext_CreateRecord(LC_CARDCONTEXT *ctx,
                                int recNum,
                                GWEN_BUFFER *buf,
                                GWEN_DB_NODE *dbRecord){
  GWEN_XMLNODE *recordNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(ctx->efNode);
  e=LC_CardMgr_GetMsgEngine(ctx->mgr);
  assert(e);
  recordNode=GWEN_XMLNode_FindFirstTag(ctx->efNode, "record", 0, 0);
  while(recordNode) {
    int lrecNum;

    if (1==sscanf(GWEN_XMLNode_GetProperty(recordNode,
                                           "recnum", "-1"),
                  "%i", &lrecNum)) {
      if (lrecNum!=-1 && recNum==lrecNum)
        break;
    }
    recordNode=GWEN_XMLNode_FindNextTag(recordNode, "record", 0, 0);
  } /* while */
  if (!recordNode)
    recordNode=GWEN_XMLNode_FindFirstTag(ctx->efNode,"record", 0, 0);

  if (recordNode) {
    /* node found, parse data */
    DBG_DEBUG(LC_LOGDOMAIN, "Creating record data");
    if (GWEN_MsgEngine_CreateMessageFromNode(e,
                                             recordNode,
                                             buf,
                                             dbRecord)) {
      DBG_ERROR(LC_LOGDOMAIN, "Error creating record");
      return -1;
    }
  } /* if record found */
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Record not found");
    return -1;
  }
  return 0;
}



int LC_CardContext_ParseData(LC_CARDCONTEXT *ctx,
                             const char *format,
                             GWEN_BUFFER *buf,
                             GWEN_DB_NODE *dbData){
  GWEN_XMLNODE *dataNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(ctx->appNode);
  e=LC_CardMgr_GetMsgEngine(ctx->mgr);
  assert(e);
  if (!GWEN_Buffer_GetBytesLeft(buf)) {
    DBG_ERROR(LC_LOGDOMAIN, "End of buffer reached");
    return -1;
  }
  dataNode=GWEN_XMLNode_FindFirstTag(ctx->appNode, "formats", 0, 0);
  if (dataNode==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No formats for this card application");
    return -1;
  }

  dataNode=GWEN_XMLNode_FindFirstTag(dataNode, "format", "name", format);
  if (!dataNode) {
    DBG_ERROR(LC_LOGDOMAIN, "Format \"%s\" not found", format);
    return -1;
  }

  /* node found, parse data */
  DBG_DEBUG(LC_LOGDOMAIN, "Parsing data");
  if (GWEN_MsgEngine_ParseMessage(e,
                                  dataNode,
                                  buf,
                                  dbData,
                                  GWEN_MSGENGINE_READ_FLAGS_DEFAULT)){
    DBG_ERROR(LC_LOGDOMAIN, "Error parsing data in format \"%s\"", format);
    return -1;
  }

  return 0;
}



int LC_CardContext_CreateData(LC_CARDCONTEXT *ctx,
                              const char *format,
                              GWEN_BUFFER *buf,
                              GWEN_DB_NODE *dbData){
  GWEN_XMLNODE *dataNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(ctx->appNode);
  e=LC_CardMgr_GetMsgEngine(ctx->mgr);
  assert(e);

  dataNode=GWEN_XMLNode_FindFirstTag(ctx->appNode, "formats", 0, 0);
  if (dataNode==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No formats for this card application");
    return -1;
  }

  dataNode=GWEN_XMLNode_FindFirstTag(dataNode, "format", "name", format);
  if (!dataNode) {
    DBG_ERROR(LC_LOGDOMAIN, "Format \"%s\" not found", format);
    return -1;
  }

  /* node found, parse data */
  DBG_DEBUG(LC_LOGDOMAIN, "Creating data");
  if (GWEN_MsgEngine_CreateMessageFromNode(e,
                                           dataNode,
                                           buf,
                                           dbData)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error creating data for format \"%s\"", format);
    return -1;
  }

  return 0;
}





