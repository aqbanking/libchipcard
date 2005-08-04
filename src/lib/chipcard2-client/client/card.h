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


#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/client.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/list2.h>
#include <stdio.h>


GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_CARD, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LC_CARD, LC_Card, CHIPCARD_API)


typedef LC_CLIENT_RESULT (*LC_CARD_OPEN_FN)(LC_CARD *card);
typedef LC_CLIENT_RESULT (*LC_CARD_CLOSE_FN)(LC_CARD *card);

CHIPCARD_API
void LC_Card_List2_freeAll(LC_CARD_LIST2 *l);


CHIPCARD_API
void LC_Card_free(LC_CARD *cd);
CHIPCARD_API
GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd);
CHIPCARD_API
GWEN_TYPE_UINT32 LC_Card_GetReaderFlags(const LC_CARD *cd);

CHIPCARD_API
const char *LC_Card_GetCardType(const LC_CARD *cd);
CHIPCARD_API
const GWEN_STRINGLIST *LC_Card_GetCardTypes(const LC_CARD *cd);
CHIPCARD_API
GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd);


CHIPCARD_API
LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd);


CHIPCARD_API
void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent);


CHIPCARD_API
LC_CARD_OPEN_FN LC_Card_GetOpenFn(const LC_CARD *card);
CHIPCARD_API
void LC_Card_SetOpenFn(LC_CARD *card, LC_CARD_OPEN_FN fn);

CHIPCARD_API
LC_CARD_CLOSE_FN LC_Card_GetCloseFn(const LC_CARD *card);
CHIPCARD_API
void LC_Card_SetCloseFn(LC_CARD *card, LC_CARD_CLOSE_FN fn);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Open(LC_CARD *card);
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Close(LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Check(LC_CARD *card);
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Reset(LC_CARD *card);

CHIPCARD_API
int LC_Card_SelectApp(LC_CARD *card, const char *appName);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectCardAndApp(LC_CARD *card,
                                          const char *cardName,
                                          const char *appName);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ExecAPDU(LC_CARD *card,
                                  const char *apdu,
                                  unsigned int len,
                                  GWEN_BUFFER *rbuf,
                                  LC_CLIENT_CMDTARGET t,
                                  int timeout);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ExecCommand(LC_CARD *card,
                                     GWEN_DB_NODE *dbReq,
                                     GWEN_DB_NODE *dbResp,
                                     int timeout);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectMF(LC_CARD *card);


CHIPCARD_API
int LC_Card_GetRecordNumber(LC_CARD *card, const char *recName);

CHIPCARD_API
int LC_Card_ParseRecord(LC_CARD *card,
                        int recNum,
                        GWEN_BUFFER *buf,
                        GWEN_DB_NODE *dbRecord);

CHIPCARD_API
int LC_Card_CreateRecord(LC_CARD *card,
                         int recNum,
                         GWEN_BUFFER *buf,
                         GWEN_DB_NODE *dbRecord);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_GetDriverVar(LC_CARD *card,
                                      const char *varName,
                                      GWEN_BUFFER *vbuf);


CHIPCARD_API
int LC_Card_ParseData(LC_CARD *card,
                      const char *format,
                      GWEN_BUFFER *buf,
                      GWEN_DB_NODE *dbData);

CHIPCARD_API
int LC_Card_CreateData(LC_CARD *card,
                       const char *format,
                       GWEN_BUFFER *buf,
                       GWEN_DB_NODE *dbData);

/**
 * Returns a copy of the XML node describing the currently selected
 * card application (or 0 if none is selected).
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetAppInfo(const LC_CARD *card);

/**
 * Returns a copy of the XML node describing the currently selected EF.
 * The caller becomes the owner of the node returned (if any).
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetEfInfo(const LC_CARD *card);

/**
 * Returns a copy of the XML node describing the currently selected DF.
 * The caller becomes the owner of the node returned (if any).
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetDfInfo(const LC_CARD *card);


/** @name Last Result
 *
 * These functions return the result of the last command executed via
 * @ref LC_Card_ExecCommand (nearly all functions internally call that one)
 */
/*@{*/
CHIPCARD_API
int LC_Card_GetLastSW1(const LC_CARD *card);
CHIPCARD_API
int LC_Card_GetLastSW2(const LC_CARD *card);
CHIPCARD_API
const char *LC_Card_GetLastResult(const LC_CARD *card);
CHIPCARD_API
const char *LC_Card_GetLastText(const LC_CARD *card);
/*@}*/



CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
                                    int offset,
                                    int size,
                                    GWEN_BUFFER *buf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_WriteBinary(LC_CARD *card,
                                     int offset,
                                     GWEN_BUFFER *buf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_WriteBinary2(LC_CARD *card,
                                      int offset,
                                      const char *ptr,
                                      unsigned int size);





/* ISO commands */

#define LC_CARD_ISO_FLAGS_EFID_MASK        0x00000001f
#define LC_CARD_ISO_FLAGS_RECSEL_MASK      0x0000000e0
#define   LC_CARD_ISO_FLAGS_RECSEL_FIRST   (0 << 5)
#define   LC_CARD_ISO_FLAGS_RECSEL_LAST    (1 << 5)
#define   LC_CARD_ISO_FLAGS_RECSEL_NEXT    (2 << 5)
#define   LC_CARD_ISO_FLAGS_RECSEL_PREV    (3 << 5)
#define   LC_CARD_ISO_FLAGS_RECSEL_GIVEN   (4 << 5)


typedef
LC_CLIENT_RESULT (*LC_CARD_ISOREADBINARY_FN)(LC_CARD *card,
					     GWEN_TYPE_UINT32 flags,
					     int offset,
					     int size,
					     GWEN_BUFFER *buf);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOWRITEBINARY_FN)(LC_CARD *card,
					      GWEN_TYPE_UINT32 flags,
					      int offset,
					      const char *ptr,
					      unsigned int size);


typedef
LC_CLIENT_RESULT (*LC_CARD_ISOUPDATEBINARY_FN)(LC_CARD *card,
					       GWEN_TYPE_UINT32 flags,
					       int offset,
					       const char *ptr,
					       unsigned int size);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOERASEBINARY_FN)(LC_CARD *card,
					      GWEN_TYPE_UINT32 flags,
					      int offset,
					      unsigned int size);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOREADRECORD_FN)(LC_CARD *card,
					     GWEN_TYPE_UINT32 flags,
					     int recNum,
					     GWEN_BUFFER *buf);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOWRITERECORD_FN)(LC_CARD *card,
					      GWEN_TYPE_UINT32 flags,
					      int recNum,
					      const char *ptr,
					      unsigned int size);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOAPPENDRECORD_FN)(LC_CARD *card,
					       GWEN_TYPE_UINT32 flags,
					       const char *ptr,
					       unsigned int size);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOUPDATERECORD_FN)(LC_CARD *card,
					       GWEN_TYPE_UINT32 flags,
					       int recNum,
					       const char *ptr,
					       unsigned int size);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOVERIFYPIN_FN)(LC_CARD *card,
                                            GWEN_TYPE_UINT32 flags,
                                            int identifier,
                                            const char *ptr,
                                            unsigned int size,
                                            int *triesLeft);


typedef LC_CLIENT_RESULT (*LC_CARD_ISOMANAGESE_FN)(LC_CARD *card,
                                                   int tmpl,
                                                   int kids, int kidp,
                                                   int ar);

typedef LC_CLIENT_RESULT (*LC_CARD_ISOSIGN_FN)(LC_CARD *card,
                                               const char *ptr,
                                               unsigned int size,
                                               GWEN_BUFFER *sigBuf);

typedef LC_CLIENT_RESULT (*LC_CARD_ISOVERIFY_FN)(LC_CARD *card,
                                                 const char *dptr,
                                                 unsigned int dsize,
                                                 const char *sigptr,
                                                 unsigned int sigsize);
typedef LC_CLIENT_RESULT (*LC_CARD_ISOENCIPHER_FN)(LC_CARD *card,
                                                   const char *ptr,
                                                   unsigned int size,
                                                   GWEN_BUFFER *codeBuf);
typedef LC_CLIENT_RESULT (*LC_CARD_ISODECIPHER_FN)(LC_CARD *card,
                                                   const char *ptr,
                                                   unsigned int size,
                                                   GWEN_BUFFER *codeBuf);



void LC_Card_SetIsoReadBinaryFn(LC_CARD *card, LC_CARD_ISOREADBINARY_FN f);
void LC_Card_SetIsoWriteBinaryFn(LC_CARD *card, LC_CARD_ISOWRITEBINARY_FN f);
void LC_Card_SetIsoUpdateBinaryFn(LC_CARD *card, LC_CARD_ISOUPDATEBINARY_FN f);
void LC_Card_SetIsoEraseBinaryFn(LC_CARD *card, LC_CARD_ISOERASEBINARY_FN f);

void LC_Card_SetIsoReadRecordFn(LC_CARD *card, LC_CARD_ISOREADRECORD_FN f);
void LC_Card_SetIsoWriteRecordFn(LC_CARD *card, LC_CARD_ISOWRITERECORD_FN f);
void LC_Card_SetIsoUpdateRecordFn(LC_CARD *card, LC_CARD_ISOUPDATERECORD_FN f);
void LC_Card_SetIsoAppendRecordFn(LC_CARD *card, LC_CARD_ISOAPPENDRECORD_FN f);

void LC_Card_SetIsoVerifyPinFn(LC_CARD *card, LC_CARD_ISOVERIFYPIN_FN f);

void LC_Card_SetIsoManageSeFn(LC_CARD *card, LC_CARD_ISOMANAGESE_FN f);
void LC_Card_SetIsoSignFn(LC_CARD *card, LC_CARD_ISOSIGN_FN f);
void LC_Card_SetIsoVerifyFn(LC_CARD *card, LC_CARD_ISOVERIFY_FN f);
void LC_Card_SetIsoEncipherFn(LC_CARD *card, LC_CARD_ISOENCIPHER_FN f);
void LC_Card_SetIsoDecipherFn(LC_CARD *card, LC_CARD_ISODECIPHER_FN f);








CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoReadBinary(LC_CARD *card,
				       GWEN_TYPE_UINT32 flags,
				       int offset,
				       int size,
				       GWEN_BUFFER *buf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoWriteBinary(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int offset,
					const char *ptr,
					unsigned int size);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoUpdateBinary(LC_CARD *card,
					 GWEN_TYPE_UINT32 flags,
					 int offset,
					 const char *ptr,
					 unsigned int size);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoEraseBinary(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int offset,
					unsigned int size);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoReadRecord(LC_CARD *card,
				       GWEN_TYPE_UINT32 flags,
				       int recNum,
				       GWEN_BUFFER *buf);
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoWriteRecord(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int recNum,
					const char *ptr,
					unsigned int size);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoAppendRecord(LC_CARD *card,
					 GWEN_TYPE_UINT32 flags,
					 const char *ptr,
					 unsigned int size);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoUpdateRecord(LC_CARD *card,
					 GWEN_TYPE_UINT32 flags,
					 int recNum,
					 const char *ptr,
					 unsigned int size);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoVerifyPin(LC_CARD *card,
                                      GWEN_TYPE_UINT32 flags,
                                      int identifier,
                                      const char *ptr,
                                      unsigned int size,
                                      int *triesLeft);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoManageSe(LC_CARD *card,
                                     int tmpl, int kids, int kidp, int ar);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoEncipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *codeBuf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoDecipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *plainBuf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoSign(LC_CARD *card,
                                 const char *ptr,
                                 unsigned int size,
                                 GWEN_BUFFER *sigBuf);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoVerify(LC_CARD *card,
                                   const char *dptr,
                                   unsigned int dsize,
                                   const char *sigptr,
                                   unsigned int sigsize);


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CLIENT_CARD_H */






