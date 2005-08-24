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

#include <chipcard2-client/client/card.h>
#include <gwenhywfar/crypttoken.h>


int LC_CryptToken_VerifyPin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt);

int LC_CryptToken_ChangePin(GWEN_PLUGIN_MANAGER *pluginManager,
                            GWEN_CRYPTTOKEN *ct,
                            LC_CARD *hcard,
                            GWEN_CRYPTTOKEN_PINTYPE pt,
                            int initial);

int LC_CryptToken_ResultToError(LC_CLIENT_RESULT res);


#endif
