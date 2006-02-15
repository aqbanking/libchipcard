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


/** @addtogroup chipcardc_card_basic
 * @short Functions available with all chipcards
 *
 * <p>
 * This group contains functions which are available with all chipcards.
 * Some of the functions are virtual, i.e. they are implemented in inheriting
 * classes.
 * </p>
 *
 * <p>
 * You obtain a chip card handle (@ref LC_CARD) via one of the following
 * functions:
 * </p>
 * <ul>
 *  <li>
 *   @ref LC_Client_WaitForNextCard (wait for the next available card)
 *  </li>
 *  <li>
 *    @ref LC_Client_GetNextCard (get next available chipcard without waiting)
 *  </li>
 * </ul>
 * <p>
 * Those functions only provide a handle to a chipcard. In order to use it
 * you must call the function @ref LC_Card_Open.
 * </p>
 * <p>
 * However, you will most likely want to use one of the derived class rather
 * than this base class. The following is an easy example of how you use
 * a card as a German health insureance card.
 * </p>
 *
 * @code
 * LC_CLIENT *cl;
 * LC_CARD *card=0;
 * LC_CLIENT_RESULT res;
 *
 * cl=LC_Client_new("MyApplication", "1.0", 0);
 * LC_Client_ReadConfigFile(cl, 0);
 *
 * LC_Client_StartWait(cl, 0, 0);
 * card=LC_Client_WaitForNextCard(cl, 30);
 * LC_KVKCard_ExtendCard(card);
 * LC_Card_Open(card);
 * LC_Client_StopWait(cl);
 * ...
 * [do something with the card]
 * ...
 * LC_Card_Close(card);
 * LC_Card_free(card);
 * ...
 * @endcode
 *
 * <p>
 * As you see in this example you need to extend a card
 * (@ref LC_KVKCard_ExtendCard before in our example) before calling
 * @ref LC_Card_Open.
 * </p>
 *
 * <p>
 * Please note that you have to call @ref LC_Card_free as soon as you are
 * finished dealing with the card to avoid memory leaks.
 * </p>
 */
/*@{*/

/**
 * This is the definition of a LC_CARD object. You can only access members
 * of this struct via functions of this group. You should treat this type
 * as opaque without making assumptions about the members.
 * Libchipcard2 only uses pointers to hide the real structure. This helps us
 * keeping newer versions binary compatible with older ones.
 */
typedef struct LC_CARD LC_CARD;

#include <chipcard2/chipcard2.h>

#include <gwenhywfar/buffer.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/list2.h>
#include <stdio.h>

GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_CARD, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LC_CARD, LC_Card, CHIPCARD_API)



/** @name Reader Flags
 *
 * These flags indicate some infos about the reader a given card is in.
 */
/*@{*/
/** Reader has a keypad */
#define LC_CARD_READERFLAGS_KEYPAD  LC_READER_FLAGS_KEYPAD
/** Reader has a display */
#define LC_CARD_READERFLAGS_DISPLAY LC_READER_FLAGS_DISPLAY
/** Reader/Driver does not support the special info APDU */
#define LC_CARD_READERFLAGS_NOINFO  LC_READER_FLAGS_NOINFO
/** Reader is remote (i.e. not directly connected to the server) */
#define LC_CARD_READERFLAGS_REMOTE  LC_READER_FLAGS_REMOTE
/** Reader has been autodetected and autoconfigured */
#define LC_CARD_READERFLAGS_AUTO    LC_READER_FLAGS_AUTO
/*@}*/


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


#include <chipcard2-client/client/client.h>
#include <chipcard2/sharedstuff/pininfo.h>



/** @name Prototypes of Virtual Functions
 *
 */
/*@{*/
typedef LC_CLIENT_RESULT (*LC_CARD_OPEN_FN)(LC_CARD *card);
typedef LC_CLIENT_RESULT (*LC_CARD_CLOSE_FN)(LC_CARD *card);

typedef
LC_CLIENT_RESULT (*LC_CARD_GETPINSTATUS_FN)(LC_CARD *card,
                                            unsigned int pid,
                                            int *maxErrors,
                                            int *currentErrors);

typedef
LC_CLIENT_RESULT (*LC_CARD_GETINITIALPIN_FN)(LC_CARD *card,
                                             int id,
                                             unsigned char *buffer,
                                             unsigned int maxLen,
                                             unsigned int *pinLength);


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
                                            const LC_PININFO *pi,
                                            const unsigned char *ptr,
                                            unsigned int size,
                                            int *triesLeft);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOMODIFYPIN_FN)(LC_CARD *card,
                                            GWEN_TYPE_UINT32 flags,
                                            const LC_PININFO *pi,
                                            const unsigned char *oldptr,
                                            unsigned int oldsize,
                                            const unsigned char *newptr,
                                            unsigned int newsize,
                                            int *triesLeft);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOPERFORMVERIFICATION_FN)(LC_CARD *card,
                                                      GWEN_TYPE_UINT32 flags,
                                                      const LC_PININFO *pi,
                                                      int *triesLeft);

typedef
LC_CLIENT_RESULT (*LC_CARD_ISOPERFORMMODIFICATION_FN)(LC_CARD *card,
                                                      GWEN_TYPE_UINT32 flags,
                                                      const LC_PININFO *pi,
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

/*@}*/


/** @name Setters for Virtual Functions
 *
 * Functions in this group set or get pointers for virtual functions and
 * will only be used by inheriting classes.
 */
/*@{*/

CHIPCARD_API
LC_CARD_OPEN_FN LC_Card_GetOpenFn(const LC_CARD *card);

CHIPCARD_API
void LC_Card_SetOpenFn(LC_CARD *card, LC_CARD_OPEN_FN fn);

CHIPCARD_API
LC_CARD_CLOSE_FN LC_Card_GetCloseFn(const LC_CARD *card);

CHIPCARD_API
void LC_Card_SetCloseFn(LC_CARD *card, LC_CARD_CLOSE_FN fn);

CHIPCARD_API
void LC_Card_SetGetInitialPinFn(LC_CARD *card, LC_CARD_GETINITIALPIN_FN fn);

CHIPCARD_API
void LC_Card_SetGetPinStatusFn(LC_CARD *card, LC_CARD_GETPINSTATUS_FN fn);

CHIPCARD_API
void LC_Card_SetIsoReadBinaryFn(LC_CARD *card, LC_CARD_ISOREADBINARY_FN f);

CHIPCARD_API
void LC_Card_SetIsoWriteBinaryFn(LC_CARD *card, LC_CARD_ISOWRITEBINARY_FN f);

CHIPCARD_API
void LC_Card_SetIsoUpdateBinaryFn(LC_CARD *card, LC_CARD_ISOUPDATEBINARY_FN f);

CHIPCARD_API
void LC_Card_SetIsoEraseBinaryFn(LC_CARD *card, LC_CARD_ISOERASEBINARY_FN f);

CHIPCARD_API
void LC_Card_SetIsoReadRecordFn(LC_CARD *card, LC_CARD_ISOREADRECORD_FN f);

CHIPCARD_API
void LC_Card_SetIsoWriteRecordFn(LC_CARD *card, LC_CARD_ISOWRITERECORD_FN f);

CHIPCARD_API
void LC_Card_SetIsoUpdateRecordFn(LC_CARD *card, LC_CARD_ISOUPDATERECORD_FN f);

CHIPCARD_API
void LC_Card_SetIsoAppendRecordFn(LC_CARD *card, LC_CARD_ISOAPPENDRECORD_FN f);

CHIPCARD_API
void LC_Card_SetIsoVerifyPinFn(LC_CARD *card, LC_CARD_ISOVERIFYPIN_FN f);

CHIPCARD_API
void LC_Card_SetIsoModifyPinFn(LC_CARD *card, LC_CARD_ISOMODIFYPIN_FN f);

CHIPCARD_API
void LC_Card_SetIsoPerformVerificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMVERIFICATION_FN f);

CHIPCARD_API
void LC_Card_SetIsoPerformModificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMMODIFICATION_FN f);

CHIPCARD_API
void LC_Card_SetIsoManageSeFn(LC_CARD *card, LC_CARD_ISOMANAGESE_FN f);

CHIPCARD_API
void LC_Card_SetIsoSignFn(LC_CARD *card, LC_CARD_ISOSIGN_FN f);

CHIPCARD_API
void LC_Card_SetIsoVerifyFn(LC_CARD *card, LC_CARD_ISOVERIFY_FN f);

CHIPCARD_API
void LC_Card_SetIsoEncipherFn(LC_CARD *card, LC_CARD_ISOENCIPHER_FN f);

CHIPCARD_API
void LC_Card_SetIsoDecipherFn(LC_CARD *card, LC_CARD_ISODECIPHER_FN f);
/*@}*/



/** @name Opening, Closing, Checking, Destroying
 *
 */
/*@{*/
/**
 * Opens a card obtained via @ref LC_Client_WaitForNextCard or
 * @ref LC_Client_GetNextCard.
 * It locks the card at the server for the calling client.
 * After that it calls the virtual function set by @ref LC_Card_SetOpenFn.
 * This function fails if the card has already been taken by another
 * application.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Open(LC_CARD *card);

/**
 * Closed the given card.
 * It releases the card at the server from the calling client thus making
 * it available to other clients.
 * Before that it calls the virtual function set by @ref LC_Card_SetCloseFn.
 * Upon receiption of such a request the server resets the chipcard to make
 * sure that the next application is unable to take over an existing security
 * status on the card. The server does this also when a client crashes.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Close(LC_CARD *card);


/**
 * Release all ressources associated with the given card. You @b must call
 * this function in order to avoid memory leaks.
 */
CHIPCARD_API
void LC_Card_free(LC_CARD *cd);

CHIPCARD_API
void LC_Card_List2_freeAll(LC_CARD_LIST2 *l);
/*@}*/


/**
 * Checks whether the given card is still inserted and available to the
 * calling client.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Check(LC_CARD *card);

/**
 * Reset the given card (thus making sure that a possible security context
 * is reset on the card).
 * The server resets a card whenever a client disconnects from it (by
 * calling @ref LC_Card_Close or by crashing).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_Reset(LC_CARD *card);

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
GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd);


/**
 * Returns the pointer to the client object this card belongs to.
 */
CHIPCARD_API
LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd);
/*@}*/



/** @name Selecting Application/Card/DF/EF
 *
 * <p>
 * This function is most likely called by inheriting classes, not by
 * applications.
 * </p>
 * <p>
 * Chip cards contain applications which provide special functionality (e.g.
 * the <i>GeldKarte</i> application or an application which supports RSA
 * encryptio etc).
 * </p>
 * <p>
 * Libchipcard2 uses application description files which describe
 * what kind of data can be found on the card and how it is formatted.
 * </p>
 * <p>
 * One card may contain multiple applications, e.g. German HBCI cards
 * contain the DDV application and the GeldKarte application. Therefore
 * Libchipcard2 must be told which application is to be used in subsequent
 * function calls.
 * </p>
 * <p>
 * The Libchipcard2 server additionally holds card decription files which
 * describe what commands are available with which cards and how these
 * commands must look like.
 * </p>
 * <p>
 * So the first thing an inheriting class will do is to select the proper
 * card type and application. The class LC_DDVCard for example uses the
 * following function:
 * </p>
 * <code>
 * LC_Card_SelectCardAndApp(card, "ddv0", "ddv");
 * </code>
 * for DDV0 cards and
 * <code>
 * LC_Card_SelectCardAndApp(card, "ddv1", "ddv");
 * </code>
 * for DDV1 cards.
 *
 * <p>
 * As described above normally applications do not have to bother selecting
 * a card or application because this is done by inheriting classes upon
 * LC_Card_Open (like in the open function set by @ref LC_DDVCard_ExtendCard).
 * </p>
 */
/*@{*/
/**
 * Select the application. This only effects the client part of Libchipcard2.
 * @param card card object
 * @param appName application name (e.g. "ddv", "kvk")
 */
CHIPCARD_API
int LC_Card_SelectApp(LC_CARD *card, const char *appName);

/**
 * Return the name of the currently selected application (if any).
 * @param card card object
 */
CHIPCARD_API
const char *LC_Card_GetSelectedApp(const LC_CARD *card);

/**
 * Select the card type and application for the given card object.
 * @param card card object
 * @param cardName card type name (e.g. "ddv1")
 * @param appName application name (e.g. "ddv", "kvk")
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectCardAndApp(LC_CARD *card,
                                          const char *cardName,
                                          const char *appName);

/**
 * Select the Master File of the card. The MAster File is similiar to the
 * root of a file system.
 * For processor cards there are also some functions which select individual
 * folders (DF) or files (EF) on a chip card (see
 * @ref LC_ProcessorCard_SelectDF and @ref LC_ProcessorCard_SelectEF)
 * @param card card object
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Card_SelectMF(LC_CARD *card);
/*@}*/


/** @name Parsing of Structures/Records
 *
 * Functions of this group use data from the application description file
 * which is selected upon @ref LC_Card_SelectApp or
 * @ref LC_Card_SelectCardAndApp. This file contains information about which
 * folders and files are available with the given card application.
 */
/*@{*/
/**
 * The application description file allows to assign names to records of
 * files on the card.
 * This function returns the number of the record of the currently selected EF
 * (see @ref LC_ProcessorCard_SelectEF) identified by the given record name.
 */
CHIPCARD_API
int LC_Card_GetRecordNumber(LC_CARD *card, const char *recName);

/**
 * This function parses the given record of the currently selected EF
 * (see @ref LC_ProcessorCard_SelectEF) and stores the parsed data in the
 * given DB.
 */
CHIPCARD_API
int LC_Card_ParseRecord(LC_CARD *card,
                        int recNum,
                        GWEN_BUFFER *buf,
                        GWEN_DB_NODE *dbRecord);

/**
 * This function creates data for the given record of the currently selected
 * EF (see @ref LC_ProcessorCard_SelectEF) and stores the resulting data
 * in the given buffer.
 */
CHIPCARD_API
int LC_Card_CreateRecord(LC_CARD *card,
                         int recNum,
                         GWEN_BUFFER *buf,
                         GWEN_DB_NODE *dbRecord);

/**
 * The application description file may contain some descriptions of formats
 * used with the acard application. These formats are identified using names.
 * This function parses the given data and stores the parsed data in the
 * given DB.
 * @param card card object
 * @param format name of the format
 * @param buf buffer containing data to parse
 * @param dbData GWEN_DB to receive the parsed data
 */
CHIPCARD_API
int LC_Card_ParseData(LC_CARD *card,
                      const char *format,
                      GWEN_BUFFER *buf,
                      GWEN_DB_NODE *dbData);

/**
 * This function creates data for the given according to the card applications
 * format identiefied by name and stores the resulting data in the given
 * buffer.
 * @param card card object
 * @param format name of the format
 * @param buf destination buffer for the resulting data
 * @param dbData GWEN_DB containing the data
 */
CHIPCARD_API
int LC_Card_CreateData(LC_CARD *card,
                       const char *format,
                       GWEN_BUFFER *buf,
                       GWEN_DB_NODE *dbData);

/*@}*/



/** @name Application/DF/EF Info
 *
 */
/*@{*/
/**
 * Returns a copy of the XML node describing the currently selected
 * card application (see @ref LC_Card_SelectApp)
 * The caller becomes the owner of the node returned (if any).
 * @return XML node (or 0 if no application is selected).
 * @param card card object
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetAppInfo(const LC_CARD *card);

/**
 * Returns a copy of the XML node describing the currently selected EF
 * (see @ref LC_ProcessorCard_SelectEF).
 * The caller becomes the owner of the node returned (if any).
 * @param card card object
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetEfInfo(const LC_CARD *card);

/**
 * Returns a copy of the XML node describing the currently selected DF
 * (see @ref LC_ProcessorCard_SelectDF).
 * The caller becomes the owner of the node returned (if any).
 * @param card card object
 */
CHIPCARD_API
GWEN_XMLNODE *LC_Card_GetDfInfo(const LC_CARD *card);


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



/** @name Command Execution
 */
/*@{*/
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
/*@}*/


/** @name Debugging Functions
 */
/*@{*/
CHIPCARD_API
void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent);
/*@}*/




/** @name Deprecated Functions
 */
/*@{*/

/** @deprecated
 * use LC_Card_IsoReadBinary(card, 0, offset, size, buf) instead
 */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
                                    int offset,
                                    int size,
                                    GWEN_BUFFER *buf);

/** @deprecated */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_Card_WriteBinary(LC_CARD *card,
                                     int offset,
                                     GWEN_BUFFER *buf);

/** @deprecated */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_Card_WriteBinary2(LC_CARD *card,
                                      int offset,
                                      const char *ptr,
                                      unsigned int size);

/** @deprecated */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_Card_GetDriverVar(LC_CARD *card,
                                      const char *varName,
                                      GWEN_BUFFER *vbuf);

/*@}*/


/*@}*/ /* addtogroup */


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CLIENT_CARD_H */



