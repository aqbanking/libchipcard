/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "card_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>



static GWEN_XMLNODE *_findCardFile(LC_CARD *card, const char *type, const char *fname);




GWEN_XMLNODE *LC_Card_FindFileById(LC_CARD *card, const char *type, const int sid)
{
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *currDF;
  int isSameLevel;
  char sidstr[10];

  currDF=card->dfNode;
  if (!currDF)
    currDF=card->appNode;

  isSameLevel=1;

  sprintf(sidstr, "%#x", sid);

  while (currDF) {
    n=GWEN_XMLNode_FindNode(currDF, GWEN_XMLNodeTypeTag, "files");
    if (n) {
      n=GWEN_XMLNode_FindFirstTag(n, type, "sid", sidstr);
      if (n) {
        if (isSameLevel)
          return n;
        if (GWEN_XMLNode_GetIntProperty(n, "inAnyDF", 0)!=0) {
          DBG_DEBUG(LC_LOGDOMAIN, "Returning file from level above");
          return n;
        }
      }
    }
    currDF=GWEN_XMLNode_GetParent(currDF);
    isSameLevel=0;
  }
  DBG_DEBUG(LC_LOGDOMAIN, "%s \"%s\" not found", type, sidstr);
  return 0;
}


int LC_Card_SelectMf(LC_CARD *card)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  int res;

  dbReq=GWEN_DB_Group_new("request");
  dbRsp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, "SelectMF", dbReq, dbRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_Group_free(dbReq);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  card->dfNode=0;

  return 0;
}



int LC_Card_SelectDf(LC_CARD *card, const char *fname)
{
  GWEN_XMLNODE *n;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  const char *cmd;
  int fid;
  int res;

  n=_findCardFile(card, "DF", fname);
  if (!n) {
    DBG_ERROR(LC_LOGDOMAIN, "DF \"%s\" not found", fname);
    return GWEN_ERROR_IO;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(n, "sid", "-1"), "%i", &fid)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad id for DF \"%s\"", fname);
    return GWEN_ERROR_IO;
  }

  dbReq=GWEN_DB_Group_new("request");
  if (fid==-1) {
    GWEN_BUFFER *buf;
    const char *lid;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    lid=GWEN_XMLNode_GetProperty(n, "lid", 0);
    if (!lid) {
      DBG_ERROR(LC_LOGDOMAIN, "No long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_IO;
    }
    if (GWEN_Text_FromHexBuffer(lid, buf)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_IO;
    }

    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId",
                        GWEN_Buffer_GetStart(buf),
                        GWEN_Buffer_GetUsedBytes(buf));
    cmd="SelectDFL";

  }
  else {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "fileId", fid);
    cmd="SelectDFS";
  }

  dbRsp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, cmd, dbReq, dbRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_Group_free(dbReq);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  card->dfNode=n;
  card->efNode=NULL;

  return 0;
}



GWEN_XMLNODE *LC_Card_GetDfNode(const LC_CARD *card)
{
  assert(card);
  return card->dfNode;
}



int LC_Card_SelectEf(LC_CARD *card, const char *fname)
{
  GWEN_XMLNODE *n;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  const char *cmd;
  int fid;
  int res;

  n=_findCardFile(card, "EF", fname);
  if (!n) {
    DBG_ERROR(LC_LOGDOMAIN, "EF \"%s\" not found", fname);
    return GWEN_ERROR_IO;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(n, "sid", "-1"), "%i", &fid)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad id for DF \"%s\"", fname);
    return GWEN_ERROR_IO;
  }

  dbReq=GWEN_DB_Group_new("request");
  if (fid==-1) {
    GWEN_BUFFER *buf;
    const char *lid;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    lid=GWEN_XMLNode_GetProperty(n, "lid", 0);
    if (!lid) {
      DBG_ERROR(LC_LOGDOMAIN, "No long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_BAD_DATA;
    }
    if (GWEN_Text_FromHexBuffer(lid, buf)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_BAD_DATA;
    }

    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId",
                        GWEN_Buffer_GetStart(buf),
                        GWEN_Buffer_GetUsedBytes(buf));
    cmd="SelectEFL";

  }
  else {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "fileId", fid);
    cmd="SelectEFS";
  }

  dbRsp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, cmd, dbReq, dbRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_Group_free(dbReq);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  card->efNode=n;

  return 0;
}



int LC_Card_SelectEfById(LC_CARD *card, const int  sid)
{
  GWEN_XMLNODE *n;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  const char *cmd;
  int fid;
  int res;

  n=LC_Card_FindFileById(card, "EF", sid);
  if (!n) {
    DBG_ERROR(LC_LOGDOMAIN, "EF \"%d\" not found", sid);
    return GWEN_ERROR_IO;
  }

  if (1!=sscanf(GWEN_XMLNode_GetProperty(n, "sid", "-1"), "%i", &fid)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad id for DF \"%d\"", sid);
    return GWEN_ERROR_IO;
  }

  dbReq=GWEN_DB_Group_new("request");
  if (fid==-1) {
    GWEN_BUFFER *buf;
    const char *lid;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    lid=GWEN_XMLNode_GetProperty(n, "lid", 0);
    if (!lid) {
      DBG_ERROR(LC_LOGDOMAIN, "No long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_BAD_DATA;
    }
    if (GWEN_Text_FromHexBuffer(lid, buf)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad long id given in XML file");
      GWEN_Buffer_free(buf);
      GWEN_DB_Group_free(dbReq);
      return GWEN_ERROR_BAD_DATA;
    }

    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "fileId",
                        GWEN_Buffer_GetStart(buf),
                        GWEN_Buffer_GetUsedBytes(buf));
    cmd="SelectEFL";

  }
  else {
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "fileId", fid);
    cmd="SelectEFS";
  }

  dbRsp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, cmd, dbReq, dbRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_Group_free(dbReq);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  card->efNode=n;

  return 0;
}



int LC_Card_SelectEfByID(LC_CARD *card, const int fid)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  const char *cmd;
  int res;


  dbReq=GWEN_DB_Group_new("request");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "fileId", fid);
  cmd="SelectEFS";


  dbRsp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, cmd, dbReq, dbRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_Group_free(dbReq);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }

  return 0;
}



GWEN_XMLNODE *LC_Card_GetEfNode(const LC_CARD *card)
{
  assert(card);
  return card->efNode;
}



GWEN_XMLNODE *_findCardFile(LC_CARD *card, const char *type, const char *fname)
{
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *currDF;
  int isSameLevel;

  currDF=card->dfNode;
  if (!currDF)
    currDF=card->appNode;

  isSameLevel=1;
  while (currDF) {
    n=GWEN_XMLNode_FindNode(currDF, GWEN_XMLNodeTypeTag, "files");
    if (n) {
      n=GWEN_XMLNode_FindFirstTag(n, type, "name", fname);
      if (n) {
        if (isSameLevel)
          return n;
        if (GWEN_XMLNode_GetIntProperty(n, "inAnyDF", 0)!=0) {
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

