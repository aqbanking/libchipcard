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


#ifndef CHIPCARD_CARD_PROCESSORCARD_H
#define CHIPCARD_CARD_PROCESSORCARD_H

#include <chipcard2-client/client/card.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup chipcardc_cards_proc Processor Cards
 * @ingroup chipcardc_cards
 *
 */
/*@{*/

/** @name Extending Basic Card Object
 *
 */
/*@{*/
/**
 * Extends a basic card type thus making functions of this group available.
 * This stores some processor-card-related data with the given card object.
 */
CHIPCARD_API
int LC_ProcessorCard_ExtendCard(LC_CARD *card);

/**
 * Unextend a card object which has previously been extended using
 * @ref LC_ProcessorCard_ExtendCard. This functions releases all
 * processor-card-related ressources.
 */
CHIPCARD_API
int LC_ProcessorCard_UnextendCard(LC_CARD *card);
/*@}*/


/** @name File/Folder Selection
 *
 * Most processor cards contain a file system, which consists of:
 * <ul>
 *  <li>MasterFile (MF, corresponds to <i>root</i> on real file systems)</li>
 *  <li>Dedicated Files (DF, corresponds to folders on real file systems)</li>
 *  <li>Elementary Files <(EF, corresponds to files on real file systems)</li>
 * </ul>
 * Functions of this group require that the card and application types have
 * already been set (either implicitly by some LC_xxx_ExtendCard functions
 * or via @ref LC_Card_SelectApp / @ref LC_Card_SelectCardAndApp).
 */
/*@{*/
/**
 * Select a dedicated file (DF) by its name. This function operates on the
 * currently selected DF. Use @ref LC_Card_SelectMF to select the
 * MasterFile (MF, <i>root</i>).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_ProcessorCard_SelectDF(LC_CARD *card,
                                           const char *fname);

/**
 * Selects an elementary file (EF) within the currently selected DF (or MF).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_ProcessorCard_SelectEF(LC_CARD *card,
                                           const char *fname);
/*@}*/

/** @deprecated */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_ProcessorCard_ReadRecord(LC_CARD *card,
                                             int recNum,
                                             GWEN_BUFFER *buf);

/** @deprecated */
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_ProcessorCard_WriteRecord(LC_CARD *card,
                                              int recNum,
                                              GWEN_BUFFER *buf);


/*@}*/ /* defgroup */

#ifdef __cplusplus
}
#endif

#endif /* CHIPCARD_CARD_PROCESSORCARD_H */


