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

#ifndef READER_LIBCHIPCARD2_P_H
#define READER_LIBCHIPCARD2_P_H

#define OPENSC_LOGDOMAIN                      "opensc"
/* soon to be made configurable */
#define OPENSC_CHIPCARD2_MAXREADER            4
#define OPENSC_CHIPCARD2_WAITCARD_TIMEOUT     30
#define OPENSC_CHIPCARD2_COMMAND_TIMEOUT      60
#define OPENSC_CHIPCARD2_CHECKSTATUS_INTERVAL 5

#include <opensc/opensc.h>
#include <chipcard2-client/client/client.h>
#include <chipcard2-client/client/card.h>
#include <gwenhywfar/logger.h>


/* This is a really ugly hack, but these symbols are very much needed.
 * Unfortunately they are not exported by OpenSC, so I have to set the
 * prototypes here.
 */
int _sc_add_reader(struct sc_context *ctx, struct sc_reader *reader);
int _sc_parse_atr(struct sc_context *ctx, struct sc_slot_info *slot);
int ctbcs_pin_cmd(struct sc_reader *, sc_slot_info_t *, struct sc_pin_cmd_data *);


typedef struct chipcard2_slot_data chipcard2_slot_data;
typedef struct chipcard2_reader_data chipcard2_reader_data;
typedef struct chipcard2_global_private_data chipcard2_global_private_data;


struct chipcard2_slot_data {
    void *empty;
};


/* reader specific private data */
struct chipcard2_reader_data {
  struct chipcard2_global_private_data *gpriv;
  GWEN_TYPE_UINT32 readerId;
  LC_CARD *card;
  int isOpen;
  struct chipcard2_slot_data *slot_data;
  time_t lastStatusCheckTime;
};



struct chipcard2_global_private_data {
  LC_CLIENT *client;
  int waiting;
};


void chipcard2_free_slot_data(struct chipcard2_slot_data *pslot);
void chipcard2_free_reader_data(struct chipcard2_reader_data *priv);
void chipcard2_free_global_data(struct chipcard2_global_private_data *gpriv);
void chipcard2__showError(LC_CARD *card,
			  LC_CLIENT_RESULT res,
			  const char *failedCommand) ;
static int chipcard2_transmit(struct sc_reader *reader,
			      struct sc_slot_info *slot,
			      const u8 *sendbuf,
			      size_t sendsize,
			      u8 *recvbuf,
			      size_t *recvsize,
			      int control);
static int chipcard2_detect_card_presence(struct sc_reader *reader,
					  struct sc_slot_info *slot);
static int chipcard2_getcard(struct sc_reader *reader,
			     struct sc_slot_info *slot);
static int chipcard2_connect(struct sc_reader *reader,
			     struct sc_slot_info *slot);
static int chipcard2_disconnect(struct sc_reader *reader,
				struct sc_slot_info *slot,
				int action);
static int chipcard2_lock(struct sc_reader *reader,
			  struct sc_slot_info *slot);
static int chipcard2_unlock(struct sc_reader *reader,
			    struct sc_slot_info *slot);
static int chipcard2_release(struct sc_reader *reader);
static int chipcard2_init(struct sc_context *ctx, void **reader_data);
static int chipcard2_finish(struct sc_context *ctx, void *prv_data);
struct sc_reader_driver * sc_get_chipcard2_driver(void);


#endif /* READER_LIBCHIPCARD2_P_H */


