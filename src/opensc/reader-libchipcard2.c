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

/* This is a reader driver for OpenSC.
 * You can enable this driver with OpenSC by adding the name "chipcard2" to
 * the OpenSC configuration file variable "app/reader_drivers".
 * You will also have to add a driver section to that configuration file:
 * -------------------------------------------------------------------------
 * reader_driver chipcard2 {
 *   module = /usr/lib/reader-libchipcard2;
 * }
 * -------------------------------------------------------------------------
 * Such a section allows OpenSC to dynamically load the driver module.
 *
 * This driver adds 4 virtual readers to OpenSC. This allows OpenSC to use
 * up to this number of readers of Libchipcard2 in parallel.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef BUILDING_LIBCHIPCARD2_DLL

#include "reader-libchipcard2_p.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>


#define GET_SLOT_PTR(s, i) (&(s)->slot[(i)])
#define GET_PRIV_DATA(r) ((chipcard2_reader_data *) (r)->drv_data)
#define GET_SLOT_DATA(r) ((chipcard2_slot_data *) (r)->drv_data)



void chipcard2_free_slot_data(chipcard2_slot_data *pslot){
  if (pslot) {
    free(pslot);
  }
}



void chipcard2_free_reader_data(chipcard2_reader_data *priv){
  DBG_DEBUG(OPENSC_LOGDOMAIN, "Freeing reader data %p", priv);
  if (priv) {
    LC_Card_free(priv->card);
    priv->card=0;
    priv->isOpen=0;
    chipcard2_free_slot_data(priv->slot_data);
    priv->slot_data=0;
    free(priv);
  }
}



void chipcard2_free_global_data(chipcard2_global_private_data *gpriv){
  if (gpriv) {
    //chipcard2_reader_data **priv;

    LC_Client_free(gpriv->client);
    gpriv->client=0;
  }
}



void chipcard2__showError(LC_CARD *card,
			  LC_CLIENT_RESULT res,
			  const char *failedCommand) {
  const char *s;

  switch(res) {
  case LC_Client_ResultOk:
    s="Ok.";
    break;
  case LC_Client_ResultWait:
    s="Timeout.";
    break;
  case LC_Client_ResultIpcError:
    s="IPC error.";
    break;
  case LC_Client_ResultCmdError:
    s="Command error.";
    break;
  case LC_Client_ResultDataError:
    s="Data error.";
    break;
  case LC_Client_ResultAborted:
    s="Aborted.";
    break;
  case LC_Client_ResultInvalid:
    s="Invalid argument to command.";
    break;
  case LC_Client_ResultInternal:
    s="Internal error.";
    break;
  case LC_Client_ResultGeneric:
    s="Generic error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  DBG_ERROR(OPENSC_LOGDOMAIN, "Error in \"%s\": %s\n", failedCommand, s);
  if (res==LC_Client_ResultCmdError && card) {
    int sw1;
    int sw2;

    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    DBG_ERROR(OPENSC_LOGDOMAIN, "  Last card command result:\n");
    if (sw1!=-1 && sw2!=-1) {
      DBG_ERROR(OPENSC_LOGDOMAIN, "   SW1=%02x, SW2=%02x\n", sw1, sw2);
    }
    s=LC_Card_GetLastResult(card);
    if (s) {
      DBG_ERROR(OPENSC_LOGDOMAIN, "   Result: %s\n", s);
    }
    s=LC_Card_GetLastText(card);
    if (s) {
      DBG_ERROR(OPENSC_LOGDOMAIN, "   Text  : %s\n", s);
    }
  }
}



static int chipcard2_transmit(struct sc_reader *reader,
			      struct sc_slot_info *slot,
			      const u8 *sendbuf,
			      size_t sendsize,
			      u8 *recvbuf,
			      size_t *recvsize,
			      int control){
  chipcard2_reader_data *priv = GET_PRIV_DATA(reader);
  int result;

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_transmit(%p)", reader);

  if (*recvsize<2) {
    DBG_ERROR(OPENSC_LOGDOMAIN,
	      "Response buffer too small (%d, need at least 2 bytes)",
	      *recvsize);
    return SC_ERROR_BUFFER_TOO_SMALL;
  }

  if (sendsize<4) {
    DBG_ERROR(OPENSC_LOGDOMAIN,
	      "Too few bytes in APDU (%d, need at least 4 bytes)", sendsize);
    return SC_ERROR_CMD_TOO_SHORT;
  }


  if (sendbuf[0]==0x20) {
    switch(sendbuf[1]) {
    case 0x11: /* RESET ICC */
    case 0x12: /* REQUEST ICC */
    case 0x13: /* GET STATUS */
    case 0x14: /* DEACTIVATE ICC */
    case 0x15: /* EJECT ICC */
    case 0x18: /* Perform Verification */
    case 0x19: /* Modify Verification Data */
      DBG_ERROR(OPENSC_LOGDOMAIN, "APDU %02x %02x not allowed",
                sendbuf[0], sendbuf[1]);
      return SC_ERROR_INVALID_ARGUMENTS;
    default:
      break;
    }
  }

  /* send APDU to the card if we have any */
  if (priv->isOpen && priv->card) {
    LC_CLIENT_RESULT res;
    GWEN_BUFFER *rbuf;
    int i;

    DBG_INFO(OPENSC_LOGDOMAIN, "Sending APDU: ");
    GWEN_Text_LogString(sendbuf, sendsize,
                        OPENSC_LOGDOMAIN,
                        GWEN_LoggerLevelInfo);
    rbuf=GWEN_Buffer_new(0, 300, 0, 1);
    res=LC_Card_ExecAPDU(priv->card,
			 sendbuf, sendsize,
			 rbuf,
			 control?LC_Client_CmdTargetReader:LC_Client_CmdTargetCard,
			 OPENSC_CHIPCARD2_COMMAND_TIMEOUT);
    i=GWEN_Buffer_GetUsedBytes(rbuf);
    if (i) {
      if (i>*recvsize) {
	DBG_ERROR(OPENSC_LOGDOMAIN, "Buffer too small");
	result=SC_ERROR_BUFFER_TOO_SMALL;;
      }
      else {
	DBG_INFO(OPENSC_LOGDOMAIN, "Response received: ");
        memmove(recvbuf, GWEN_Buffer_GetStart(rbuf), i);
	*recvsize=i;
        GWEN_Text_LogString(recvbuf, *recvsize,
                            OPENSC_LOGDOMAIN,
                            GWEN_LoggerLevelInfo);
        result=0;
      }
    }
    else {
      if (res!=LC_Client_ResultOk) {
	chipcard2__showError(priv->card, res, "LC_Card_ExecAPDU");
	result=SC_ERROR_TRANSMIT_FAILED;
      }
      else {
	/* nothing returned */
	DBG_ERROR(OPENSC_LOGDOMAIN, "Nothing returned");
	result=SC_ERROR_TRANSMIT_FAILED;
      }
    } /* if buffer filled */
    GWEN_Buffer_free(rbuf);
  } /* if open */
  else {
    recvbuf[0]=0x62; /* no card */
    recvbuf[1]=0x00;
    *recvsize=2;
    result=0;
  }

  return result;
}



static int chipcard2_detect_card_presence(struct sc_reader *reader,
					  struct sc_slot_info *slot){
  chipcard2_reader_data *priv = GET_PRIV_DATA(reader);

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_detect_card_presence(%p)", reader);

  if (priv->card && priv->isOpen) {
    if (difftime(time(0), priv->lastStatusCheckTime)
	>=
	OPENSC_CHIPCARD2_CHECKSTATUS_INTERVAL) {
      LC_CLIENT_RESULT res;

      res=LC_Card_Check(priv->card);
      priv->lastStatusCheckTime=time(0);
      if (res!=LC_Client_ResultOk) {
	if (res==LC_Client_ResultCardRemoved) {
          DBG_INFO(OPENSC_LOGDOMAIN, "Card removed");
	  LC_Card_Close(priv->card);
	  LC_Card_free(priv->card);
	  priv->card=0;
	  priv->isOpen=0;
	}
	else {
	  chipcard2__showError(priv->card, res, "LC_Card_Check");
	}
	slot->flags&=~SC_SLOT_CARD_PRESENT;
      }
    }
    else
      slot->flags|=SC_SLOT_CARD_PRESENT;
  }
  else {
    int rv;

    rv=chipcard2_getcard(reader, slot);
    if (rv)
      slot->flags&=~SC_SLOT_CARD_PRESENT;
    else
      slot->flags|=SC_SLOT_CARD_PRESENT;
  }

  return slot->flags;
}



static int chipcard2_getcard(struct sc_reader *reader,
			     struct sc_slot_info *slot){
  chipcard2_reader_data *priv = GET_PRIV_DATA(reader);

  if (priv->gpriv->waiting==0) {
    LC_CLIENT_RESULT res;

    /* make Libchipcard2 aware of our intentions */
    res=LC_Client_StartWait(priv->gpriv->client, 0, 0);
    if (res!=LC_Client_ResultOk) {
      chipcard2__showError(0, res, "StartWait");
      return SC_ERROR_TRANSMIT_FAILED;
    }
    priv->gpriv->waiting=1;
  }

  if (priv->card==0) {
    LC_CLIENT_RESULT res;
    const GWEN_STRINGLIST *sl;

    /* try to get the next card */
    priv->card=LC_Client_WaitForNextCard(priv->gpriv->client, OPENSC_CHIPCARD2_WAITCARD_TIMEOUT);
    DBG_DEBUG(OPENSC_LOGDOMAIN, "Got card %p", priv->card);
    if (!priv->card) {
      DBG_ERROR(OPENSC_LOGDOMAIN, "No card");
      slot->flags&=~SC_SLOT_CARD_PRESENT;
      return SC_ERROR_CARD_NOT_PRESENT;
    }

    if (!priv->isOpen) {
      GWEN_BUFFER *atr;

      res=LC_Card_Open(priv->card);
      if (res!=LC_Client_ResultOk) {
	chipcard2__showError(priv->card, res, "CardOpen");
	return res;
      }
      DBG_INFO(OPENSC_LOGDOMAIN, "Card is open");
      priv->isOpen=1;
      slot->flags|=SC_SLOT_CARD_PRESENT;
      priv->lastStatusCheckTime=time(0);
  
      sl=LC_Card_GetCardTypes(priv->card);
      assert(sl);
      if (GWEN_StringList_HasString(sl, "rsacard")) {
	res=LC_Card_SelectCardAndApp(priv->card, "rsacard", "rsacard");
	if (res!=LC_Client_ResultOk) {
	  DBG_INFO(OPENSC_LOGDOMAIN, "here");
	  return SC_ERROR_TRANSMIT_FAILED;
	}
      }
      else if (GWEN_StringList_HasString(sl, "ddv1")) {
	res=LC_Card_SelectCardAndApp(priv->card, "ddv1", "ddv");
	if (res!=LC_Client_ResultOk) {
	  DBG_INFO(OPENSC_LOGDOMAIN, "here");
	  return SC_ERROR_TRANSMIT_FAILED;
	}
      }
      else if (GWEN_StringList_HasString(sl, "ddv0")) {
	res=LC_Card_SelectCardAndApp(priv->card, "ddv0", "ddv");
	if (res!=LC_Client_ResultOk) {
	  DBG_INFO(OPENSC_LOGDOMAIN, "here");
	  return SC_ERROR_TRANSMIT_FAILED;
	}
      }
      else if (GWEN_StringList_HasString(sl, "geldkarte")) {
	res=LC_Card_SelectCardAndApp(priv->card, "geldkarte", "geldkarte");
	if (res!=LC_Client_ResultOk) {
	  DBG_INFO(OPENSC_LOGDOMAIN, "here");
	  return SC_ERROR_TRANSMIT_FAILED;
	}
	/* add other types here */
      }
      else {
	if ((strcasecmp(LC_Card_GetCardType(priv->card), "processor")==0) &&
	    GWEN_StringList_HasString(sl, "processorCard")) {
	  res=LC_Card_SelectCardAndApp(priv->card, "ProcessorCard", "ProcessorCard");
	  if (res!=LC_Client_ResultOk) {
	    DBG_INFO(OPENSC_LOGDOMAIN, "here");
	    return SC_ERROR_TRANSMIT_FAILED;
	  }
	}
	else if ((strcasecmp(LC_Card_GetCardType(priv->card), "memory")==0) &&
		 GWEN_StringList_HasString(sl, "MemoryCard")) {
	  res=LC_Card_SelectCardAndApp(priv->card, "MemoryCard", "MemoryCard");
	  if (res!=LC_Client_ResultOk) {
	    DBG_INFO(OPENSC_LOGDOMAIN, "here");
	    return SC_ERROR_TRANSMIT_FAILED;
	  }
	}
      } /* if no special card */

      atr=LC_Card_GetAtr(priv->card);
      if (atr && GWEN_Buffer_GetUsedBytes(atr)) {
	memcpy(slot->atr,
	       GWEN_Buffer_GetStart(atr),
	       GWEN_Buffer_GetUsedBytes(atr));
	slot->atr_len=GWEN_Buffer_GetUsedBytes(atr);
      }
      else
	slot->atr_len=0;

    } /* if card not open */
  } /* if no card */
  return 0;
}



static int chipcard2_connect(struct sc_reader *reader,
			     struct sc_slot_info *slot){
  int rv;

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_connect(%p)", reader);

  rv=chipcard2_getcard(reader, slot);
  if (rv)
    return rv;

  return 0;
}



static int chipcard2_disconnect(struct sc_reader *reader,
				struct sc_slot_info *slot,
				int action){
  chipcard2_reader_data *priv = GET_PRIV_DATA(reader);

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_disconnect(%p)", reader);

  if (priv->card) {
    chipcard2_slot_data *pslot = GET_SLOT_DATA(slot);

    /* FIXME: check action */
    if (priv->isOpen) {
      LC_Card_Close(priv->card);
      priv->isOpen=0;
    }
    LC_Card_free(priv->card);
    priv->card=0;
    memset(pslot, 0, sizeof(*pslot));
    slot->flags=0;
  }

  return 0;
}



static int chipcard2_lock(struct sc_reader *reader,
			  struct sc_slot_info *slot){
  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_lock(%p)", reader);

  return 0;
}



static int chipcard2_unlock(struct sc_reader *reader,
			    struct sc_slot_info *slot){
  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_unlock(%p)", reader);

  return 0;
}



static int chipcard2_release(struct sc_reader *reader){
  chipcard2_reader_data *priv = GET_PRIV_DATA(reader);

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_release(%p)", reader);

  chipcard2_free_reader_data(priv);
  return 0;
}



/*
static int chipcard2_wait_for_event(struct sc_reader **readers,
				    struct sc_slot_info **slots,
				    size_t nslots,
				    unsigned int event_mask,
				    int *reader,
				    unsigned int *event, int timeout){
}
*/


static struct sc_reader_operations chipcard2_ops;

static struct sc_reader_driver chipcard2_drv = {
  "Libchipcard2 module",
  "chipcard2",
  &chipcard2_ops
};



static int chipcard2_init(struct sc_context *ctx, void **reader_data){
  int i;
  chipcard2_global_private_data *gpriv;
  scconf_block **blocks = NULL, *conf_block = NULL;
  int rv;
  const char *s;
  GWEN_LOGGER_LEVEL lv;

  DBG_DEBUG(OPENSC_LOGDOMAIN, "chipcard2_init");

  gpriv = (chipcard2_global_private_data *) malloc(sizeof(chipcard2_global_private_data));
  if (gpriv == NULL)
    return SC_ERROR_OUT_OF_MEMORY;
  memset(gpriv, 0, sizeof(*gpriv));
  *reader_data = gpriv;

  for (i = 0; ctx->conf_blocks[i] != NULL; i++) {
    blocks = scconf_find_blocks(ctx->conf, ctx->conf_blocks[i],
				"reader_driver", "chipcard2");
    conf_block = blocks[0];
    free(blocks);
    if (conf_block != NULL)
      break;
  }

  /* get and set log level */
  s=getenv("OPENSC_LOGLEVEL");
  if (!s)
    s="critical";
  lv=GWEN_Logger_Name2Level(s);
  GWEN_Logger_SetLevel(OPENSC_LOGDOMAIN, lv);
  /* create libchipcard2 client */
  gpriv->client=LC_Client_new("opensc", VERSION, 0);
  /* make libchipcard read its own configuration file */
  rv=LC_Client_ReadConfigFile(gpriv->client, 0);
  if (rv) {
    DBG_INFO(OPENSC_LOGDOMAIN, "here");
    chipcard2_free_global_data(gpriv);
    *reader_data=0;
    return SC_ERROR_CANNOT_LOAD_MODULE;
  }

  /* setup some fake readers */
  for (i=0; i<OPENSC_CHIPCARD2_MAXREADER; i++) {
    struct sc_reader *reader;
    chipcard2_reader_data *priv;
    chipcard2_slot_data *pslot;
    struct sc_slot_info *slot;
    char nameBuf[32];
    int r;

    reader=(struct sc_reader *) malloc(sizeof(struct sc_reader));
    priv=(chipcard2_reader_data *) malloc(sizeof(chipcard2_reader_data));
    pslot=(chipcard2_slot_data *) malloc(sizeof(chipcard2_slot_data));

    if (reader == NULL || priv == NULL || pslot == NULL) {
      if (reader)
	free(reader);
      chipcard2_free_reader_data(priv);
      chipcard2_free_slot_data(pslot);
      break;
    }

    memset(reader, 0, sizeof(*reader));
    memset(priv, 0, sizeof(*priv));
    memset(pslot, 0, sizeof(*pslot));

    reader->drv_data = priv;
    reader->ops = &chipcard2_ops;
    reader->driver = &chipcard2_drv;
    reader->slot_count = 1;
    snprintf(nameBuf, sizeof(nameBuf), "Libchipcard%d", i);
    reader->name = strdup(nameBuf);
    priv->gpriv = gpriv;
    DBG_DEBUG(OPENSC_LOGDOMAIN, "adding reader(%p)", reader);
    r = _sc_add_reader(ctx, reader);
    if (r) {
      chipcard2_free_reader_data(priv);
      free(reader->name);
      free(reader);
      chipcard2_free_slot_data(pslot);
      break;
    }
    slot = &reader->slot[0];
    memset(slot, 0, sizeof(*slot));
    slot->drv_data = pslot;
    memset(pslot, 0, sizeof(*pslot));

    //gpriv->readers[i]=priv;
  } /* for */



  return 0;
}



static int chipcard2_finish(struct sc_context *ctx, void *prv_data){
  chipcard2_global_private_data *gpriv;

  gpriv=(chipcard2_global_private_data *) prv_data;
  if (gpriv) {
    /* TODO: Release all cards */
    chipcard2_free_global_data(gpriv);
  }
  return 0;
}



struct sc_reader_driver * sc_get_chipcard2_driver(void){
  chipcard2_ops.init = chipcard2_init;
  chipcard2_ops.finish = chipcard2_finish;
  chipcard2_ops.transmit = chipcard2_transmit;
  chipcard2_ops.detect_card_presence = chipcard2_detect_card_presence;
  chipcard2_ops.lock = chipcard2_lock;
  chipcard2_ops.unlock = chipcard2_unlock;
  chipcard2_ops.release = chipcard2_release;
  chipcard2_ops.connect = chipcard2_connect;
  chipcard2_ops.disconnect = chipcard2_disconnect;
  chipcard2_ops.perform_verify = ctbcs_pin_cmd;
  //chipcard2_ops.wait_for_event = chipcard2_wait_for_event;

  return &chipcard2_drv;
}






/* module interface for OpenSC */

void *sc_module_init(const char *name) {
  DBG_DEBUG(OPENSC_LOGDOMAIN, "Libchipcard2s' OpenSC driver loaded");
  return sc_get_chipcard2_driver;
}



char *sc_driver_version() {
  return OPENSC_VERSION;
}



