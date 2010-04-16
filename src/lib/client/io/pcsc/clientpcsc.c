/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "clientpcsc_p.h"
#include "cardpcsc_l.h"

#include <chipcard/client/client_imp.h>

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inetsocket.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/text.h>

#define I18N(msg) msg



GWEN_INHERIT(LC_CLIENT, LC_CLIENT_PCSC)


#ifdef OS_WIN32
# include "mywinscard.c"
#endif




LC_CLIENT *LC_ClientPcsc_new(const char *programName,
                             const char *programVersion) {
  LC_CLIENT *cl;
  LC_CLIENT_PCSC *xcl;

#ifdef OS_WIN32
  if (MySCard_LoadLibrary("winscard.dll")) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Could not load WINSCARD.DLL");
    return 0;
  }
#endif

  cl=LC_BaseClient_new(LC_CLIENT_PCSC_NAME, programName, programVersion);
  if (cl==0)
    return cl;

  GWEN_NEW_OBJECT(LC_CLIENT_PCSC, xcl);
  GWEN_INHERIT_SETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl, xcl,
                       LC_ClientPcsc_FreeData);

  xcl->readers=LC_ReaderPcsc_List_new();

  xcl->initFn=LC_Client_SetInitFn(cl, LC_ClientPcsc_V_Init);
  xcl->finiFn=LC_Client_SetFiniFn(cl, LC_ClientPcsc_V_Fini);
  LC_Client_SetStartFn(cl, LC_ClientPcsc_V_Start);
  LC_Client_SetStopFn(cl, LC_ClientPcsc_V_Stop);
  LC_Client_SetGetNextCardFn(cl, LC_ClientPcsc_V_GetNextCard);
  LC_Client_SetReleaseCardFn(cl, LC_ClientPcsc_V_ReleaseCard);
  LC_Client_SetExecApduFn(cl, LC_ClientPcsc_V_ExecApdu);

  return cl;
}



void GWENHYWFAR_CB LC_ClientPcsc_FreeData(void *bp, void *p) {
  LC_CLIENT_PCSC *xcl;

  xcl=(LC_CLIENT_PCSC*)p;
  LC_ReaderPcsc_List_free(xcl->readers);
  GWEN_FREE_OBJECT(xcl);

#ifdef OS_WIN32
  /* don't worry, this function keeps a counter, it only unloads the
   * library if the counter reaches zero */
  MySCard_UnloadLibrary();
#endif
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Init(LC_CLIENT *cl,
                                                  GWEN_DB_NODE *db) {
  LC_CLIENT_PCSC *xcl;
  LONG rv;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  /* establish context */
  rv=SCardEstablishContext(SCARD_SCOPE_SYSTEM,    /* scope */
                           NULL,                  /* reserved1 */
                           NULL,                  /* reserved2 */
                           &(xcl->scardContext)); /* ptr to context */
  if (rv!=SCARD_S_SUCCESS) {
    if (rv == SCARD_E_NO_SERVICE) {
      DBG_ERROR(LC_LOGDOMAIN,
		"SCardEstablishContext: "
		"Error SCARD_E_NO_SERVICE: "
		"The Smartcard resource manager is not running. "
		"Maybe you have to start the Smartcard service manually?");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN,
		"SCardEstablishContext: %ld (%04lx)", rv,
		rv);
    }
    return LC_Client_ResultIoError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Fini(LC_CLIENT *cl) {
  LC_CLIENT_PCSC *xcl;
  LONG rv;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  LC_ReaderPcsc_List_Clear(xcl->readers);

  rv=SCardReleaseContext(xcl->scardContext);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN,
              "SCardReleaseContext: %04lx", rv);
    return LC_Client_ResultIoError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Start(LC_CLIENT *cl) {
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Stop(LC_CLIENT *cl) {
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientPcsc_ConnectReader(LC_CLIENT *cl,
                                             LC_READER_PCSC *r) {
  LC_CLIENT_PCSC *xcl;
  int res;
  LONG rv;
  SCARDHANDLE scardHandle;
  DWORD dwActiveProtocol;
  LC_CARD *card;
  char readerName[256];
  DWORD pcchReaderLen;
  BYTE pbAtr[MAX_ATR_SIZE];
  DWORD dwAtrLen;
  DWORD dwState;
  uint32_t rflags=0;
  uint32_t assumedRFlags=0;
  unsigned char rbuffer[300];
  DWORD rblen;
  GWEN_BUFFER *bDriverType;
  GWEN_BUFFER *bReaderType;


  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  DBG_INFO(LC_LOGDOMAIN,
           "Trying to connect \"%s\"",
           LC_ReaderPcsc_GetReaderName(r));
  rv=SCardConnect(xcl->scardContext,
                  LC_ReaderPcsc_GetReaderName(r),
                  SCARD_SHARE_EXCLUSIVE,
                  SCARD_PROTOCOL_T1,
                  &scardHandle,
                  &dwActiveProtocol);
  if (rv!=SCARD_S_SUCCESS)
    rv=SCardConnect(xcl->scardContext,
                    LC_ReaderPcsc_GetReaderName(r),
                    SCARD_SHARE_EXCLUSIVE,
                    SCARD_PROTOCOL_T0,
                    &scardHandle,
                    &dwActiveProtocol);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN,
             "SCardConnect: %04lx", rv);
    return LC_Client_ResultIoError;
  }

  DBG_INFO(LC_LOGDOMAIN,
           "Reader \"%s\" connected, setting up card data:",
           LC_ReaderPcsc_GetReaderName(r));

  /* get control codes */
  DBG_INFO(LC_LOGDOMAIN, "- reading control codes for CCID features");
  rv=SCardControl(scardHandle,
                  CM_IOCTL_GET_FEATURE_REQUEST,
                  NULL,
                  0,
                  rbuffer,
                  sizeof(rbuffer),
                  &rblen);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN,
             "SCardControl: %04lx", rv);
  }
  else {
    int cnt;
    PCSC_TLV_STRUCTURE *tlv;
    int i;

    cnt=rblen/sizeof(PCSC_TLV_STRUCTURE);
    if (cnt>LC_READER_PCSC_MAX_FEATURES)
      cnt=LC_READER_PCSC_MAX_FEATURES;
    tlv=(PCSC_TLV_STRUCTURE*)rbuffer;
    for (i=0; i<cnt; i++) {
      uint32_t v;

      v=tlv[i].value;
#ifdef LC_ENDIAN_LITTLE
      v=((v & 0xff000000)>>24) |
        ((v & 0x00ff0000)>>8) |
        ((v & 0x0000ff00)<<8) |
        ((v & 0x000000ff)<<24);
#endif
      DBG_INFO(LC_LOGDOMAIN, "Feature %d: %08x", tlv[i].tag, v);
      if (tlv[i].tag==FEATURE_VERIFY_PIN_DIRECT)
	assumedRFlags|=LC_READER_FLAGS_KEYPAD;
      LC_ReaderPcsc_SetFeatureCode(r, tlv[i].tag, v);
    }
  }

  /* get protocol and ATR */
  DBG_INFO(LC_LOGDOMAIN, "- reading protocol and ATR");
  pcchReaderLen=sizeof(readerName);
  dwAtrLen=sizeof(pbAtr);
  rv=SCardStatus(scardHandle,
                 readerName,
                 &pcchReaderLen,
                 &dwState,
                 &dwActiveProtocol,
                 pbAtr,
                 &dwAtrLen);

  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN,
              "SCardStatus: %04lx", rv);
    return LC_Client_ResultIoError;
  }

  /* derive reader and driver type from name */
  DBG_INFO(LC_LOGDOMAIN, "- getting reader- and driver type");
  bDriverType=GWEN_Buffer_new(0, 32, 0, 1);
  bReaderType=GWEN_Buffer_new(0, 32, 0, 1);
  res=LC_Client_GetReaderAndDriverType(cl, LC_ReaderPcsc_GetReaderName(r),
				       bDriverType, bReaderType, &rflags);
  if (res) {
    DBG_INFO(LC_LOGDOMAIN,
	     "Unable to determine type of reader [%s] (%d), assuming generic pcsc",
	     LC_ReaderPcsc_GetReaderName(r),
	     res);
    GWEN_Buffer_AppendString(bDriverType, "generic_pcsc");
    GWEN_Buffer_AppendString(bReaderType, "generic_pcsc");
    rflags=assumedRFlags;
  }

  /* create new card */
  card=LC_CardPcsc_new(cl, ++(xcl->lastCardId), scardHandle,
                       LC_ReaderPcsc_GetReaderName(r),
                       dwActiveProtocol,
                       "processor",      /* cardType */
                       rflags,           /* rflags */
                       dwAtrLen?pbAtr:0, /* atrBuf */
		       dwAtrLen);        /* atrLen */

  /* complete card data */
  LC_Card_SetDriverType(card, GWEN_Buffer_GetStart(bDriverType));
  LC_Card_SetReaderType(card, GWEN_Buffer_GetStart(bReaderType));

  GWEN_Buffer_free(bReaderType);
  GWEN_Buffer_free(bDriverType);

  DBG_INFO(LC_LOGDOMAIN, "Card data finished.");

  LC_ReaderPcsc_SetCurrentCard(r, card);

  return LC_Client_ResultOk;
}



LC_READER_PCSC *LC_ClientPcsc_FindReader(LC_CLIENT *cl, const char *rname) {
  LC_CLIENT_PCSC *xcl;
  LC_READER_PCSC *r;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  r=LC_ReaderPcsc_List_First(xcl->readers);
  while(r) {
    const char *s;

    s=LC_ReaderPcsc_GetReaderName(r);
    if (s && strcasecmp(s, rname)==0)
      break;
    r=LC_ReaderPcsc_List_Next(r);
  }

  return r;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_GetNextCard(LC_CLIENT *cl,
                                                         LC_CARD **pCard,
                                                         int timeout) {
  LC_CLIENT_PCSC *xcl;
  time_t startt;
  int distance;
  uint32_t progressId;
  uint64_t to;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  startt=time(0);
  assert(cl);

  if (timeout==GWEN_TIMEOUT_NONE ||
      timeout==GWEN_TIMEOUT_FOREVER)
    to=0;
  else
    to=timeout;

  progressId=GWEN_Gui_ProgressStart(GWEN_GUI_PROGRESS_DELAY |
				    GWEN_GUI_PROGRESS_ALLOW_EMBED |
				    GWEN_GUI_PROGRESS_SHOW_PROGRESS |
				    GWEN_GUI_PROGRESS_SHOW_ABORT,
				    I18N("Waiting for card to be inserted"),
				    NULL,
				    to,
				    0);

  if (timeout==GWEN_TIMEOUT_NONE)
    distance=GWEN_TIMEOUT_NONE;
  else if (timeout==GWEN_TIMEOUT_FOREVER)
    distance=timeout;
  else {
    distance=GWEN_GUI_CHECK_PERIOD;
    if (distance>timeout)
      distance=timeout;
  }

  for (;;) {
    LC_READER_PCSC *r=0;
    int rcount;
    double d;
    int err;

    if (xcl->lastUsedReader)
      r=LC_ReaderPcsc_List_Next(xcl->lastUsedReader);
    if (r==0) {
      LC_CLIENT_RESULT res;

      res=LC_ClientPcsc_ScanReaders(cl);
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      }
      r=LC_ReaderPcsc_List_First(xcl->readers);
    }
    if (r==0) {
      DBG_INFO(LC_LOGDOMAIN, "No readers available, waiting for readers");
    }
    else {
      /* cycle through the list of readers */
      rcount=LC_ReaderPcsc_List_GetCount(xcl->readers);

      while(rcount--) {
	if (LC_ReaderPcsc_GetCurrentCard(r)==0) {
	  LC_CLIENT_RESULT res;
    
	  res=LC_ClientPcsc_ConnectReader(cl, r);
	  if (res==LC_Client_ResultOk) {
	    xcl->lastUsedReader=r;
	    *pCard=LC_ReaderPcsc_GetCurrentCard(r);
	    GWEN_Gui_ProgressEnd(progressId);
	    return res;
	  }
	}
    
	r=LC_ReaderPcsc_List_Next(r);
	if (r==0)
	  r=LC_ReaderPcsc_List_First(xcl->readers);
      } /* while */
    }

    /* check timeout */
    d=difftime(time(0), startt);
    if (timeout!=GWEN_TIMEOUT_FOREVER) {
      if (timeout==GWEN_TIMEOUT_NONE ||
	  d>timeout) {
	DBG_INFO(GWEN_LOGDOMAIN,
		 "Timeout (%d) while waiting, giving up",
		 timeout);
	GWEN_Gui_ProgressEnd(progressId);
	return LC_Client_ResultWait;
      }
    }

    /* wait for a very little while */
    DBG_VERBOUS(0, "Waiting for %d ms", distance);
#ifdef OS_WIN32
    Sleep(distance);
#else
    GWEN_Socket_Select(0, 0, 0, distance);
#endif

    /* check for user abort */
    err=GWEN_Gui_ProgressAdvance(progressId, (uint64_t)(d*1000));
    if (err==GWEN_ERROR_USER_ABORTED) {
      DBG_ERROR(GWEN_LOGDOMAIN, "User aborted");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultAborted;
    }

  } /* for */

  /* no card available */
  return LC_Client_ResultWait;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_ReleaseCard(LC_CLIENT *cl,
                                                         LC_CARD *card) {

  LC_CLIENT_PCSC *xcl;
  LONG rv;
  LC_READER_PCSC *r;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  assert(card);
  r=LC_ClientPcsc_FindReader(cl, LC_CardPcsc_GetReaderName(card));
  if (!r) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Reader \"%s\" not found",
              LC_CardPcsc_GetReaderName(card));
    return LC_Client_ResultInvalid;
  }

  if (LC_ReaderPcsc_GetCurrentCard(r)==card)
    LC_ReaderPcsc_SetCurrentCard(r, 0);

  rv=SCardDisconnect(LC_CardPcsc_GetScardHandle(card),
                     SCARD_RESET_CARD);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN,
              "SCardDisconnect: %04lx", rv);
    return LC_Client_ResultIoError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_ExecApdu(LC_CLIENT *cl,
                                                      LC_CARD *card,
                                                      const char *apdu,
                                                      unsigned int apdulen,
                                                      GWEN_BUFFER *rbuf,
                                                      LC_CLIENT_CMDTARGET t,
                                                      int timeout) {
  LC_CLIENT_PCSC *xcl;
  LONG rv;
  LC_READER_PCSC *r;
  unsigned char rbuffer[300];
  DWORD rblen;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);

  assert(card);
  assert(apdu);
  assert(apdulen>3);

  r=LC_ClientPcsc_FindReader(cl, LC_CardPcsc_GetReaderName(card));
  if (!r) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Reader \"%s\" not found",
              LC_CardPcsc_GetReaderName(card));
    return LC_Client_ResultInvalid;
  }

  if (t==LC_Client_CmdTargetReader) {
    int feature;
    uint32_t controlCode;

    feature=apdu[0];
    controlCode=
        (apdu[1]<<24)+
        (apdu[2]<<16)+
        (apdu[3]<<8)+
      apdu[4];
    if (feature && controlCode==0)
      controlCode=LC_ReaderPcsc_GetFeatureCode(r, feature);
    if (controlCode==0) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Bad control code for feature %d of reader \"%s\"",
                feature,
                LC_CardPcsc_GetReaderName(card));
      return LC_Client_ResultInvalid;
    }

    DBG_DEBUG(LC_LOGDOMAIN, "Sending command to reader (control: %08x):",
              controlCode);
    GWEN_Text_LogString((const char*)apdu+5, apdulen-5,
                        LC_LOGDOMAIN,
			GWEN_LoggerLevel_Debug);

    rblen=sizeof(rbuffer);
    rv=SCardControl(LC_CardPcsc_GetScardHandle(card),
                    controlCode,
                    apdu+5,
                    apdulen-5,
                    rbuffer,
                    sizeof(rbuffer),
                    &rblen);
    if (rv!=SCARD_S_SUCCESS) {
      DBG_ERROR(LC_LOGDOMAIN,
                "SCardControl: %04lx", rv);
      return LC_Client_ResultIoError;
    }
    if (rblen) {
      GWEN_Buffer_AppendBytes(rbuf, (const char*)rbuffer, rblen);
      if (rblen>1) {
	LC_Card_SetLastResult(card, "ok",
			      "SCardControl succeeded",
			      rbuffer[rblen-2],
			      rbuffer[rblen-1]);
      }
    }
    return LC_Client_ResultOk;
  }
  else {
    SCARD_IO_REQUEST txHeader;
    SCARD_IO_REQUEST rxHeader;

    DBG_DEBUG(LC_LOGDOMAIN, "Sending command to card:");
    GWEN_Text_LogString((const char*)apdu, apdulen,
                        LC_LOGDOMAIN,
                        GWEN_LoggerLevel_Debug);
    txHeader.dwProtocol=LC_CardPcsc_GetProtocol(card);
    //txHeader.dwProtocol=1;
    txHeader.cbPciLength=sizeof(txHeader);
    rxHeader.cbPciLength=sizeof(rxHeader);
    rblen=sizeof(rbuffer);
    rv=SCardTransmit(LC_CardPcsc_GetScardHandle(card),
                     &txHeader,
                     (LPCBYTE) apdu,
                     apdulen,
                     &rxHeader,
                     rbuffer,
                     &rblen);
    if (rv!=SCARD_S_SUCCESS) {
      DBG_ERROR(LC_LOGDOMAIN,
                "SCardControl: %04lx", rv);
      return LC_Client_ResultIoError;
    }
    if (rblen) {
      GWEN_Buffer_AppendBytes(rbuf, (const char*)rbuffer, rblen);
      if (rblen>1) {
	LC_Card_SetLastResult(card, "ok",
			      "SCardTransmit succeeded",
			      rbuffer[rblen-2],
			      rbuffer[rblen-1]);
      }
    }
    return LC_Client_ResultOk;
  }
}



LC_CLIENT_RESULT LC_ClientPcsc_ScanReaders(LC_CLIENT *cl) {
  LC_CLIENT_PCSC *xcl;
  LONG rv;
  LPSTR mszGroups=0;
  LPSTR mszReaders=0;
  DWORD dwReaders=0;
  const char *p;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_PCSC, cl);
  assert(xcl);


  /* allocate buffer for reader list */
  rv=SCardListReaders(xcl->scardContext,  /* context */
                      NULL,               /* mszGroups */
                      NULL,               /* mszReaders */
                      &dwReaders);
  if (rv!=SCARD_S_SUCCESS) {
    if (rv==SCARD_E_NO_READERS_AVAILABLE) {
      DBG_ERROR(LC_LOGDOMAIN,
                "No readers available");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN,
		"SCardListReaders(1): %08lx", rv);
      SCardReleaseContext(xcl->scardContext);
    }
    return LC_Client_ResultIoError;
  }
  mszReaders=(LPSTR)malloc(sizeof(char)*dwReaders);
  if (mszReaders==0) {
    SCardReleaseContext(xcl->scardContext);
    return LC_Client_ResultInternal;
  }

  /* list readers */
  rv=SCardListReaders(xcl->scardContext,  /* context */
                      mszGroups,          /* mszGroups */
                      mszReaders,         /* mszReaders */
                      &dwReaders);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN,
              "SCardListReaders(2): %04lx", rv);
    SCardReleaseContext(xcl->scardContext);
    return LC_Client_ResultIoError;
  }

  /* create reader objects for every reader name */
  p=(const char*)mszReaders;
  while(*p) {
    LC_READER_PCSC *r;

    r=LC_ClientPcsc_FindReader(cl, p);
    if (r) {
      DBG_INFO(LC_LOGDOMAIN, "Reader \"%s\" already listed", p);
    }
    else {
      r=LC_ReaderPcsc_new(p);
      DBG_INFO(LC_LOGDOMAIN, "Creating reader \"%s\"", p);
      LC_ReaderPcsc_List_Add(r, xcl->readers);
    }
    while(*p)
      p++;
    p++;
  } /* while */
  free(mszReaders);

  return 0;
}


