/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <libchipcard/chipcard.h>
#include <libchipcard/base/client.h>


/*
 * This is a small tutorial on how to use the basic functions of
 * libchipcard2. It just waits for a card to be inserted and prints some
 * card's information.
 * This is the most basic type of application using a chipcard, no error
 * checking is performed.
 *
 * This version now does error checking.
 *
 * Usage:
 *   tutorial1c
 */


/* This function explains an error */
void showError(LC_CARD *card, int res, const char *failedCommand)
{
  LC_Card_PrintResult(card, failedCommand, res);
}



int main(int argc, char **argv)
{
  LC_CLIENT *cl;
  LC_CARD *card=0;
  int res;

  cl=LC_Client_new("tutorial1c", "1.0");
  res=LC_Client_Init(cl);
  if (res<0) {
    showError(card, res, "Init");
    LC_Client_free(cl);
    return 1;
  }

  fprintf(stderr, "INFO: Connecting to server.\n");
  res=LC_Client_Start(cl);
  if (res<0) {
    showError(card, res, "StartWait");
    LC_Client_free(cl);
    return 2;
  }

  fprintf(stderr, "Please insert a chip card.\n");
  res=LC_Client_GetNextCard(cl, &card, 30);
  if (res<0) {
    showError(card, res, "GetNextCard");
    LC_Client_Stop(cl);
    LC_Client_free(cl);
    return 2;
  }

  /* stop waiting */
  fprintf(stderr, "INFO: Telling the server that we need no more cards.\n");
  res=LC_Client_Stop(cl);
  if (res<0) {
    showError(card, res, "Stop");
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    LC_Client_free(cl);
    return 2;
  }

  /* open card */
  fprintf(stderr, "INFO: Opening card.\n");
  res=LC_Card_Open(card);
  if (res<0) {
    showError(card, res, "CardOpen");
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    LC_Client_free(cl);
    return 2;
  }

  /* show card information */
  fprintf(stderr, "INFO: I got this card:\n");
  LC_Card_Dump(card, 0);

  /* close card */
  fprintf(stderr, "INFO: Closing card.\n");
  res=LC_Card_Close(card);
  if (res<0) {
    showError(card, res, "CardClose");
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    LC_Client_free(cl);
    return 2;
  }
  fprintf(stderr, "INFO: Card closed.\n");

  /* release card */
  res=LC_Client_ReleaseCard(cl, card);
  if (res<0) {
    showError(card, res, "CardRelease");
    LC_Card_free(card);
    LC_Client_free(cl);
    return 2;
  }

  /* cleanup */
  LC_Card_free(card);
  LC_Client_free(cl);
  return 0;
}


