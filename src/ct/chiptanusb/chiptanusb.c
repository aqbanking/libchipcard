/***************************************************************************
    begin       : Thu Jan 09 2020
    copyright   : (C) 2020 by Herbert Ellebruch
    copyright   : 2021 Martin Preuss
    email       :

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "chiptanusb.h"

#include <libchipcard/cards/chiptanusb/chiptanusb.h>
#include <libchipcard/cards/processorcard/processorcard.h>
#include <libchipcard/ct/ct_card.h>

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/ctplugin_be.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/padd.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/i18n.h>
#include <gwenhywfar/tlv.h>

#define PROGRAM_VERSION "1.0"



static void _extractReceivedGeneratorFields(GWEN_BUFFER *mbuf, int *ATC, char *TAN_String, uint32_t maxTanLen,
                                            char *pCardnummber, char *pEndDate, char *pIssueDate);




/*
	Building the apdu string
	Die Eingabedaten werden wie folgt aufgebaut:
	Laenge             Inhalt        Bedeutung
        4                  00 00 00 00   Activation ID (fix)
        1                  01            Processing Option Flag (POF) (fix)
        1                  00            Controllbyte (fix)
        2                  XX XX         Laenge m des Datenblocks (MSB first, dez 35 = 00 23)
        m                  XX  XX        Datenblock der Laenge m (Flickerdaten)

	diese sind als Daten Teil in ein Secoder-Kommando Nr 6 einzutragen also
		20 76 00 00 00 <Lc> <Lc> <Eingabedaten> 00(Le) 00(Le)
	Oder Boxing
		FF 91 06 00 00 <Lc> <Lc> <Eingabedaten> 00(Le) 00(Le)

        Quittung fuer die TanGenerierung
         transmitted:
          FF 91 07 00 00 00 06 00 00 00 00 00 00 00 00
         received:
          00 01 91 00
        Zurueckmeldung:
        sw1 = 0x91
        sw2 = 0;
*/


/* Extract card Info
   Parameter:
   mbuf:           Info received from card
   ATC:            pointer to ATC
   TAN_String:     pointer to generated Tan
   maxTanLen:      max Tan length in characters (size of receive Buffer must be maxTanLen + 1)
   pCardnummber:   pointer to card number (char[11] 10 + Terminator)
   pEndDate:       pointer to end date of card yymm
              type of field must be char[5]
   pIssueDate:      pointer to IssueDate of card ddmmyy
              type of field must be char[7]
*/

void _extractReceivedGeneratorFields(GWEN_BUFFER *mbuf, int *ATC, char *TAN_String, uint32_t maxTanLen,
                                    char *pCardnummber, char *pEndDate, char *pIssueDate)
{

  unsigned int tmpATC;

  int iTAN_Length;

  int ATC_Size;
  int ATC_Count;
  int SizeField;
  char cardInfo[50];

  int SizeCardinfo;

  /* copy TAN */
  int count = 0;
  unsigned char *pResultBuffer = (unsigned char *)GWEN_Buffer_GetStart(mbuf);
  char *pDest = TAN_String;

  iTAN_Length = *pResultBuffer++;
  for (count = 0; count < iTAN_Length; count++) {
    unsigned char temp;
    *pDest++ = ((*pResultBuffer) >> 4) + '0';
    temp = (*pResultBuffer++) & 0xF;
    if (temp != 0xF) {
      *pDest++ = temp + '0';
    }
  }
  *pDest = '\0';

  /* copy ATC */
  ATC_Size = (*pResultBuffer++);

  tmpATC = (unsigned char)(*pResultBuffer++);
  for (ATC_Count = 1; ATC_Count < ATC_Size; ATC_Count++) {
    tmpATC = (tmpATC << 8) + (unsigned char)(*pResultBuffer++);
  }
  *ATC = tmpATC;

  /* skip Next field */
  SizeField = *pResultBuffer++;
  pResultBuffer += SizeField;

  pDest = &cardInfo[0];

  /* convert complete card Info to char */
  SizeCardinfo = *pResultBuffer++;;
  for (count = 0; count < SizeCardinfo; count++) {
    unsigned char temp;
    *pDest++ = ((*pResultBuffer) >> 4) + '0';
    temp = (*pResultBuffer++) & 0xF;
    if (temp != 0xF) {
      *pDest++ = temp + '0';
    }
  }
  *pDest++ = '\0';

  /* copy card number */
  memcpy(pCardnummber, &cardInfo[8], 10);
  *(pCardnummber + 10) = '\0';

  /* copy end date */
  memcpy(pEndDate, &cardInfo[20], 4);
  *(pEndDate + 4) = '\0';

  memcpy(pIssueDate, &cardInfo[24], 6);
  *(pIssueDate + 6) = '\0';
}

/* Read information from Card
   Parameter:
   HHDCommand:     Parameter for the card
   fullHHD_Len:    size of HHDCommand field
   pGeneratedTAN:   pointer to generated Tan
   maxTanLen:       max Tan length in characters (size of receive Buffer must be maxTanLen + 1)
   pATC:            pointer to ATC
   pCardnummber:    pointer to card number (char[11] 10 + Terminator)
   pEndDate:        pointer to end date of card yymm
                          (char[5] 4 +  Terminator )
   pIssueDate:       pointer to IssueDate of card ddmmyy
                          (char[7])  4 +  Terminator
*/
int GetTanfromUSB_Generator(unsigned char *HHDCommand, int fullHHD_Len, int *pATC, char *pGeneratedTAN,
                            uint32_t maxTanLen, char *pCardnummber, char *pEndDate, char *pIssueDate)
{

  LC_CLIENT *cl;
  LC_CARD *card;
  int res;
  GWEN_BUFFER *mbuf;

  cl = LC_Client_new("PinTanKarte", PROGRAM_VERSION);//  client.c
  if (LC_Client_Init(cl)) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not init libchipcard");
    LC_Client_free(cl);
    return GWEN_ERROR_NOT_CONNECTED;
  }

  DBG_INFO(LC_LOGDOMAIN, "Connecting to server.");
  res = LC_Client_Start(cl);
  if (res<0) {
    return GWEN_ERROR_NOT_CONNECTED;
  }
  DBG_INFO(LC_LOGDOMAIN, "Connected.");

  DBG_INFO(LC_LOGDOMAIN, "Waiting for card...");
  res = LC_Client_GetNextCard(cl, &card, 20);
  if (res<0) {
    DBG_ERROR(LC_LOGDOMAIN, "GetNextCard.");
    return GWEN_ERROR_REMOVED;
  }

  DBG_INFO(LC_LOGDOMAIN, "Found a card.");
  if (LC_ChiptanusbCard_ExtendCard(card)) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not extend card as CipTanUsb card.");
    return GWEN_ERROR_INVALID;
  }

  DBG_INFO(LC_LOGDOMAIN, "Opening card.");
  res = LC_Card_Open(card);
  if (res<0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error executing command CardOpen (%d).\n", res);
    return GWEN_ERROR_OPEN;
  }

  DBG_INFO(LC_LOGDOMAIN, "Card is a ChipTanUsb card as expected.");

  mbuf = GWEN_Buffer_new(0, 256, 0, 1);
  res = LC_ChiptanusbCard_GenerateTan(card, HHDCommand, fullHHD_Len, mbuf);
  if (res<0) {
    DBG_ERROR(LC_LOGDOMAIN, "Error Reading Tan from card.\n");
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_READ;
  }

  _extractReceivedGeneratorFields(mbuf, pATC, pGeneratedTAN, maxTanLen, pCardnummber, pEndDate, pIssueDate);
  GWEN_Buffer_free(mbuf);

  LC_Card_Close(card);
  LC_Client_ReleaseCard(cl, card);
  LC_Card_free(card);
  LC_Client_free(cl);

  return (0);
}



CHIPCARD_EXPORT
GWEN_PLUGIN *ct_chiptanusb_factory(GWEN_PLUGIN_MANAGER *pm,
                                   const char *modName,
                                   const char *fileName)
{
  GWEN_PLUGIN *pl;

  pl=GWEN_Crypt_Token_Plugin_new(pm, GWEN_Crypt_Token_Device_Card, modName, fileName);
  if (pl==NULL) {
    DBG_ERROR(LC_LOGDOMAIN, "No plugin created");
    return NULL;
  }
  return pl;
}











