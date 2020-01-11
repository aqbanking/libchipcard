/***************************************************************************
    begin       : Thu Jan 09 2020
    copyright   : (C) 2020 by Ellebruch Herbert
    email       : 

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef CHIPCARD_CT_CHIPTANUSB_P_H
#define CHIPCARD_CT_CHIPTANUSB_P_H

// #include <gwenhywfar/ct_be.h>
// #include <gwenhywfar/ctplugin.h>
#include <chipcard/card.h>

/* Extract card Info
   Parameter:
   mbuf:           Info received from card
   ATC:            pointer to ATC
   TAN_String:     pointer to generated Tan
   maxTanLen:      max Tan length in characters (size of receive Buffer must be maxTanLen + 1)
   pCardnummber:   pointer to card number (char[11] 10 + Terminator)
   pEndDate:       pointer to end date of card yymm
						  type of field must be char[5]
   pIssueDate:     pointer to IssueDate of card ddmmyy
						  type of field must be char[7]
*/

static void ExtractReceivedGeneratorFields(GWEN_BUFFER * mbuf, int* ATC, char *TAN_String, uint32_t maxTanLen, char* pCardnummber, char* pEndDate, char* pIssueDate);


#endif
