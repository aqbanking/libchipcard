/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef LC_CT_CARD_H
#define LC_CT_CARD_H

#include <libchipcard/base/card.h>
#include <libchipcard/base/pininfo.h>

#include <gwenhywfar/ct.h>


/** @addtogroup chipcardc_client_ct
 * @short Chipcard-based CryptTokens
 *
 * Libchipcard provides CryptToken plugins for Gwenhywfar. These plugins
 * can be used by AqBanking.
 *
 * The following CryptToken plugins are provided:
 * <ul>
 *  <li>DdvCard (supports DDV0 and DDV1 cards)</li>
 *  <li>
 *    StarcosCard (with subtypes <i>starcoscard-vr</i> and
 *    <i>starcoscard-hvb</i>)
 *  </li>
 * </ul>
 */
/*@{*/

/** @name Functions for Inheriting Classes
 *
 * Functions in this group can be used by other card based CryptToken
 * plugins.
 */
/*@{*/
/**
 * Let the chipcard verify a pin. If the card reader has a keypad then
 * secure input will be used. Otherwise the user will be asked to enter
 * a pin which is then relayed to the card.
 * Used by ddvcard, starcos, zkacard.
 */
CHIPCARD_API int LC_Crypt_Token_VerifyPin(GWEN_CRYPT_TOKEN *ct,
                                          LC_CARD *hcard,
                                          GWEN_CRYPT_PINTYPE pinType,
                                          uint32_t guiid);

/* used by zkacard */
CHIPCARD_API int LC_Crypt_Token_VerifyPinWithPinInfo(GWEN_CRYPT_TOKEN *ct,
                                                     LC_CARD *hcard,
                                                     GWEN_CRYPT_PINTYPE pinType,
                                                     const LC_PININFO *pinInfo,
                                                     uint32_t guiid);


/**
 * Let the chipcard change a pin. If the card reader has a keypad then
 * secure input will be used. Otherwise the user will be asked to enter
 * a pin which is then relayed to the card.
 */
CHIPCARD_API int LC_Crypt_Token_ModifyPin(GWEN_CRYPT_TOKEN *ct,
                                          LC_CARD *hcard,
                                          GWEN_CRYPT_PINTYPE pinType,
                                          int initial,
                                          uint32_t guiid);

/**
 * Let the chipcard change a pin. If the card reader has a keypad then
 * secure input will be used. Otherwise the user will be asked to enter
 * a pin which is then relayed to the card.
 */
CHIPCARD_API int LC_Crypt_Token_ModifyPinWithPinInfo(GWEN_CRYPT_TOKEN *ct, LC_CARD *hcard,
                                                     GWEN_CRYPT_PINTYPE pinType, const LC_PININFO *pinInfo, int initial,
                                                     uint32_t guiid);

/*@}*/

/*@}*/ /* addtogroup */


#endif
