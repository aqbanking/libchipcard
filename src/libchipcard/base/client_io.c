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

#include "client_xml.h"
#include "client_p.h"
#include "card_l.h"

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/i18n.h>


#define I18N(msg) GWEN_I18N_Translate(PACKAGE, msg)



static int _updateReaderStates(LC_CLIENT *cl);
static int _findReaderState(LC_CLIENT *cl, const char *readerName);
static int _connectCard(LC_CLIENT *cl, const char *readerName, LC_CARD **pCard);
static char *_getActiveReaderList(LC_CLIENT *cl);
static void _reassignReaderNamesAndRemoveStatesOfMissingReaders(LC_CLIENT *cl, const char *mszReaders);
static void _addNewReaders(LC_CLIENT *cl, const char *mszReaders);
static void _addReaderState(LC_CLIENT *cl, const char *readerName);
static const char *_findStringInMultiString(const char *mszString, const char *s);




int LC_Client_Start(LC_CLIENT *cl)
{
  LONG rv;

  assert(cl);

#if 0
  /* check whether pnp pseudo reader is available */
  cl->readerStates[0].szReader = "\\\\?PnP?\\Notification";
  cl->readerStates[0].dwCurrentState = SCARD_STATE_UNAWARE;
  rv=SCardGetStatusChange(cl->scardContext, 0, cl->readerStates, 1);

  if (cl->readerStates[0].dwEventState && SCARD_STATE_UNKNOWN) {
    DBG_INFO(LC_LOGDOMAIN, "PnP not supported");
    cl->pnpAvailable=0;
  }
  else
    cl->pnpAvailable=1;
#endif

  rv=_updateReaderStates(cl);
  if (rv<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", (int) rv);
    return rv;
  }
  cl->lastUsedReader=-1;

  return 0;
}



int LC_Client_Stop(LC_CLIENT *cl)
{
  assert(cl);

  /* clear reader list and reader status list */
  memset((void *) &cl->readerStates, 0, sizeof(SCARD_READERSTATE)*MAX_READERS);
  cl->readerCount=0;
  free(cl->readerList);
  cl->readerList=NULL;

  return 0;
}



int LC_Client_GetNextCard(LC_CLIENT *cl, LC_CARD **pCard, int timeout)
{
  LONG rv;
  int i;
  uint32_t progressId;
  time_t startt;
  int distance;

  assert(cl);

  startt=time(0);

  progressId=GWEN_Gui_ProgressStart(GWEN_GUI_PROGRESS_DELAY |
                                    GWEN_GUI_PROGRESS_ALLOW_EMBED |
                                    GWEN_GUI_PROGRESS_SHOW_PROGRESS |
                                    GWEN_GUI_PROGRESS_SHOW_ABORT,
                                    I18N("Waiting for card to be inserted"),
                                    NULL,
				    (timeout==GWEN_TIMEOUT_NONE || timeout==GWEN_TIMEOUT_FOREVER)?0:timeout,
                                    0);

  distance=GWEN_GUI_CHECK_PERIOD;
  if (distance>timeout)
    distance=timeout;

  for (;;) {
    double d;
    int err;

    /* continue checking */
    for (i=cl->lastUsedReader+1; i<cl->readerCount; i++) {
      /* we have a change here */
      if (cl->readerStates[i].dwEventState & SCARD_STATE_CHANGED)
        cl->readerStates[i].dwCurrentState=cl->readerStates[i].dwEventState;
      else
        continue;

      DBG_DEBUG(LC_LOGDOMAIN, "Status changed on reader [%s] (%08x, %08x)",
                cl->readerStates[i].szReader,
                (unsigned int)(cl->readerStates[i].dwCurrentState),
                (unsigned int)(cl->readerStates[i].dwEventState));

      if (cl->pnpAvailable && i==cl->readerCount-1) {
        /* pnp pseudo reader: a reader has been added or removed */
        DBG_DEBUG(LC_LOGDOMAIN, "Pseudo reader, updating reader list (%08x, %08x)",
                  (unsigned int)(cl->readerStates[i].dwCurrentState),
                  (unsigned int)(cl->readerStates[i].dwEventState));
        _updateReaderStates(cl);
        cl->lastUsedReader=-1;
        break;
      }
      else {
        if ((cl->readerStates[i].dwEventState & SCARD_STATE_PRESENT) &&
            !(cl->readerStates[i].dwEventState & SCARD_STATE_EXCLUSIVE) &&
            !(cl->readerStates[i].dwEventState & SCARD_STATE_INUSE)) {
          int res;
          LC_CARD *card=NULL;

          /* card inserted and not used by another application */
          DBG_DEBUG(LC_LOGDOMAIN, "Found usable card in reader [%s]", cl->readerStates[i].szReader);
          res=_connectCard(cl, cl->readerStates[i].szReader, &card);
          if (res==0) {
            /* card csuccessfully connected, return */
            *pCard=card;
            cl->lastUsedReader=i;
            GWEN_Gui_ProgressEnd(progressId);
            return 0;
          }
          else {
            DBG_ERROR(LC_LOGDOMAIN,
                      "Error connecting to card in reader [%s]",
                      cl->readerStates[i].szReader);
          }
        }
        else {
          DBG_INFO(LC_LOGDOMAIN,
                   "Either no card in reader or card unavailable in reader [%s]",
                   cl->readerStates[i].szReader);
        }
      }
    }

    if (i>=cl->readerCount) {
      /* there was no relevant change in a reader, wait for status change */
      cl->lastUsedReader=-1;
      rv=SCardGetStatusChange(cl->scardContext, distance, cl->readerStates, cl->readerCount);
      if (rv==SCARD_E_TIMEOUT) {
        /* timeout, just repeat next loop */
        if (timeout==GWEN_TIMEOUT_NONE) {
          GWEN_Gui_ProgressEnd(progressId);
          return GWEN_ERROR_TIMEOUT;
        }
      }
      else if (rv!=SCARD_S_SUCCESS) {
        DBG_ERROR(LC_LOGDOMAIN, "SCardGetStatusChange: %d", (int) rv);
        GWEN_Gui_ProgressEnd(progressId);
        return GWEN_ERROR_IO;
      }
    }

    /* check timeout */
    d=difftime(time(0), startt);
    if (timeout!=GWEN_TIMEOUT_FOREVER) {
      if (timeout==GWEN_TIMEOUT_NONE || d>timeout) {
        DBG_INFO(GWEN_LOGDOMAIN, "Timeout (%d) while waiting, giving up", timeout);
        GWEN_Gui_ProgressEnd(progressId);
        return GWEN_ERROR_TIMEOUT;
      }
    }

    /* check for user abort */
    err=GWEN_Gui_ProgressAdvance(progressId, (uint64_t)(d*1000));
    if (err==GWEN_ERROR_USER_ABORTED) {
      DBG_ERROR(GWEN_LOGDOMAIN, "User aborted");
      GWEN_Gui_ProgressEnd(progressId);
      return GWEN_ERROR_USER_ABORTED;
    }
  } /* for */
}



int LC_Client_ReleaseCard(LC_CLIENT *cl, LC_CARD *card)
{
  LONG rv;

  assert(cl);
  assert(card);

  rv=SCardDisconnect(LC_Card_GetSCardHandle(card), SCARD_RESET_CARD);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN, "SCardDisconnect: %04lx", (long unsigned int) rv);
    return GWEN_ERROR_IO;
  }

  return 0;
}



int LC_Client_ExecApdu(LC_CLIENT *cl,
                       LC_CARD *card,
                       const char *apdu,
                       unsigned int apdulen,
                       GWEN_BUFFER *rbuf,
                       LC_CLIENT_CMDTARGET t)
{
  LONG rv;
  unsigned char rbuffer[300];
  DWORD rblen;

  assert(cl);
  assert(card);
  assert(apdu);
  assert(apdulen>3);

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
      controlCode=LC_Card_GetFeatureCode(card, feature);

    if (controlCode==0) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Bad control code for feature %d of reader \"%s\"",
                feature,
                LC_Card_GetReaderName(card));
      return GWEN_ERROR_INVALID;
    }

    DBG_DEBUG(LC_LOGDOMAIN, "Sending command to reader (control: %08x):",
              controlCode);
    GWEN_Text_LogString((const char *)apdu+5, apdulen-5, LC_LOGDOMAIN, GWEN_LoggerLevel_Debug);

    rblen=sizeof(rbuffer);
    rv=SCardControl(LC_Card_GetSCardHandle(card),
                    controlCode,
                    apdu+5,
                    apdulen-5,
                    rbuffer,
                    sizeof(rbuffer),
                    &rblen);
    if (rv!=SCARD_S_SUCCESS) {
      DBG_ERROR(LC_LOGDOMAIN, "SCardControl: %04lx", (long unsigned int) rv);
      return GWEN_ERROR_IO;
    }
    if (rblen) {
      GWEN_Buffer_AppendBytes(rbuf, (const char *)rbuffer, rblen);
      if (rblen>1)
        LC_Card_SetLastResult(card, "ok", "SCardControl succeeded", rbuffer[rblen-2], rbuffer[rblen-1]);
    }
    return 0;
  }
  else {
    SCARD_IO_REQUEST txHeader;
    SCARD_IO_REQUEST rxHeader;

    DBG_DEBUG(LC_LOGDOMAIN, "Sending command to card:");
    GWEN_Text_LogString((const char *)apdu, apdulen, LC_LOGDOMAIN, GWEN_LoggerLevel_Debug);
    txHeader.dwProtocol=LC_Card_GetProtocol(card);
    //txHeader.dwProtocol=1;
    txHeader.cbPciLength=sizeof(txHeader);
    rxHeader.cbPciLength=sizeof(rxHeader);
    rblen=sizeof(rbuffer);
    rv=SCardTransmit(LC_Card_GetSCardHandle(card),
                     &txHeader,
                     (LPCBYTE) apdu,
                     apdulen,
                     &rxHeader,
                     rbuffer,
                     &rblen);
    if (rv!=SCARD_S_SUCCESS) {
      DBG_ERROR(LC_LOGDOMAIN, "SCardTransmit: %04lx", (long unsigned int) rv);
      return GWEN_ERROR_IO;
    }
    DBG_DEBUG(LC_LOGDOMAIN, "Received response:");
    GWEN_Text_LogString((const char *)rbuffer, rblen, LC_LOGDOMAIN, GWEN_LoggerLevel_Debug);
    if (rblen) {
      GWEN_Buffer_AppendBytes(rbuf, (const char *)rbuffer, rblen);
      if (rblen>1)
        LC_Card_SetLastResult(card, "ok", "SCardTransmit succeeded", rbuffer[rblen-2], rbuffer[rblen-1]);
    }
    else {
      DBG_DEBUG(LC_LOGDOMAIN, "Empty response");
    }
    return 0;
  }
}



int _findReaderState(LC_CLIENT *cl, const char *readerName)
{
  int i;

  assert(cl);
  for (i=0; i<cl->readerCount; i++) {
    if (strcasecmp(cl->readerStates[i].szReader, readerName)==0)
      return i;
  }

  return -1;
}



int _updateReaderStates(LC_CLIENT *cl)
{
  char *mszReaders;

  assert(cl);

  mszReaders=_getActiveReaderList(cl);
  if (mszReaders==NULL) {
    DBG_INFO(LC_LOGDOMAIN, "No readers available");
    return GWEN_ERROR_IO;
  }

  _reassignReaderNamesAndRemoveStatesOfMissingReaders(cl, mszReaders);

  /* replace reader string */
  free(cl->readerList);
  cl->readerList=mszReaders;

  _addNewReaders(cl, mszReaders);

  if (cl->pnpAvailable) {
    if (-1==_findReaderState(cl, "\\\\?PnP?\\Notification"))
      _addReaderState(cl, "\\\\?PnP?\\Notification"); /* add pnp reader */
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Too many readers (%d)", cl->readerCount);
    }
  }

  return 0;
}



void _reassignReaderNamesAndRemoveStatesOfMissingReaders(LC_CLIENT *cl, const char *mszReaders)
{
  int i;

  /* find reader */
  for (i=0; i<cl->readerCount; i++) {
    const char *p;

    p=_findStringInMultiString(mszReaders, cl->readerStates[i].szReader);
    if (p)
      /* re-assign reader name to point into the new mszString */
      cl->readerStates[i].szReader=p;
    else {
      int j;

      /* not in the reader list, remove */
      memset((void *) &(cl->readerStates[i]), 0, sizeof(SCARD_READERSTATE));
      for (j=i; j<(cl->readerCount-1); j++)
	cl->readerStates[j]=cl->readerStates[j+1];
      cl->readerCount--;
    }
  }
}



void _addNewReaders(LC_CLIENT *cl, const char *mszReaders)
{
  const char *p;

  /* add new readers  */
  p=(const char *)mszReaders;
  while (*p) {
    if (_findReaderState(cl, p)!=-1) {
      DBG_INFO(LC_LOGDOMAIN, "Reader \"%s\" already listed", p);
    }
    else
      _addReaderState(cl, p);
    /* next reader */
    while (*p)
      p++;
    p++;
  } /* while */
}



void _addReaderState(LC_CLIENT *cl, const char *readerName)
{
  if (cl->readerCount<MAX_READERS) {
    int i;

    DBG_INFO(LC_LOGDOMAIN, "Adding reader \"%s\"", readerName);
    i=cl->readerCount;
    /* preset */
    memset((void *) &(cl->readerStates[i]), 0, sizeof(SCARD_READERSTATE));
    cl->readerStates[i].szReader=readerName;
    cl->readerStates[i].dwCurrentState=SCARD_STATE_UNAWARE;
    /* reader added */
    cl->readerCount++;
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Not adding reader \"%p\", too many readers (%d)", readerName, cl->readerCount);
  }
}


const char *_findStringInMultiString(const char *mszString, const char *s)
{
  while(*mszString) {
    if (strcasecmp(mszString, s)==0)
      return mszString;
    while(*mszString)
      mszString++;
    mszString++;
  }

  return NULL;
}



char *_getActiveReaderList(LC_CLIENT *cl)
{
  LONG rv;
  LPSTR mszGroups=0;
  LPSTR mszReaders=0;
  DWORD dwReaders=0;

  assert(cl);

  /* allocate buffer for reader list */
  rv=SCardListReaders(cl->scardContext,   /* context */
                      NULL,               /* mszGroups */
                      NULL,               /* mszReaders */
                      &dwReaders);
  if (rv!=SCARD_S_SUCCESS) {
    if (rv==SCARD_E_NO_READERS_AVAILABLE) {
      DBG_ERROR(LC_LOGDOMAIN, "No readers available");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "SCardListReaders(1): %08lx", (long unsigned int) rv);
    }
    return NULL;
  }
  mszReaders=(LPSTR)malloc(sizeof(char)*dwReaders);
  assert(mszReaders);

  /* list readers */
  rv=SCardListReaders(cl->scardContext,   /* context */
                      mszGroups,          /* mszGroups */
                      mszReaders,         /* mszReaders */
                      &dwReaders);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN, "SCardListReaders(2): %04lx", (long unsigned int) rv);
    return NULL;
  }

  return mszReaders;
}



int _connectCard(LC_CLIENT *cl, const char *rname, LC_CARD **pCard)
{
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
  GWEN_BUFFER *bDriverType;
  GWEN_BUFFER *bReaderType;
  uint32_t rflags=0;

  assert(cl);

  DBG_INFO(LC_LOGDOMAIN, "Trying protocol T1");
  rv=SCardConnect(cl->scardContext,
                  rname,
                  SCARD_SHARE_EXCLUSIVE,
                  SCARD_PROTOCOL_T1,
                  &scardHandle,
                  &dwActiveProtocol);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN, "Trying protocol T0");
    rv=SCardConnect(cl->scardContext,
                    rname,
                    SCARD_SHARE_EXCLUSIVE,
                    SCARD_PROTOCOL_T0,
                    &scardHandle,
                    &dwActiveProtocol);
  }
#ifdef SCARD_PROTOCOL_RAW
  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN, "Trying protocol RAW");
    rv=SCardConnect(cl->scardContext,
                    rname,
                    SCARD_SHARE_EXCLUSIVE,
                    SCARD_PROTOCOL_RAW,
                    &scardHandle,
                    &dwActiveProtocol);
  }
#endif

  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN, "SCardConnect: %04lx", (long unsigned int) rv);
    return GWEN_ERROR_IO;
  }

  /* get protocol and ATR */
  DBG_INFO(LC_LOGDOMAIN, "Reading protocol and ATR");
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
    DBG_ERROR(LC_LOGDOMAIN, "SCardStatus: %04lx", (long unsigned int) rv);
    SCardDisconnect(scardHandle, SCARD_UNPOWER_CARD);
    return GWEN_ERROR_IO;
  }

  /* derive reader and driver type from name */
  DBG_INFO(LC_LOGDOMAIN, "Getting reader- and driver type");
  bDriverType=GWEN_Buffer_new(0, 32, 0, 1);
  bReaderType=GWEN_Buffer_new(0, 32, 0, 1);
  res=LC_Client_GetReaderAndDriverType(cl, readerName, bDriverType, bReaderType, &rflags);
  if (res) {
    DBG_INFO(LC_LOGDOMAIN,
             "Unable to determine type of reader [%s] (%d), assuming generic pcsc",
             readerName,
             res);
    GWEN_Buffer_AppendString(bDriverType, "generic_pcsc");
    GWEN_Buffer_AppendString(bReaderType, "generic_pcsc");
  }

  /* create new card */
  card=LC_Card_new(cl,
                   scardHandle,
                   readerName,
                   dwActiveProtocol,
                   "processor",      /* cardType */
                   rflags,
                   dwAtrLen?pbAtr:0, /* atrBuf */
                   dwAtrLen);        /* atrLen */

  /* complete card data */
  LC_Card_SetDriverType(card, GWEN_Buffer_GetStart(bDriverType));
  LC_Card_SetReaderType(card, GWEN_Buffer_GetStart(bReaderType));

  GWEN_Buffer_free(bReaderType);
  GWEN_Buffer_free(bDriverType);

  *pCard=card;

  return 0;
}



