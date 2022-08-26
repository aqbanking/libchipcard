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
#include "client_l.h"

#include <gwenhywfar/debug.h>



int LC_Card_ParseData(LC_CARD *card,
                      const char *format,
                      GWEN_BUFFER *buf,
                      GWEN_DB_NODE *dbData)
{
  GWEN_XMLNODE *dataNode;
  GWEN_MSGENGINE *e;

  /* find format node */
  assert(card->appNode);
  e=LC_Client_GetMsgEngine(card->client);
  assert(e);
  if (!GWEN_Buffer_GetBytesLeft(buf)) {
    DBG_ERROR(LC_LOGDOMAIN, "End of buffer reached");
    return GWEN_ERROR_NO_DATA;
  }
  dataNode=GWEN_XMLNode_FindFirstTag(card->appNode, "formats", 0, 0);
  if (dataNode==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No formats for this card application");
    return GWEN_ERROR_NOT_FOUND;
  }

  dataNode=GWEN_XMLNode_FindFirstTag(dataNode, "format", "name", format);
  if (!dataNode) {
    DBG_ERROR(LC_LOGDOMAIN, "Format \"%s\" not found", format);
    return GWEN_ERROR_NOT_FOUND;
  }

  /* node found, parse data */
  DBG_DEBUG(LC_LOGDOMAIN, "Parsing data");
  if (GWEN_MsgEngine_ParseMessage(e,
                                  dataNode,
                                  buf,
                                  dbData,
                                  GWEN_MSGENGINE_READ_FLAGS_DEFAULT)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error parsing data in format \"%s\"", format);
    return GWEN_ERROR_BAD_DATA;
  }

  return 0;
}



int LC_Card_CreateData(LC_CARD *card,
                       const char *format,
                       GWEN_BUFFER *buf,
                       GWEN_DB_NODE *dbData)
{
  GWEN_XMLNODE *dataNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(card->appNode);
  e=LC_Client_GetMsgEngine(card->client);
  assert(e);

  dataNode=GWEN_XMLNode_FindFirstTag(card->appNode, "formats", 0, 0);
  if (dataNode==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No formats for this card application");
    return GWEN_ERROR_NO_DATA;
  }

  dataNode=GWEN_XMLNode_FindFirstTag(dataNode, "format", "name", format);
  if (!dataNode) {
    DBG_ERROR(LC_LOGDOMAIN, "Format \"%s\" not found", format);
    return GWEN_ERROR_NO_DATA;
  }

  /* node found, parse data */
  DBG_DEBUG(LC_LOGDOMAIN, "Creating data");
  if (GWEN_MsgEngine_CreateMessageFromNode(e,
                                           dataNode,
                                           buf,
                                           dbData)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error creating data for format \"%s\"", format);
    return GWEN_ERROR_BAD_DATA;
  }

  return 0;
}



int LC_Card_ParseRecord(LC_CARD *card,
                        int recNum,
                        GWEN_BUFFER *buf,
                        GWEN_DB_NODE *dbRecord)
{
  GWEN_XMLNODE *recordNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(card->efNode);
  e=LC_Client_GetMsgEngine(card->client);
  assert(e);
  if (!GWEN_Buffer_GetBytesLeft(buf)) {
    DBG_ERROR(LC_LOGDOMAIN, "End of buffer reached");
    return GWEN_ERROR_NO_DATA;
  }
  recordNode=GWEN_XMLNode_FindFirstTag(card->efNode, "record", 0, 0);
  while (recordNode) {
    int lrecNum;

    if (1==sscanf(GWEN_XMLNode_GetProperty(recordNode, "recnum", "-1"), "%i", &lrecNum)) {
      if (lrecNum!=-1 && recNum==lrecNum)
        break;
    }
    recordNode=GWEN_XMLNode_FindNextTag(recordNode, "record", 0, 0);
  } /* while */
  if (!recordNode)
    recordNode=GWEN_XMLNode_FindFirstTag(card->efNode, "record", 0, 0);

  if (recordNode) {
    /* node found, parse data */
    DBG_DEBUG(LC_LOGDOMAIN, "Parsing record data");
    if (GWEN_MsgEngine_ParseMessage(e,
                                    recordNode,
                                    buf,
                                    dbRecord,
                                    GWEN_MSGENGINE_READ_FLAGS_DEFAULT)) {
      DBG_ERROR(LC_LOGDOMAIN, "Error parsing response");
      return GWEN_ERROR_BAD_DATA;
    }
  } /* if record found */
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Record not found");
    return GWEN_ERROR_NOT_FOUND;
  }
  return 0;
}



int LC_Card_CreateRecord(LC_CARD *card,
                         int recNum,
                         GWEN_BUFFER *buf,
                         GWEN_DB_NODE *dbRecord)
{
  GWEN_XMLNODE *recordNode;
  GWEN_MSGENGINE *e;

  /* find record node */
  assert(card->efNode);
  e=LC_Client_GetMsgEngine(card->client);
  assert(e);
  recordNode=GWEN_XMLNode_FindFirstTag(card->efNode, "record", 0, 0);
  while (recordNode) {
    int lrecNum;

    if (1==sscanf(GWEN_XMLNode_GetProperty(recordNode, "recnum", "-1"), "%i", &lrecNum)) {
      if (lrecNum!=-1 && recNum==lrecNum)
        break;
    }
    recordNode=GWEN_XMLNode_FindNextTag(recordNode, "record", 0, 0);
  } /* while */
  if (!recordNode)
    recordNode=GWEN_XMLNode_FindFirstTag(card->efNode, "record", 0, 0);

  if (recordNode) {
    /* node found, parse data */
    DBG_DEBUG(LC_LOGDOMAIN, "Creating record data");
    if (GWEN_MsgEngine_CreateMessageFromNode(e, recordNode, buf, dbRecord)) {
      DBG_ERROR(LC_LOGDOMAIN, "Error creating record");
      return GWEN_ERROR_BAD_DATA;
    }
  } /* if record found */
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Record not found");
    return GWEN_ERROR_NOT_FOUND;
  }
  return 0;
}



