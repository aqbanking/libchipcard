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
#include <chipcard2-server/common/driverinfo.h>
#include "card_l.h"
#include "cardmgr_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>

#include <chipcard2/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CARDCONTEXT, LC_CardContext);
GWEN_INHERIT_FUNCTIONS(LC_CARDCONTEXT);


LC_CARDCONTEXT *LC_CardContext_new(LC_CARDMGR *mgr){
  LC_CARDCONTEXT *ctx;

  assert(mgr);
  GWEN_NEW_OBJECT(LC_CARDCONTEXT, ctx);
  DBG_MEM_INC("LC_CARDCONTEXT", 0);
  ctx->usage=1;
  GWEN_INHERIT_INIT(LC_CARDCONTEXT, ctx);
  GWEN_LIST_INIT(LC_CARDCONTEXT, ctx);
  ctx->mgr=mgr;
  ctx->buildCmdFn=LC_CardContext__BuildCmd;
  ctx->checkCmdFn=LC_CardContext__CheckCmd;
  LC_CardMgr_Attach(ctx->mgr);
  return ctx;
}




void LC_CardContext_free(LC_CARDCONTEXT *ctx){
  if (ctx) {
    assert(ctx->usage);
    DBG_MEM_DEC("LC_CARDCONTEXT");
    if (--(ctx->usage)==0) {
      GWEN_INHERIT_FINI(LC_CARDCONTEXT, ctx);
      LC_CardMgr_free(ctx->mgr);
      GWEN_LIST_FINI(LC_CARDCONTEXT, ctx);
      GWEN_FREE_OBJECT(ctx);
    }
  }
}



LC_CARDMGR_RESULT LC_CardContext_BuildCmd(LC_CARDCONTEXT *ctx,
                                          LC_CARD *card,
                                          GWEN_TYPE_UINT32 rid,
                                          GWEN_DB_NODE *dbReq,
                                          GWEN_DB_NODE *dbRsp){
  assert(ctx);
  assert(ctx->buildCmdFn);
  return ctx->buildCmdFn(ctx, card, rid, dbReq, dbRsp);
}



LC_CARDMGR_RESULT LC_CardContext_CheckCmd(LC_CARDCONTEXT *ctx,
                                          LC_REQUEST *rq,
                                          GWEN_DB_NODE *dbDriverRsp,
                                          GWEN_DB_NODE *dbRsp){
  assert(ctx);
  assert(ctx->checkCmdFn);
  return ctx->checkCmdFn(ctx, rq, dbDriverRsp, dbRsp);
}



void LC_CardContext_SetBuildCmdFn(LC_CARDCONTEXT *ctx,
                                  LC_CARDCONTEXT_BUILDCMD fn){
  assert(ctx);
  ctx->buildCmdFn=fn;
}



void LC_CardContext_SetCheckCmdFn(LC_CARDCONTEXT *ctx,
                                  LC_CARDCONTEXT_CHECKCMD fn){
  assert(ctx);
  ctx->checkCmdFn=fn;
}



LC_CARDMGR *LC_CardContext_GetManager(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->mgr;
}



GWEN_XMLNODE *LC_CardContext_GetCardNode(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->cardNode;
}



void LC_CardContext_SetCardNode(LC_CARDCONTEXT *ctx,
                                GWEN_XMLNODE *n){
  assert(ctx);
  ctx->cardNode=n;
}



GWEN_XMLNODE *LC_CardContext_GetCmdNode(const LC_CARDCONTEXT *ctx){
  assert(ctx);
  return ctx->cmdNode;
}



void LC_CardContext_SetCmdNode(LC_CARDCONTEXT *ctx, GWEN_XMLNODE *n){
  assert(ctx);
  ctx->cmdNode=n;
}



GWEN_XMLNODE *LC_CardContext__FindCommand(GWEN_XMLNODE *node,
                                          const char *commandName,
                                          const char *driverType,
                                          const char *readerType){
  GWEN_XMLNODE *cmds;
  GWEN_XMLNODE *n;

  DBG_DEBUG(0, "Searching in \"%s\"",
            GWEN_XMLNode_GetProperty(node, "name", "(noname)"));

  cmds=GWEN_XMLNode_FindNode(node,
                             GWEN_XMLNodeTypeTag,
                             "commands");
  if (!cmds) {
    DBG_INFO(0, "No commands in card data");
    return 0;
  }

  /* first try exact match */
  if (driverType && readerType) {
    DBG_DEBUG(0, "Searching for %s/%s/%s",
              driverType, readerType, commandName);
    n=GWEN_XMLNode_FindFirstTag(cmds,
                                "command",
                                "name",
                                commandName);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0 &&
          strcasecmp(GWEN_XMLNode_GetProperty(n, "reader", ""),
                     readerType)==0) {
        DBG_DEBUG(0, "Found command in %s/%s", driverType, readerType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
    } /* while */
  }

  if (driverType) {
    /* try match of driver only */
    DBG_DEBUG(0, "Searching for %s/%s",
              driverType, commandName);
    n=GWEN_XMLNode_FindFirstTag(cmds,
                                "command",
                                "name",
                                commandName);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0) {
        DBG_DEBUG(0, "Found command in %s", driverType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
    } /* while */
  }

  /* try match of command name only */
  DBG_DEBUG(0, "Searching for %s", commandName);
  n=GWEN_XMLNode_FindFirstTag(cmds,
                              "command",
                              "name",
                              commandName);
  while(n) {
    if (!GWEN_XMLNode_GetProperty(n, "driver", 0))
      return n;
    n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
  } /* while */

  return n;
}



GWEN_XMLNODE *LC_CardContext_FindCommand(LC_CARDCONTEXT *ctx,
                                         const char *commandName,
                                         const char *driverType,
                                         const char *readerType){
  GWEN_XMLNODE *node;

  node=ctx->cardNode;
  assert(node);

  while(node) {
    GWEN_XMLNODE *n;
    const char *parent;

    n=LC_CardContext__FindCommand(node, commandName,
                                  driverType, readerType);
    if (n) {
      return n;
    }
    parent=GWEN_XMLNode_GetProperty(node, "extends", 0);
    if (!parent)
      break;
    node=LC_CardMgr_FindCardNode(ctx->mgr, parent);
    if (!node) {
      DBG_WARN(0, "Extended card \"%s\" not found", parent);
      break;
    }
    DBG_DEBUG(0, "Searching in parent \"%s\"", parent);
  } /* while */

  DBG_DEBUG(0, "Command \"%s\" not found", commandName);
  return 0;
}



GWEN_XMLNODE *LC_CardContext__FindResult(LC_CARDCONTEXT *ctx,
                                         GWEN_XMLNODE *cmd,
                                         int sw1,
                                         int sw2) {
  GWEN_XMLNODE *rnode;
  GWEN_XMLNODE *n;
  int lsw1, lsw2;

  DBG_DEBUG(0, "Searching for result type of %02x/%02x", sw1, sw2);
  while(cmd) {
    rnode=GWEN_XMLNode_FindNode(cmd,
                                GWEN_XMLNodeTypeTag,
                                "results");
    if (rnode) {
      /* first try exact match */
      n=GWEN_XMLNode_GetFirstTag(rnode);
      while(n) {
        if (1==sscanf(GWEN_XMLNode_GetProperty(n, "sw1", "-1"),
                      "%i", &lsw1) &&
            1==sscanf(GWEN_XMLNode_GetProperty(n, "sw2", "-1"),
                      "%i", &lsw2)) {
          DBG_VERBOUS(0, "Checking %02x/%02x", lsw1, lsw2);
          if (lsw1==sw1 && lsw2==sw2) {
            return n;
          }
        }
        else {
          DBG_WARN(0, "Bad SW1 or SW2 value");
        }
        n=GWEN_XMLNode_GetNextTag(n);
      } /* while */

      /* try SW1 only */
      n=GWEN_XMLNode_GetFirstTag(rnode);
      while(n) {
        if (1==sscanf(GWEN_XMLNode_GetProperty(n, "sw1", "-1"),
                      "%i", &lsw1) &&
            1==sscanf(GWEN_XMLNode_GetProperty(n, "sw2", "-1"),
                      "%i", &lsw2)) {
          if (lsw1==sw1 && lsw2==-1) {
            return n;
          }
        }
        else {
          DBG_WARN(0, "Bad SW1 or SW2 value");
        }
        n=GWEN_XMLNode_GetNextTag(n);
      } /* while */
    } /* if rnode */

    /* select parent */
    cmd=GWEN_XMLNode_GetParent(cmd);
  }

  return 0;
}



GWEN_XMLNODE *LC_CardContext_FindResult(LC_CARDCONTEXT *ctx,
                                        GWEN_XMLNODE *cmd,
                                        int sw1,
                                        int sw2) {
  GWEN_XMLNODE *tmpNode;
  GWEN_XMLNODE *rnode;

  tmpNode=cmd;
  rnode=0;

  /* find in command */
  rnode=LC_CardContext__FindResult(ctx, cmd, sw1, sw2);
  if (rnode)
    return rnode;
  rnode=LC_CardContext__FindResult(ctx, cmd, -1, -1);
  if (rnode)
    return rnode;

  tmpNode=ctx->cardNode;
  for(;;) {
    const char *parent;

    parent=GWEN_XMLNode_GetProperty(tmpNode, "extends", 0);
    if (!parent) {
      break;
    }
    tmpNode=LC_CardMgr_FindCardNode(ctx->mgr, parent);
    if (!tmpNode) {
      break;
    }

    rnode=LC_CardContext__FindResult(ctx, tmpNode, sw1, sw2);
    if (rnode) {
      break;
    }
    rnode=LC_CardContext__FindResult(ctx, tmpNode, -1, -1);
    if (rnode) {
      break;
    }
  } /* for */

  return rnode;
}



GWEN_XMLNODE *LC_CardContext_FindResponse(LC_CARDCONTEXT *ctx,
                                          GWEN_XMLNODE *cmd,
                                          const char *typ) {
  GWEN_XMLNODE *rnode;
  GWEN_XMLNODE *n;
  const char *ltyp;

  DBG_DEBUG(0, "Searching for response type \"%s\"", typ);
  rnode=GWEN_XMLNode_FindNode(cmd,
                              GWEN_XMLNodeTypeTag,
                              "responses");
  if (!rnode) {
    DBG_DEBUG(0, "No <responses> tag in command definition");
    return 0;
  }

  /* first try exact match */
  n=GWEN_XMLNode_GetFirstTag(rnode);
  while(n) {
    ltyp=GWEN_XMLNode_GetProperty(n, "type", 0);
    if (ltyp) {
      if (strcasecmp(ltyp, typ)==0)
        return n;
    }
    n=GWEN_XMLNode_GetNextTag(n);
  } /* while */

  n=GWEN_XMLNode_GetFirstTag(rnode);
  while(n) {
    ltyp=GWEN_XMLNode_GetProperty(n, "type", 0);
    if (!ltyp)
      return n;
    n=GWEN_XMLNode_GetNextTag(n);
  } /* while */

  return 0;
}


int LC_CardContext_CreateAPDU(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *node,
                              GWEN_BUFFER *gbuf,
                              GWEN_DB_NODE *cmdData){
  GWEN_MSGENGINE *e;
  GWEN_XMLNODE *sendNode;
  GWEN_XMLNODE *dataNode;
  GWEN_XMLNODE *apduNode;
  GWEN_BUFFER *dataBuffer;
  unsigned int i;
  int j;

  e=LC_CardMgr_GetMsgEngine(LC_CardContext_GetManager(ctx));
  assert(e);

  sendNode=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "send");
  if (!sendNode) {
    DBG_ERROR(0, "No <send> tag in command definition");
    return -1;
  }

  apduNode=GWEN_XMLNode_FindNode(sendNode,
                                 GWEN_XMLNodeTypeTag, "apdu");
  if (!apduNode) {
    DBG_ERROR(0, "No <apdu> tag in command definition");
    return -1;
  }

  dataBuffer=GWEN_Buffer_new(0, 256, 0, 1);
  dataNode=GWEN_XMLNode_FindNode(sendNode,
                                 GWEN_XMLNodeTypeTag, "data");
  if (dataNode) {
    /* there is a data node, sample data */
    if (GWEN_MsgEngine_CreateMessageFromNode(e,
                                             dataNode,
                                             dataBuffer,
                                             cmdData)) {
      DBG_ERROR(0, "Error creating data for APDU");
      GWEN_Buffer_free(dataBuffer);
      return -1;
    }
  }

  if (GWEN_MsgEngine_CreateMessageFromNode(e,
                                           apduNode,
                                           gbuf,
                                           cmdData)) {
    DBG_ERROR(0, "Error creating APDU");
    GWEN_Buffer_free(dataBuffer);
    return -1;
  }

  i=GWEN_Buffer_GetUsedBytes(dataBuffer);
  if (i) {
    GWEN_Buffer_AppendByte(gbuf, (unsigned char)i);
    GWEN_Buffer_AppendBuffer(gbuf, dataBuffer);
  }
  GWEN_Buffer_free(dataBuffer);

  j=0;
  if (1!=sscanf(GWEN_XMLNode_GetProperty(apduNode, "lr", "0"),
                "%i", &j))
    j=0;

  if (j!=-1) {
    j=GWEN_DB_GetIntValue(cmdData, "lr", 0, -1);
    if (j==-1) {
      if (1!=sscanf(GWEN_XMLNode_GetProperty(apduNode, "lr", "-1"),
                    "%i", &j))
        j=-1;
    }
  }
  if (j>=0)
    GWEN_Buffer_AppendByte(gbuf, (unsigned char)j);

  DBG_INFO(0, "APDU is:");
  GWEN_Text_LogString(GWEN_Buffer_GetStart(gbuf),
                      GWEN_Buffer_GetUsedBytes(gbuf),
                      0,
                      GWEN_LoggerLevelInfo);
  return 0;
}



int LC_CardContext_ParseResult(LC_CARDCONTEXT *ctx,
                               GWEN_XMLNODE *node,
                               GWEN_BUFFER *gbuf,
                               GWEN_DB_NODE *rspData){
  unsigned int i;
  int sw1, sw2;
  GWEN_DB_NODE *dbTmp;
  GWEN_XMLNODE *rnode;

  GWEN_Buffer_Rewind(gbuf); /* just in case ... */
  i=GWEN_Buffer_GetUsedBytes(gbuf);
  if (i<2) {
    DBG_ERROR(0, "Answer too small (less than 2 bytes)");
    return -1;
  }
  DBG_INFO(0, "Driver response:");
  GWEN_Text_LogString(GWEN_Buffer_GetStart(gbuf), i,
                      0, GWEN_LoggerLevelInfo);
  sw1=(unsigned char)(GWEN_Buffer_GetStart(gbuf)[i-2]);
  sw2=(unsigned char)(GWEN_Buffer_GetStart(gbuf)[i-1]);
  GWEN_Buffer_Crop(gbuf, 0, i-2);
  /* store result */
  dbTmp=GWEN_DB_GetGroup(rspData,
                         GWEN_DB_FLAGS_DEFAULT |
                         GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                         "result");
  assert(dbTmp);
  GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                      "sw1", sw1);
  GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                      "sw2", sw2);

  rnode=LC_CardContext_FindResult(ctx, node, sw1, sw2);
  if (rnode) {
    const char *t;
    GWEN_XMLNODE *tnode;
    GWEN_BUFFER *txtbuf;
    int first;

    t=GWEN_XMLNode_GetProperty(rnode, "type", "success");
    DBG_INFO(0, "Result is: %s", t);
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "type", t);
    /* get text */
    txtbuf=GWEN_Buffer_new(0, 256, 0, 1);
    first=1;
    tnode=GWEN_XMLNode_GetFirstData(rnode);
    while(tnode) {
      const char *p;

      p=GWEN_XMLNode_GetData(tnode);
      if (p) {
        if (!first)
          GWEN_Buffer_AppendByte(txtbuf, ' ');
        GWEN_Buffer_AppendString(txtbuf, p);
      }
      if (first)
        first=0;
      tnode=GWEN_XMLNode_GetNextData(tnode);
    } /* while */

    if (GWEN_Buffer_GetUsedBytes(txtbuf))
      GWEN_DB_SetCharValue(dbTmp, GWEN_DB_FLAGS_DEFAULT, "text",
                           GWEN_Buffer_GetStart(txtbuf));
    GWEN_Buffer_free(txtbuf);
  }
  else {
    DBG_ERROR(0,
              "Result for %02x/%02x not found, assuming error",
              sw1, sw2);
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "type", "error");
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "text", "Result not found");
  }

  return 0;
}



int LC_CardContext_ParseResponse(LC_CARDCONTEXT *ctx,
                                 GWEN_XMLNODE *node,
                                 GWEN_BUFFER *gbuf,
                                 GWEN_DB_NODE *rspData){
  GWEN_MSGENGINE *e;
  GWEN_DB_NODE *dbTmp;
  GWEN_XMLNODE *rnode;
  const char *p;

  assert(ctx);

  GWEN_Buffer_Rewind(gbuf); /* just in case ... */
  e=LC_CardMgr_GetMsgEngine(LC_CardContext_GetManager(ctx));
  assert(e);

  p=GWEN_DB_GetCharValue(rspData, "result/type", 0, 0);
  if (!p) {
    DBG_ERROR(0, "No result type given");
    return -1;
  }
  dbTmp=GWEN_DB_GetGroup(rspData,
                         GWEN_DB_FLAGS_DEFAULT |
                         GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                         "response");
  if (!dbTmp) {
    DBG_ERROR(0, "No matching response tag found");
    return -1;
  }

  rnode=LC_CardContext_FindResponse(ctx, node, p);
  if (!rnode) {
    DBG_DEBUG(0, "Did not find response");
    if (GWEN_Buffer_GetUsedBytes(gbuf)) {
      GWEN_DB_SetBinValue(dbTmp,
                          GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "data",
                          GWEN_Buffer_GetStart(gbuf),
                          GWEN_Buffer_GetUsedBytes(gbuf));
    }
  }
  else {
    if (GWEN_MsgEngine_ParseMessage(e,
                                    rnode,
                                    gbuf,
                                    dbTmp,
                                    GWEN_MSGENGINE_READ_FLAGS_DEFAULT)){
      DBG_ERROR(0, "Error parsing response");
      return -1;
    }
  }

  return 0;
}



int LC_CardContext_ParseAnswer(LC_CARDCONTEXT *ctx,
                               GWEN_BUFFER *gbuf,
                               GWEN_DB_NODE *rspData){
  assert(ctx);
  assert(ctx->cmdNode);
  if (LC_CardContext_ParseResult(ctx,
                                 ctx->cmdNode,
                                 gbuf,
                                 rspData)) {
    DBG_INFO(0, "Error parsing result");
    return -1;
  }

  if (LC_CardContext_ParseResponse(ctx, ctx->cmdNode, gbuf, rspData)){
    DBG_INFO(0, "Error parsing response");
    return -1;
  }

  return 0;
}



LC_CARDMGR_RESULT LC_CardContext__BuildCmd(LC_CARDCONTEXT *ctx,
                                           LC_CARD *card,
                                           GWEN_TYPE_UINT32 rid,
                                           GWEN_DB_NODE *dbReq,
                                           GWEN_DB_NODE *dbRsp){
  const char *cmdName;

  assert(ctx);

  DBG_VERBOUS(0, "Building command from this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbReq, stderr, 2);

  cmdName=GWEN_DB_GetCharValue(dbReq, "body/command/name", 0, 0);
  if (!cmdName) {
    DBG_ERROR(0, "Command name missing");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "command name missing");
    return LC_CardMgr_ResultCmdError;
  }

  DBG_INFO(0, "Command: \"%s\"", cmdName);

  if (LC_CardContext_CreateGenericCommand(ctx,
                                          card,
                                          rid,
                                          cmdName,
					  dbReq)){
    DBG_ERROR(0, "Error creating command");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			"code", LC_ERROR_SETUP);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "error creating command");
    return LC_CardMgr_ResultError;
  }
  return LC_CardMgr_ResultOk;
}



LC_CARDMGR_RESULT LC_CardContext__CheckCmd(LC_CARDCONTEXT *ctx,
                                           LC_REQUEST *rq,
                                           GWEN_DB_NODE *dbDriverRsp,
                                           GWEN_DB_NODE *dbRsp){
  const char *cmdName;
  const char *rspName;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbDriverRspBody;

  assert(ctx);
  assert(rq);
  dbReq=LC_Request_GetInRequestData(rq);
  assert(dbReq);
  cmdName=GWEN_DB_GetCharValue(dbReq, "body/command/name", 0, 0);
  if (!cmdName) {
    DBG_ERROR(0, "Command name missing");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "command name missing");
    return LC_CardMgr_ResultCmdError;
  }

  if (1) {
    const void *p;
    unsigned int bs;

    p=GWEN_DB_GetBinValue(dbDriverRsp, "body/data", 0, 0, 0, &bs);
    if (p && bs) {
      DBG_INFO(0, "Driver response:");
      GWEN_Text_LogString(p, bs, 0, GWEN_LoggerLevelInfo);
    }
  }

  DBG_VERBOUS(0, "Driver response was this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbDriverRsp, stderr, 2);

  dbDriverRspBody=GWEN_DB_GetGroup(dbDriverRsp,
                                   GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                   "body");
  assert(dbDriverRspBody);
  rspName=GWEN_DB_GetCharValue(dbDriverRsp, "command/vars/cmd",0, 0);
  assert(rspName);

  /* check for error */
  if (strcasecmp(rspName, "error")==0) {
    DBG_ERROR(0, "Error in driver response");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_AddGroupChildren(dbRsp, dbDriverRspBody);
    return LC_CardMgr_ResultCmdError;
  }
  else if (strcasecmp(rspName,
                      "CardCommandResponse")==0) {
    const void *p;
    unsigned int bs;

    /* parse response */
    p=GWEN_DB_GetBinValue(dbDriverRspBody, "data", 0, 0, 0, &bs);
    if (p && bs) {
      GWEN_BUFFER *buf;

      buf=GWEN_Buffer_new(0, bs, 0, 1);
      GWEN_Buffer_AppendBytes(buf, p, bs);
      if (LC_CardContext_ParseAnswer(ctx, buf, dbRsp)) {
        GWEN_Buffer_free(buf);
        GWEN_DB_GroupRename(dbRsp, "error");
        GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text",
                             "error parsing driver response");
        return LC_CardMgr_ResultCmdError;
      }
      GWEN_Buffer_free(buf);
      buf=GWEN_Buffer_new(0, 64, 0, 1);
      GWEN_Buffer_AppendString(buf, cmdName);
      GWEN_Buffer_AppendString(buf, "Response");
      GWEN_DB_GroupRename(dbRsp, GWEN_Buffer_GetStart(buf));
      GWEN_Buffer_free(buf);
    }
    else {
      DBG_ERROR(0, "No response data from driver");
      GWEN_DB_GroupRename(dbRsp, "error");
      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", LC_ERROR_GENERIC);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           "no data in driver response");
      return LC_CardMgr_ResultCmdError;
    }
  }
  else {
    DBG_ERROR(0, "Unknown driver response \"%s\"", rspName);
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "internal error "
                         "(unknown driver response)");
    return LC_CardMgr_ResultCmdError;
  }

  return LC_CardMgr_ResultOk;
}



int LC_CardContext_CreateGenericCommand(LC_CARDCONTEXT *ctx,
                                        LC_CARD *card,
                                        GWEN_TYPE_UINT32 rid,
                                        const char *cmd,
                                        GWEN_DB_NODE *dbReq){
  LC_READER *r;
  LC_DRIVER *d;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbDriverCmd;
  LC_REQUEST *rq;
  GWEN_DB_NODE *dbCmd;
  char numbuf[16];
  const char *target;
  GWEN_XMLNODE *cmdNode;

  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);

  cmdNode=LC_CardContext_FindCommand(ctx,
                                     cmd,
                                     LC_Driver_GetDriverName(d),
                                     LC_Reader_GetReaderType(r));

  if (!cmdNode) {
    DBG_ERROR(0, "Command \"%s\" not found", cmd);
    return -1;
  }
  ctx->cmdNode=cmdNode;

  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  dbCmd=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                         "body/command");

  if (LC_CardContext_CreateAPDU(ctx, cmdNode, mbuf, dbCmd)) {
    DBG_ERROR(0, "Error creating APDU for command \"%s\"",
              cmd);
    GWEN_Buffer_free(mbuf);
    return -1;
  }

  dbDriverCmd=GWEN_DB_Group_new("CardCommand");
  target=GWEN_XMLNode_GetProperty(cmdNode, "target", "card");
  GWEN_DB_SetCharValue(dbDriverCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);

  GWEN_DB_SetBinValue(dbDriverCmd, GWEN_DB_FLAGS_DEFAULT,
                      "data",
                      GWEN_Buffer_GetStart(mbuf),
                      GWEN_Buffer_GetUsedBytes(mbuf));
  GWEN_Buffer_free(mbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbDriverCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbDriverCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LC_Card_GetSlot(card));
  GWEN_DB_SetIntValue(dbDriverCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", LC_Card_GetReadersCardId(card));

  rq=LC_Request_new(LC_Card_GetClient(card),
                    GWEN_DB_Group_dup(dbReq),
                    dbDriverCmd,
                    card);
  LC_Request_SetInRequestId(rq, rid);
  LC_Reader_AddRequest(r, rq);
  DBG_DEBUG(0, "Added request \"%08x\" for command \"%s\"",
            rid, cmd);
  return 0;
}



GWEN_XMLNODE *LC_CardContext__FindFlags(GWEN_XMLNODE *node,
                                        const char *driverType,
                                        const char *readerType){
  GWEN_XMLNODE *n;

  DBG_DEBUG(0, "Searching in \"%s\"",
            GWEN_XMLNode_GetProperty(node, "name", "(noname)"));

  /* first try exact match */
  if (driverType && readerType) {
    DBG_DEBUG(0, "Searching for %s/%s",
              driverType, readerType);
    n=GWEN_XMLNode_FindFirstTag(node, "flags", 0, 0);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0 &&
          strcasecmp(GWEN_XMLNode_GetProperty(n, "reader", ""),
                     readerType)==0) {
        DBG_DEBUG(0, "Found flags in %s/%s", driverType, readerType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "flags", 0, 0);
    } /* while */
  }

  if (driverType) {
    /* try match of driver only */
    DBG_DEBUG(0, "Searching for %s", driverType);
    n=GWEN_XMLNode_FindFirstTag(node, "flags", 0, 0);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0) {
        DBG_DEBUG(0, "Found command in %s", driverType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "flags", 0, 0);
    } /* while */
  }

  /* try match of command name only */
  DBG_DEBUG(0, "Searching for global card flags");
  n=GWEN_XMLNode_FindFirstTag(node, "flags", 0, 0);
  while(n) {
    if (!GWEN_XMLNode_GetProperty(n, "driver", 0))
      return n;
    n=GWEN_XMLNode_FindNextTag(n, "flags", 0, 0);
  } /* while */

  return n;
}



GWEN_XMLNODE *LC_CardContext_FindFlags(LC_CARDCONTEXT *ctx,
                                       const char *driverType,
                                       const char *readerType){
  GWEN_XMLNODE *node;

  node=ctx->cardNode;
  assert(node);

  while(node) {
    GWEN_XMLNODE *n;
    const char *parent;

    n=LC_CardContext__FindFlags(node, driverType, readerType);
    if (n) {
      return n;
    }
    parent=GWEN_XMLNode_GetProperty(node, "extends", 0);
    if (!parent)
      break;
    node=LC_CardMgr_FindCardNode(ctx->mgr, parent);
    if (!node) {
      DBG_WARN(0, "Extended card \"%s\" not found", parent);
      break;
    }
    DBG_DEBUG(0, "Searching in parent \"%s\"", parent);
  } /* while */

  DBG_DEBUG(0, "Flags not found");
  return 0;
}



GWEN_TYPE_UINT32 LC_CardContext_GetReaderFlags(LC_CARD *card) {
  GWEN_XMLNODE *nflags;
  LC_CARDCONTEXT *ctx;
  LC_READER *r;
  LC_DRIVER *d;
  GWEN_TYPE_UINT32 flags;

  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);

  ctx=LC_Card_GetContext(card);
  if (!ctx) {
    DBG_INFO(0, "No context for this card, returning default flags");
    return LC_Reader_GetFlags(r);
  }


  nflags=LC_CardContext_FindFlags(ctx,
                                  LC_Driver_GetDriverName(d),
                                  LC_Reader_GetReaderType(r));
  if (!nflags) {
    DBG_INFO(0, "No flags for this card, returning default flags");
    return LC_Reader_GetFlags(r);
  }

  flags=LC_DriverInfo_ReaderFlagsFromXml(nflags, "flag");

  return flags;
}












