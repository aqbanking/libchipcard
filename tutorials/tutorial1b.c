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

/** @file tutorial1b.c
 * @brief Basic Usage of Libchipcard2: Documented
 */

/** @defgroup MOD_TUTORIAL1B Documented
 * @ingroup MOD_TUTORIAL1
 *
 */
/*@{*/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <chipcard2-client/chipcard2.h>
#include <chipcard2-client/client/client.h>


/*
 * This is a small tutorial on how to use the basic functions of
 * libchipcard2. It just waits for a card to be inserted and prints some
 * card's information.
 * This is the most basic type of application using a chipcard, no error
 * checking is performed.
 *
 * This tutorial is intended to show the basics only.
 * After studying this tutorial you should advance to the next one, which
 * will explain all the methods used.
 *
 * Usage:
 *   tutorial1b
 */


int main(int argc, char **argv) {
  /* The basic object of Libchipcard2 itself is LC_CLIENT.
   * You must create and initialize such an object before doing anything
   * with Libchipcard2.
   */
  LC_CLIENT *cl;

  /* The other central object is LC_CARD. This is the object most card
   * commands operate on.
   */
  LC_CARD *card;

  /* Create an instance of Libchipcard2.
   * Libchipcard2 wants to know what application is requesting its service to
   * improve server-side logging. It also makes it easier to debug
   * Libchipcard2.
   * The last parameter is the path to the data folder of Libchipcard2.
   * We don't want any special handling here so we provide a NULL to make
   * Libchipcard2 use its default path.
   */
  cl=LC_Client_new("tutorial1b", "1.0", 0);

  /* Initialize Libchipcard2 by reading its configuration file.
   * Again, we don't want special treatment so we provide a NULL for the
   * filename thus making Libchipcard2 use its default configuration file.
   */
  LC_Client_ReadConfigFile(cl, 0);

  /* We now need to tell Libchipcard2 that we are interested in chipcards.
   * After sending this command the chipcard server will notify us about
   * available cards.
   * Only now the server will be connected, and if we are the only client
   * then the server now starts acquiring card readers.
   * The two latter arguments can be used to restrict the notifications to
   * inform us only about cards in special readers:
   * The second argument tells this function about the flags for an acceptable
   * reader. The third argument is the mask to define which of the flags
   * are interesting.
   * Examples:
   * 1) We are a homebanking tool, and we will be using a chipcard which is
   *    protected by a pin. This pin should be entered in secure mode (i.e.
   *    via the keypad of the card reader).
   *    Therefore we are only interested in readers which have a keypad.
   *    In this case the call would look like:
   *      LC_Client_StartWait(cl,
   *                          LC_CARD_READERFLAGS_KEYPAD,   [flags]
   *                          LC_CARD_READERFLAGS_KEYPAD);  [mask]
   *
   * 2) For some weird reason we don't like readers with keypads.
   *    So we only accept readers which have no keypad:
   *      LC_Client_StartWait(cl,
   *                          0,                            [flags]
   *                          LC_CARD_READERFLAGS_KEYPAD);  [mask]
   * 3) The default case is to accept any card in any reader, so we
   *    just pass 0 for both parameters.
   */
  LC_Client_StartWait(cl, 0, 0);

  /* It's always nice to tell the user what we expect of him. */
  fprintf(stderr, "Please insert a chip card.\n");

  /* Now that the server is informed about us being interested in chipcards
   * we can just wait for the server to notify us about inserted cards.
   * We will wait for about 30 seconds.
   */
  card=LC_Client_WaitForNextCard(cl, 30);

  /* Now that we have found a card we can tell the server that we don't want
   * to be informed about other cards.
   */
  LC_Client_StopWait(cl);

  /* We don't own the card yet! The server just informed us that a card has
   * been inserted. To work with the card we need to grab it. This is all
   * done internally by this call. If this call succeeds we are the only
   * client to access the card. All other clients are blocked out until we
   * release the card via @ref LC_Card_Close() or @ref LC_Client_free().
   */
  LC_Card_Open(card);

  /* NOW we own the card and are free to do whatever we like with it ;-)
   * Well, we could call the following function without grabbing the card
   * since it only prints information from the notification received via
   * @ref LC_Client_WaitForNextCard().
   * However, this is a tutorial, so we go all the way ;-)
   */
  LC_Card_Dump(card, stderr, 0);

  /* After working with the card we should always release it so that other
   * clients may access it. If no other client is interested in this card
   * the reader is shutdown after a grace period, so releasing a card after
   * using it is always a good idea.
   * However, if we disconnect from the chipcard server the card is released
   * anyway but is is much nicer to release the card explicitly.
   */
  LC_Card_Close(card);

  /* Release all ressources associated with Libchipcard2.
   * You should always do this at the end of your program to prevent
   * memory leaks.
   */
  LC_Card_free(card);
  LC_Client_free(cl);

  /* Aaaand that's it ;-)
   */
  return 0;
}


/*@}*/


