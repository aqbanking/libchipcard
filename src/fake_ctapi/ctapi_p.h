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
GWEN_LIST_FUNCTION_DEFS(CTAPI_CONTEXT, CTAPI_Context)


struct CTAPI_CONTEXT {
  GWEN_LIST_ELEMENT(CTAPI_CONTEXT)
  unsigned short ctn;
  unsigned short port;
  LC_CARD *card;
  int isOpen;
};

CTAPI_CONTEXT *CTAPI_Context_new(unsigned short ctn,
				 unsigned short port);
void CTAPI_Context_free(CTAPI_CONTEXT *ctx);


CTAPI_CONTEXT *CTAPI_Context_FindByCtn(unsigned short ctn);
CTAPI_CONTEXT *CTAPI_Context_FindByPort(unsigned short port);


void CT__showError(LC_CARD *card,
		   LC_CLIENT_RESULT res,
		   const char *failedCommand);



#endif
