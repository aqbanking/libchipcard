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


#ifndef CHIPCARD_CARD_STARCOS_H
#define CHIPCARD_CARD_STARCOS_H

#include <chipcard2-client/client/card.h>
#include <gwenhywfar/keyspec.h>
#include <gwenhywfar/crypt.h>



int LC_Starcos_ExtendCard(LC_CARD *card);
int LC_Starcos_UnextendCard(LC_CARD *card);
LC_CLIENT_RESULT LC_Starcos_Reopen(LC_CARD *card);


GWEN_DB_NODE *LC_Starcos_GetCardDataAsDb(const LC_CARD *card);
GWEN_BUFFER *LC_Starcos_GetCardDataAsBuffer(const LC_CARD *card);



/** @name PIN Management Functions
 *
 */
/*@{*/
LC_CLIENT_RESULT LC_Starcos_GetPinStatus(LC_CARD *card,
                                         unsigned int pid,
                                         int *maxErrors,
                                         int *currentErrors);
LC_CLIENT_RESULT LC_Starcos_VerifyPin(LC_CARD *card,
                                      unsigned int pid,
                                      const char *pin);
LC_CLIENT_RESULT LC_Starcos_SecureVerifyPin(LC_CARD *card,
                                            unsigned int pid);


LC_CLIENT_RESULT LC_Starcos_VerifyInitialPin(LC_CARD *card,
                                             unsigned int pid);
LC_CLIENT_RESULT LC_Starcos_ModifyInitialPin(LC_CARD *card,
                                             unsigned int pid,
                                             const char *newpin);


LC_CLIENT_RESULT LC_Starcos_ModifyPin(LC_CARD *card,
                                      unsigned int pid,
                                      const char *oldpin,
                                      const char *newpin);

LC_CLIENT_RESULT LC_Starcos_SecureModifyPin(LC_CARD *card,
                                            unsigned int pid);
/*@}*/


/** @name Key Management Functions
 *
 */
/*@{*/
/**
 * Returns a GWEN_KEYSPEC for the given key.
 * The caller becomes the owner of the returned object (if any), so he is
 * responsible for freeing it (using @ref GWEN_KeySpec_free).
 */
GWEN_KEYSPEC *LC_Starcos_GetKeySpec(LC_CARD *card, int kid);

LC_CLIENT_RESULT LC_Starcos_SetKeySpec(LC_CARD *card,
                                       int kid,
                                       const GWEN_KEYSPEC *ks);


/**
 * Generates a temporary key pair. To use the newly created key pair you
 * need to call @ref LC_Starcos_ActivateKeyPair.
 * @param kid use 0x8e for crypt keys and 0x8f for sign keys
 */
LC_CLIENT_RESULT LC_Starcos_GenerateKeyPair(LC_CARD *card,
                                            int kid,
                                            int bits);

LC_CLIENT_RESULT LC_Starcos_ActivateKeyPair(LC_CARD *card,
                                            int srcKid,
                                            int dstKid,
                                            const GWEN_KEYSPEC *ks);

LC_CLIENT_RESULT LC_Starcos_WritePublicKey(LC_CARD *card, int kid,
                                           const GWEN_CRYPTKEY *key);

GWEN_CRYPTKEY *LC_Starcos_ReadPublicKey(LC_CARD *card, int kid);
/*@}*/



/** @name Bank Information Functions
 *
 */
/*@{*/
/**
 * This function reads institute data from the card. You need to verify
 * the pin prior to using this function.
 * Please note that this function always returns contexts if there was no
 * error. However, if the context is empty the corresponding context group
 * returned will be empty, too.
 * @param idx if 0 then all 5 entries are read, if 1 then entry 1 is read etc
 * @param dbData if idx==0 then every context read will be added as a new
 * group called "context" to this given group. Otherwise the specified context
 * is directly stored within this given group.
 *
 */
LC_CLIENT_RESULT LC_Starcos_ReadInstituteData(LC_CARD *card,
                                              int idx,
                                              GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Starcos_WriteInstituteData(LC_CARD *card,
                                               int idx,
                                               GWEN_DB_NODE *dbData);
/*@}*/



/** @name Cryptographic Functions
 *
 */
/*@{*/
GWEN_TYPE_UINT32 LC_Starcos_ReadSigCounter(LC_CARD *card, int kid);

LC_CLIENT_RESULT LC_Starcos_Sign(LC_CARD *card,
                                 int kid,
                                 GWEN_BUFFER *hashBuf,
                                 GWEN_BUFFER *sigBuf);
LC_CLIENT_RESULT LC_Starcos_Verify(LC_CARD *card,
                                   int kid,
                                   GWEN_BUFFER *hashBuf,
                                   GWEN_BUFFER *sigBuf);
LC_CLIENT_RESULT LC_Starcos_Encipher(LC_CARD *card,
                                     int kid,
                                     GWEN_BUFFER *plainBuf,
                                     GWEN_BUFFER *codeBuf);

LC_CLIENT_RESULT LC_Starcos_Decipher(LC_CARD *card,
                                     int kid,
                                     GWEN_BUFFER *codeBuf,
                                     GWEN_BUFFER *plainBuf);

/**
 * returns 8 random bytes
 */
LC_CLIENT_RESULT LC_Starcos_GetChallenge(LC_CARD *card, GWEN_BUFFER *mbuf);


/*@}*/

#endif /* CHIPCARD_CARD_STARCOS_H */


