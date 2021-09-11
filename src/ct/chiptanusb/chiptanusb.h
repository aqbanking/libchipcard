/***************************************************************************
    begin       : Sat Sep 11 2021
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CT_CHIPTANUSB_H
#define CHIPCARD_CT_CHIPTANUSB_H


#include <chipcard/card.h>



/** Read information from Card
   @param HHDCommand      parameter for the card
   @param fullHHD_Len     size of HHDCommand field
   @param pGeneratedTAN   pointer to generated Tan
   @param maxTanLen       max Tan length in characters (size of receive Buffer must be maxTanLen + 1)
   @param pATC            pointer to ATC
   @param pCardnummber    pointer to card number (char[11] 10 + Terminator)
   @param pEndDate        pointer to end date of card yymm
                          (char[5] 4 +  Terminator )
   @param pIssueDate      pointer to IssueDate of card ddmmyy
                          (char[7])  4 +  Terminator
*/
CHIPCARD_EXPORT
  int GetTanfromUSB_Generator(unsigned char *HHDCommand, int fullHHD_Len, int *pATC, char *pGeneratedTAN,
			      uint32_t maxTanLen, char *pCardnummber, char *pEndDate, char *pIssueDate);




#endif
