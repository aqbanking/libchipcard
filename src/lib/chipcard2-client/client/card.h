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


#ifndef CHIPCARD_CLIENT_CARD_H
#define CHIPCARD_CLIENT_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LC_CARD LC_CARD;


#define LC_CARD_READERFLAGS_KEYPAD  0x00010000
#define LC_CARD_READERFLAGS_DISPLAY 0x00020000
#define LC_CARD_READERFLAGS_NOINFO  0x00040000
#define LC_CARD_READERFLAGS_REMOTE  0x00080000
#define LC_CARD_READERFLAGS_AUTO    0x00100000


#include <chipcard2-client/client/client.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/xml.h>
#include <stdio.h>


GWEN_INHERIT_FUNCTION_DEFS(LC_CARD)


typedef LC_CLIENT_RESULT (*LC_CARD_OPEN_FN)(LC_CARD *card);
typedef LC_CLIENT_RESULT (*LC_CARD_CLOSE_FN)(LC_CARD *card);


void LC_Card_free(LC_CARD *cd);
GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd);
GWEN_TYPE_UINT32 LC_Card_GetReaderFlags(const LC_CARD *cd);

const char *LC_Card_GetCardType(const LC_CARD *cd);
GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd);


LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd);


void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent);


LC_CARD_OPEN_FN LC_Card_GetOpenFn(const LC_CARD *card);
void LC_Card_SetOpenFn(LC_CARD *card, LC_CARD_OPEN_FN fn);

LC_CARD_CLOSE_FN LC_Card_GetCloseFn(const LC_CARD *card);
void LC_Card_SetCloseFn(LC_CARD *card, LC_CARD_CLOSE_FN fn);


LC_CLIENT_RESULT LC_Card_Open(LC_CARD *card);
LC_CLIENT_RESULT LC_Card_Close(LC_CARD *card);

LC_CLIENT_RESULT LC_Card_SelectCardAndApp(LC_CARD *card,
                                          const char *cardName,
                                          const char *appName);


LC_CLIENT_RESULT LC_Card_ExecAPDU(LC_CARD *card,
                                  const char *apdu,
                                  unsigned int len,
                                  GWEN_BUFFER *rbuf,
                                  LC_CLIENT_CMDTARGET t,
                                  int timeout);

LC_CLIENT_RESULT LC_Card_ExecCommand(LC_CARD *card,
                                     GWEN_DB_NODE *dbReq,
                                     GWEN_DB_NODE *dbResp,
                                     int timeout);

LC_CLIENT_RESULT LC_Card_SelectMF(LC_CARD *card);


int LC_Card_GetRecordNumber(LC_CARD *card, const char *recName);

int LC_Card_ParseRecord(LC_CARD *card,
                        int recNum,
                        GWEN_BUFFER *buf,
                        GWEN_DB_NODE *dbRecord);

int LC_Card_CreateRecord(LC_CARD *card,
                         int recNum,
                         GWEN_BUFFER *buf,
                         GWEN_DB_NODE *dbRecord);

LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
                                    int offset,
                                    int size,
                                    GWEN_BUFFER *buf);

LC_CLIENT_RESULT LC_Card_WriteBinary(LC_CARD *card,
                                     int offset,
                                     GWEN_BUFFER *buf);

LC_CLIENT_RESULT LC_Card_WriteBinary2(LC_CARD *card,
                                      int offset,
                                      const char *ptr,
                                      unsigned int size);

LC_CLIENT_RESULT LC_Card_GetDriverVar(LC_CARD *card,
                                      const char *varName,
                                      GWEN_BUFFER *vbuf);


int LC_Card_ParseData(LC_CARD *card,
                      const char *format,
                      GWEN_BUFFER *buf,
                      GWEN_DB_NODE *dbData);

int LC_Card_CreateData(LC_CARD *card,
                       const char *format,
                       GWEN_BUFFER *buf,
                       GWEN_DB_NODE *dbData);

/**
 * Returns a copy of the XML node describing the currently selected EF.
 * The caller becomes the owner of the node returned (if any).
 */
GWEN_XMLNODE *LC_Card_GetEfInfo(const LC_CARD *card);

/**
 * Returns a copy of the XML node describing the currently selected DF.
 * The caller becomes the owner of the node returned (if any).
 */
GWEN_XMLNODE *LC_Card_GetDfInfo(const LC_CARD *card);


/** @name Last Result
 *
 * These functions return the result of the last command executed via
 * @ref LC_Card_ExecCommand (nearly all functions internally call that one)
 */
/*@{*/
int LC_Card_GetLastSW1(const LC_CARD *card);
int LC_Card_GetLastSW2(const LC_CARD *card);
const char *LC_Card_GetLastResult(const LC_CARD *card);
const char *LC_Card_GetLastText(const LC_CARD *card);
/*@}*/



#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CLIENT_CARD_H */






