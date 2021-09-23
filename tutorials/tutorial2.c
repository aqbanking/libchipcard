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
#undef BUILDING_LIBCHIPCARD2_DLL


#include <libchipcard/chipcard.h>
#include <libchipcard/base/client.h>
#include <libchipcard/cards/kvkcard/kvkcard.h>


/*
 * While tutorial1 only works with base classes we are now to understand how
 * more complex card classes can be used.
 * This tutorial waits for a German medical card to be inserted, reads the
 * information stored on the card and prints it to the standard output
 * channel.
 *
 * The only parts which differ a marked below by lines of equation marks,
 * the rest is pretty much the same as in the other tutorials.
 *
 * Usage:
 *   tutorial2
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
  int rv;
  GWEN_DB_NODE *dbData=NULL;

  cl=LC_Client_new("tutorial2", "1.0");
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

  fprintf(stderr, "Please insert a German medical card.\n");
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

  /* ======================================================================
   * Until now we only handled basic card functions as the other tutorials
   * did.
   * The inserted card is supposed to be a German medical card, so we need
   * to tell Libchipcard2 that we want to use it as such. This makes sure that
   * the correct card commands for the reader/card combination is used
   * internally.
   * The following function also sets more specific functions for the complex
   * card type to be called internally upon LC_Card_Open() and
   * LC_Card_Close(), so we need to call the _ExtendCard() function before
   * the function LC_Card_Open() !
   * The specific open() function for a German medical card for example reads
   * the known fields from the card.
   * This function seldomly fails, however, you should always be prepared
   * that it could.
   *
   * If you later want to use the card as a memory card you can just
   * unextend the card (LC_KVKCard_UnextendCard()) and then extend it as
   * a memory card, like in:
   *    LC_KVKCard_UnextentCard(card);
   *    LC_MemoryCard_ExtendCard(card);
   * Please always remember to unextend an extended card before extending it
   * as a different type.
   *
   * By the way: Since a German medical card basically is a memory card you
   * would in this case not need to unextend it, since internally the function
   * LC_KVKCard_ExtendCard() also extends it as such.
   *
   * This is a fine example of the heritage model used in Libchipcard2:
   * You could also create your own card type by extending an existing one.
   */
  rv=LC_KVKCard_ExtendCard(card);
  if (rv) {
    fprintf(stderr, "Could not extend card as German medical card\n");
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

  /* Since the card has been extended as a German medical card we can now
   * use the functions of that module.
   * In this case we retrieve the user information stored on the card.
   * Please note that there is no data if the card is corrupted (e.g. a bad
   * checksum).
   */
  /* dbData=LC_KVKCard_GetCardData(card); */ /* FIXME: deprecated function */
  if (!dbData) {
    fprintf(stderr, "ERROR: No card data available.\n");
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    LC_Client_free(cl);
    return 2;
  }

  /* The data returned is stored in a GWEN_DB which we now present to the
   * user (see the API documentation in Gwenhywfar for a description of
   * a GWEN_DB, search for GWEN_DB_NODE).
   */
  fprintf(stderr, "INFO: I got this card:\n");
  GWEN_DB_Dump(dbData, 2);

  /* ====================================================================== */


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






