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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef BUILDING_LIBCHIPCARD2_DLL


/* You always need to include the header files of Libchipcard2 to work with
 * it ;-)
 */
#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/client.h>


/**
 * Please go to the source of this for a crosslinked view (see link below).
 * @callgraph
 */
int main(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CARD *card;

  cl=LC_Client_new("tutorial1a", "1.0", 0);
  LC_Client_ReadConfigFile(cl, 0);

  LC_Client_StartWait(cl, 0, 0);

  fprintf(stderr, "Please insert a chip card.\n");
  card=LC_Client_WaitForNextCard(cl, 30);

  LC_Client_StopWait(cl);

  LC_Card_Open(card);

  LC_Card_Dump(card, stderr, 0);

  LC_Card_Close(card);

  LC_Card_free(card);
  LC_Client_free(cl);
  return 0;
}




