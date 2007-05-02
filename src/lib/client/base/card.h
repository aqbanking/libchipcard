/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.h 163 2006-02-15 19:31:45Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CARD_H
#define CHIPCARD_CLIENT_CARD_H


/** @addtogroup chipcardc_card_basic
 */
/*@{*/

#include <gwenhywfar/inherit.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LC_CARD LC_CARD;

#ifdef __cplusplus
}
#endif

#include <chipcard3/chipcard3.h>
#include <chipcard3/client/client.h>
#include <chipcard3/sharedstuff/pininfo.h>


#ifdef __cplusplus
extern "C" {
#endif

GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_CARD, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LC_CARD, LC_Card, CHIPCARD_API)

/** @name Flags for ISO Commands
 *
 */
/*@{*/
#define LC_CARD_ISO_FLAGS_EFID_MASK        0x00000001f
/** Mask for flags used with record based functions (like
 * @ref LC_Card_IsoReadRecord) */
#define LC_CARD_ISO_FLAGS_RECSEL_MASK      0x0000000e0
/** Select first record (used with record based functions like
 * @ref LC_Card_IsoReadRecord) */
#define   LC_CARD_ISO_FLAGS_RECSEL_FIRST   (0 << 5)
/** Select last record (used with record based functions like
 * @ref LC_Card_IsoReadRecord) */
#define   LC_CARD_ISO_FLAGS_RECSEL_LAST    (1 << 5)
/** Select next record (used with record based functions like
 * @ref LC_Card_IsoReadRecord) */
#define   LC_CARD_ISO_FLAGS_RECSEL_NEXT    (2 << 5)
/** Select previous record (used with record based functions like
 * @ref LC_Card_IsoReadRecord) */
#define   LC_CARD_ISO_FLAGS_RECSEL_PREV    (3 << 5)
/** Select given record (used with record based functions like
 * @ref LC_Card_IsoReadRecord) */
#define   LC_CARD_ISO_FLAGS_RECSEL_GIVEN   (4 << 5)
/*@}*/




/** @name Opening, Closing, Destroying, Executing
 *
 */
/*@{*/

/**
 * Release all ressources associated with the given card. You @b must call
 * this function in order to avoid memory leaks.
 */
CHIPCARD_API
void LC_Card_free(LC_CARD *cd);

CHIPCARD_API
void LC_Card_List2_freeAll(LC_CARD_LIST2 *l);


/**
 * Opens a card obtained via @ref LC_Client_GetNextCard.
 * The action taken here depends on the derived card class (e.g.
 * LC_DdvCard).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Open(LC_CARD *card);

/**
 * Closed the given card.
 * The action taken here depends on the derived card class (e.g.
 * LC_DdvCard).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Close(LC_CARD *card);



CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ExecApdu(LC_CARD *card,
                                  const char *apdu,
                                  unsigned int len,
                                  GWEN_BUFFER *rbuf,
                                  LC_CLIENT_CMDTARGET t,
                                  int timeout);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ExecCommand(LC_CARD *card,
                                     const char *commandName,
                                     GWEN_DB_NODE *cmdData,
                                     GWEN_DB_NODE *rspData,
                                     int timeout);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_BuildApdu(LC_CARD *card,
                                   const char *command,
                                   GWEN_DB_NODE *cmdData,
                                   GWEN_BUFFER *gbuf);

/*@}*/




/** @name Select Application/DF/EF
 *
 */
/*@{*/
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectApp(LC_CARD *card, const char *appName);

CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetAppNode(const LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectCard(LC_CARD *card, const char *s);

CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetCardNode(const LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectMF(LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectDf(LC_CARD *card, const char *fname);

CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetDfNode(const LC_CARD *card);


CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectEf(LC_CARD *card, const char *fname);

CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetEfNode(const LC_CARD *card);
/*@}*/





/** @name Informational Functions
 *
 */
/*@{*/

/**
 * The chipcard2 server identifies cards by ids which are unique in the
 * server environment. No two cards can have the same id.
 */
CHIPCARD_API
GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd);


CHIPCARD_API
const char *LC_Card_GetReaderType(const LC_CARD *cd);

CHIPCARD_API
const char *LC_Card_GetDriverType(const LC_CARD *cd);

/**
 * Returns the reader flags of the reader the given card is in (see
 * @ref LC_CARD_READERFLAGS_KEYPAD and others).
 */
CHIPCARD_API
GWEN_TYPE_UINT32 LC_Card_GetReaderFlags(const LC_CARD *cd);

/**
 * Returns the type of the given card. Possible values are "MEMORY" and
 * "PROCESSOR".
 */
CHIPCARD_API
const char *LC_Card_GetCardType(const LC_CARD *cd);

/**
 * Returns a stringlist containing all types which match the ATR string of
 * the given card. Possibly contents are "ddv0", "ddv1", "geldkarte" etc.
 */
CHIPCARD_API
const GWEN_STRINGLIST *LC_Card_GetCardTypes(const LC_CARD *cd);

/**
 * Returns the <i>Answer To Reset</i> string returned by the card upon
 * power up. These bytes indicate some properties of the card
 * (e.g. card type, manufacturer, memory size etc).
 * This function returns a pointer to the internally stored ATR. The card
 * object still remains the owner of the object returned (if any) so you
 * must neither manipulate nor free it.
 */
CHIPCARD_API
unsigned int LC_Card_GetAtr(const LC_CARD *cd, const unsigned char **pbuf);


/**
 * Returns the pointer to the client object this card belongs to.
 */
CHIPCARD_API
LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd);
/*@}*/



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

CHIPCARD_API
void LC_Card_CreateResultString(const LC_CARD *card,
                                const char *lastCommand,
                                LC_CLIENT_RESULT res,
                                GWEN_BUFFER *buf);
/*@}*/



/** @name Debugging Functions
 */
/*@{*/
CHIPCARD_API
void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent);
/*@}*/




/** @name Pin Functions
 *
 */
/*@{*/

/**
 * Returns a pininfo object of the pin given by its id.
 * The caller becomes the owner of the object returned (if any) and must
 * call @ref LC_PinInfo_free on it to avoid memory leaks.
 */
CHIPCARD_API
LC_PININFO *LC_Card_GetPinInfoById(LC_CARD *card, GWEN_TYPE_UINT32 pid);

/**
 * Returns a pininfo object of the pin given by its name.
 * The caller becomes the owner of the object returned (if any) and must
 * call @ref LC_PinInfo_free on it to avoid memory leaks.
 * Standard names are "ch_pin" for the cardholder pin and "eg_pin" for
 * the device pin (needed by STARCOS cards to modify security data on a card).
 */
CHIPCARD_API
LC_PININFO *LC_Card_GetPinInfoByName(LC_CARD *card, const char *name);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_GetPinStatus(LC_CARD *card,
                                      unsigned int pid,
                                      int *maxErrors,
                                      int *currentErrors);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_GetInitialPin(LC_CARD *card,
                                       int id,
                                       unsigned char *buffer,
                                       unsigned int maxLen,
                                       unsigned int *pinLength);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoVerifyPin(LC_CARD *card,
                                      GWEN_TYPE_UINT32 flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *ptr,
                                      unsigned int size,
                                      int *triesLeft);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoModifyPin(LC_CARD *card,
                                      GWEN_TYPE_UINT32 flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *oldptr,
                                      unsigned int oldsize,
                                      const unsigned char *newptr,
                                      unsigned int newsize,
                                      int *triesLeft);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoPerformVerification(LC_CARD *card,
                                                GWEN_TYPE_UINT32 flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_IsoPerformModification(LC_CARD *card,
                                                GWEN_TYPE_UINT32 flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft);

/*@}*/



/** @name Reading and Writing Data
 *
 */
/*@{*/
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

/**
 * This functions reads any number of bytes from an already selected file
 * of the card. It therefore issues multiple read requests until all bytes
 * are read.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
				    int offset,
				    int size,
				    GWEN_BUFFER *buf);

/*@}*/


/** @name Reading and Writing Records
 *
 */
/*@{*/

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
/*@}*/



/** @name Crypto Functions
 *
 */
/*@{*/

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


/*@}*/




/** @name Data Formats (Parsing and Generating)
 *
 */
/*@{*/

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ParseData(LC_CARD *card,
                                   const char *format,
                                   GWEN_BUFFER *buf,
                                   GWEN_DB_NODE *dbData);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_CreateData(LC_CARD *card,
                                    const char *format,
                                    GWEN_BUFFER *buf,
                                    GWEN_DB_NODE *dbData);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_ParseRecord(LC_CARD *card,
                                     int recNum,
                                     GWEN_BUFFER *buf,
                                     GWEN_DB_NODE *dbRecord);

CHIPCARD_API
LC_CLIENT_RESULT LC_Card_CreateRecord(LC_CARD *card,
                                      int recNum,
                                      GWEN_BUFFER *buf,
                                      GWEN_DB_NODE *dbRecord);


/*@}*/

#ifdef __cplusplus
}
#endif

/*@}*/ /* addtogroup */


#endif /* CHIPCARD_CLIENT_CARD_H */
