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


#ifndef CHIPCARD_CTAPI_P_H
#define CHIPCARD_CTAPI_P_H

#include "ctapi.h"

#include <chipcard2-client/client/client.h>
#include <chipcard2-client/client/card.h>
#include <gwenhywfar/misc.h>


typedef struct CTAPI_CONTEXT CTAPI_CONTEXT;
typedef struct CTAPI_APDU CTAPI_APDU;
GWEN_LIST_FUNCTION_DEFS(CTAPI_CONTEXT, CTAPI_Context)


struct CTAPI_CONTEXT {
  GWEN_LIST_ELEMENT(CTAPI_CONTEXT)
  unsigned short ctn;
  unsigned short port;
  LC_CARD *card;
  int isOpen;
  char *cardType;
};



CTAPI_CONTEXT *CTAPI_Context_new(unsigned short ctn,
				 unsigned short port);
void CTAPI_Context_free(CTAPI_CONTEXT *ctx);
void CTAPI_Context_SetCardType(CTAPI_CONTEXT *ctx, const char *ct);


CTAPI_CONTEXT *CTAPI_Context_FindByCtn(unsigned short ctn);
CTAPI_CONTEXT *CTAPI_Context_FindByPort(unsigned short port);


void CT__showError(LC_CARD *card,
		   LC_CLIENT_RESULT res,
		   const char *failedCommand);

LC_CLIENT_RESULT CT__openCard(CTAPI_CONTEXT *ctx, int timeout);

char CT__secureVerify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response);

char CT__secureModify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response);


char CT__requestICC(CTAPI_CONTEXT *ctx,
                    unsigned char *dad,
                    unsigned char *sad,
                    CTAPI_APDU *apdu,
                    unsigned short *lenr,
                    unsigned char *response);



char CT__getStatusICC(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response);



char CT__ejectICC(CTAPI_CONTEXT *ctx,
                  unsigned char *dad,
                  unsigned char *sad,
                  CTAPI_APDU *apdu,
                  unsigned short *lenr,
                  unsigned char *response);



char CT__resetICC(CTAPI_CONTEXT *ctx,
                  unsigned char *dad,
                  unsigned char *sad,
                  CTAPI_APDU *apdu,
                  unsigned short *lenr,
                  unsigned char *response);



struct CTAPI_APDU {
  int cla;
  int ins;
  int p1;
  int p2;
  int dlen;
  unsigned char *data;
  int rlen;
};

CTAPI_APDU *CTAPI_APDU_new(unsigned char *cmd, int len);
void CTAPI_APDU_free(CTAPI_APDU *apdu);

int CT__getPinId(CTAPI_APDU *apdu);


#endif
