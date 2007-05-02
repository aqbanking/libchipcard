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


#ifndef LC_CT_CARD_H
#define LC_CT_CARD_H

#include <chipcard3/client/card.h>
#include <gwenhywfar/crypttoken.h>


/** @addtogroup chipcardc_client_ct
 * @short Chipcard-based CryptTokens
 *
 * Libchipcard2 provides CryptToken plugins for Gwenhywfar. These plugins
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
 */
CHIPCARD_API
int LC_CryptToken_VerifyPin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt);

/**
 * Let the chipcard change a pin. If the card reader has a keypad then
 * secure input will be used. Otherwise the user will be asked to enter
 * a pin which is then relayed to the card.
 */
CHIPCARD_API
int LC_CryptToken_ChangePin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt,
                            int initial);

/**
 * Convert the given client result code to one of Gwenhywfars error codes.
 */
CHIPCARD_API
int LC_CryptToken_ResultToError(LC_CLIENT_RESULT res);
/*@}*/

/*@}*/ /* addtogroup */


#endif
