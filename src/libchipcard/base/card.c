/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2022 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "card_p.h"
#include "client_l.h"
#include "client_cmd.h"
#include "client_xml.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>

#ifdef OS_WIN32
# define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)

# define FEATURE_VERIFY_PIN_START  0x01 /* OMNIKEY Proposal */
# define FEATURE_VERIFY_PIN_FINISH 0x02 /* OMNIKEY Proposal */
# define FEATURE_MODIFY_PIN_START  0x03 /* OMNIKEY Proposal */
# define FEATURE_MODIFY_PIN_FINISH 0x04 /* OMNIKEY Proposal */
# define FEATURE_GET_KEY_PRESSED   0x05 /* OMNIKEY Proposal */
# define FEATURE_VERIFY_PIN_DIRECT 0x06 /* USB CCID PIN Verify */
# define FEATURE_MODIFY_PIN_DIRECT 0x07 /* USB CCID PIN Modify */
# define FEATURE_MCT_READERDIRECT  0x08 /* KOBIL Proposal */
# define FEATURE_MCT_UNIVERSAL     0x09 /* KOBIL Proposal */
# define FEATURE_IFD_PIN_PROP      0x0A /* Gemplus Proposal */
# define FEATURE_ABORT             0x0B /* SCM Proposal */

/* Set structure elements aligment on bytes
 * http://gcc.gnu.org/onlinedocs/gcc/Structure_002dPacking-Pragmas.html */
#pragma pack(push, 1)

/* the structure must be 6-bytes long */
typedef struct {
  uint8_t tag;
  uint8_t length;
  uint32_t value;
} PCSC_TLV_STRUCTURE;

#pragma pack(pop)


#elif defined (OS_DARWIN)
# define SCARD_CTL_CODE(code) (0x42000000 + (code))
# define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)

# define FEATURE_VERIFY_PIN_START  0x01 /* OMNIKEY Proposal */
# define FEATURE_VERIFY_PIN_FINISH 0x02 /* OMNIKEY Proposal */
# define FEATURE_MODIFY_PIN_START  0x03 /* OMNIKEY Proposal */
# define FEATURE_MODIFY_PIN_FINISH 0x04 /* OMNIKEY Proposal */
# define FEATURE_GET_KEY_PRESSED   0x05 /* OMNIKEY Proposal */
# define FEATURE_VERIFY_PIN_DIRECT 0x06 /* USB CCID PIN Verify */
# define FEATURE_MODIFY_PIN_DIRECT 0x07 /* USB CCID PIN Modify */
# define FEATURE_MCT_READERDIRECT  0x08 /* KOBIL Proposal */
# define FEATURE_MCT_UNIVERSAL     0x09 /* KOBIL Proposal */
# define FEATURE_IFD_PIN_PROP      0x0A /* Gemplus Proposal */
# define FEATURE_ABORT             0x0B /* SCM Proposal */

/* Set structure elements aligment on bytes
 * http://gcc.gnu.org/onlinedocs/gcc/Structure_002dPacking-Pragmas.html */
#ifdef __APPLE__
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

/* the structure must be 6-bytes long */
typedef struct {
  uint8_t tag;
  uint8_t length;
  uint32_t value;
} PCSC_TLV_STRUCTURE;

#ifdef __APPLE__
#pragma pack()
#else
#pragma pack(pop)
#endif

#else
# include <PCSC/reader.h>
#endif


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CARD, LC_Card)
GWEN_INHERIT_FUNCTIONS(LC_CARD)
GWEN_LIST2_FUNCTIONS(LC_CARD, LC_Card)



static int CHIPCARD_CB _cardOpen(LC_CARD *card);
static int CHIPCARD_CB _cardClose(LC_CARD *card);




LC_CARD *LC_Card_new(LC_CLIENT *cl,
                     SCARDHANDLE scardHandle,
                     const char *readerName,
                     DWORD protocol,
                     const char *cardType,
                     uint32_t rflags,
                     const unsigned char *atrBuf,
                     unsigned int atrLen)
{
  LC_CARD *cd;

  assert(cl);
  assert(cardType);

  GWEN_NEW_OBJECT(LC_CARD, cd);
  GWEN_LIST_INIT(LC_CARD, cd);
  GWEN_INHERIT_INIT(LC_CARD, cd);
  cd->client=cl;
  cd->cardType=strdup(cardType);
  cd->readerFlags=rflags;
  cd->cardTypes=GWEN_StringList_new();
  cd->dbCommandCache=GWEN_DB_Group_new("commandCache");
  cd->usage=1;
  if (atrBuf && atrLen) {
    cd->atr=GWEN_Buffer_new(0, atrLen, 0, 1);
    GWEN_Buffer_AppendBytes(cd->atr, (const char *)atrBuf, atrLen);
  }

  cd->openFn=_cardOpen;
  cd->closeFn=_cardClose;

  /* determine card types by comparing the ATR to known ATRs */
  if (cd->atr) {
    int rv;

    rv=LC_Client_AddCardTypesByAtr(cl, cd);
    if (rv) {
      if (rv==1) {
        DBG_WARN(LC_LOGDOMAIN, "Unknown card type (no matching ATR)");
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Error determining card types");
      }
    }
  }

  cd->readerName=strdup(readerName);
  cd->scardHandle=scardHandle;
  cd->protocol=protocol;

  return cd;
}



void LC_Card_free(LC_CARD *cd)
{
  if (cd) {
    assert(cd->usage>0);
    cd->usage--;
    if (cd->usage==0) {
      GWEN_INHERIT_FINI(LC_CARD, cd);
      free(cd->readerName);
      free(cd->cardType);
      free(cd->lastResult);
      free(cd->lastText);
      free(cd->readerType);
      free(cd->driverType);
      GWEN_StringList_free(cd->cardTypes);
      GWEN_Buffer_free(cd->atr);
      GWEN_DB_Group_free(cd->dbCommandCache);
      GWEN_LIST_FINI(LC_CARD, cd);
      GWEN_FREE_OBJECT(cd);
    }
  }
}



void LC_Card_List2_freeAll(LC_CARD_LIST2 *l)
{
  if (l) {
    LC_CARD_LIST2_ITERATOR *cit;

    cit=LC_Card_List2_First(l);
    if (cit) {
      LC_CARD *card;

      card=LC_Card_List2Iterator_Data(cit);
      while (card) {
        LC_CARD *next;

        next=LC_Card_List2Iterator_Next(cit);
        LC_Card_free(card);
        card=next;
      } /* while */
      LC_Card_List2Iterator_free(cit);
    }
    LC_Card_List2_free(l);
  }
}



SCARDHANDLE LC_Card_GetSCardHandle(const LC_CARD *card)
{
  assert(card);
  return card->scardHandle;
}



const char *LC_Card_GetReaderType(const LC_CARD *cd)
{
  assert(cd);
  return cd->readerType;
}



void LC_Card_SetReaderType(LC_CARD *cd, const char *s)
{
  assert(cd);
  free(cd->readerType);
  if (s)
    cd->readerType=strdup(s);
  else
    cd->readerType=0;
}



const char *LC_Card_GetDriverType(const LC_CARD *cd)
{
  assert(cd);
  return cd->driverType;
}



void LC_Card_SetDriverType(LC_CARD *cd, const char *s)
{
  assert(cd);
  free(cd->driverType);
  if (s)
    cd->driverType=strdup(s);
  else
    cd->driverType=0;
}



const GWEN_STRINGLIST *LC_Card_GetCardTypes(const LC_CARD *cd)
{
  assert(cd);
  return cd->cardTypes;
}



int LC_Card_AddCardType(LC_CARD *cd, const char *s)
{
  assert(cd);
  return GWEN_StringList_AppendString(cd->cardTypes, s, 0, 1);
}



void LC_Card_SetLastResult(LC_CARD *cd,
                           const char *result,
                           const char *text,
                           int sw1, int sw2)
{
  assert(cd);
  free(cd->lastResult);
  free(cd->lastText);
  if (result)
    cd->lastResult=strdup(result);
  else
    cd->lastResult=0;
  if (text)
    cd->lastText=strdup(text);
  else
    cd->lastText=0;
  cd->lastSW1=sw1;
  cd->lastSW2=sw2;
}


int LC_Card_GetLastSW1(const LC_CARD *cd)
{
  assert(cd);
  return cd->lastSW1;
}



int LC_Card_GetLastSW2(const LC_CARD *cd)
{
  assert(cd);
  return cd->lastSW2;
}



const char *LC_Card_GetLastResult(const LC_CARD *cd)
{
  assert(cd);
  return cd->lastResult;
}



const char *LC_Card_GetLastText(const LC_CARD *cd)
{
  assert(cd);
  return cd->lastText;
}



LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd)
{
  assert(cd);
  return cd->client;
}



uint32_t LC_Card_GetReaderFlags(const LC_CARD *cd)
{
  assert(cd);
  return cd->readerFlags;
}



const char *LC_Card_GetCardType(const LC_CARD *cd)
{
  assert(cd);
  return cd->cardType;
}



void LC_Card_SetCardType(LC_CARD *cd, const char *ct)
{
  assert(cd);
  assert(ct);

  free(cd->cardType);
  cd->cardType=strdup(ct);
}



unsigned int LC_Card_GetAtr(const LC_CARD *cd, const unsigned char **pbuf)
{
  assert(cd);
  if (cd->atr) {
    unsigned int len;

    len=GWEN_Buffer_GetUsedBytes(cd->atr);
    if (len) {
      *pbuf=(const unsigned char *)GWEN_Buffer_GetStart(cd->atr);
      return len;
    }
  }
  return 0;
}



uint32_t LC_Card_GetFeatureCode(const LC_CARD *cd, int idx)
{
  assert(cd);
  assert(idx<LC_PCSC_MAX_FEATURES);
  return cd->featureCode[idx];
}



const char *LC_Card_GetReaderName(const LC_CARD *card)
{
  assert(card);
  return card->readerName;
}



DWORD LC_Card_GetProtocol(const LC_CARD *card)
{
  assert(card);
  return card->protocol;
}



void LC_Card_Dump(const LC_CARD *cd, int insert)
{
  int k;
  GWEN_STRINGLISTENTRY *se;
  GWEN_DB_NODE *dbT;

  assert(cd);
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Card\n");
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr,
          "==================="
          "==================="
          "==================="
          "==================\n");
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Card type     : %s\n", cd->cardType);
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Driver type   : %s\n", cd->driverType);
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Reader type   : %s\n", cd->readerType);
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Card types    :");
  se=GWEN_StringList_FirstEntry(cd->cardTypes);
  while (se) {
    const char *s;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    fprintf(stderr, " %s", s);
    se=GWEN_StringListEntry_Next(se);
  } /* while */
  fprintf(stderr, "\n");
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr, "Reader flags  : ");

  dbT=GWEN_DB_Group_new("flags");
  LC_ReaderFlags_toDb(dbT, "flags", cd->readerFlags);
  for (k=0; k<32; k++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbT, "flags", k, 0);
    if (!s)
      break;
    if (k)
      fprintf(stderr, ", ");
    fprintf(stderr, "%s", s);
  }
  fprintf(stderr, "\n");
  GWEN_DB_Group_free(dbT);

  if (cd->atr) {
    for (k=0; k<insert; k++)
      fprintf(stderr, " ");
    fprintf(stderr, "ATR\n");
    for (k=0; k<insert; k++)
      fprintf(stderr, " ");
    fprintf(stderr,
            "-------------------"
            "-------------------"
            "-------------------"
            "------------------\n");
    GWEN_Text_DumpString(GWEN_Buffer_GetStart(cd->atr),
                         GWEN_Buffer_GetUsedBytes(cd->atr),
                         insert+2);
  }
  for (k=0; k<insert; k++)
    fprintf(stderr, " ");
  fprintf(stderr,
          "==================="
          "==================="
          "==================="
          "==================\n");
}



int LC_Card_ReadFeatures(LC_CARD *card)
{
  LONG rv;
  unsigned char rbuffer[300];
  DWORD rblen;

  assert(card);

  /* get control codes */
  DBG_INFO(LC_LOGDOMAIN, "Reading control codes for CCID features");
  rv=SCardControl(card->scardHandle,
                  CM_IOCTL_GET_FEATURE_REQUEST,
                  NULL,
                  0,
                  rbuffer,
                  sizeof(rbuffer),
                  &rblen);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_INFO(LC_LOGDOMAIN,
             "SCardControl: %04lx", (long unsigned int) rv);
  }
  else {
    int cnt;
    PCSC_TLV_STRUCTURE *tlv;
    int i;

    /* clear keypad flag; if there is TLV indicating the reader has a keypad and
     * the driver supports it we set the flag upon encounter of the tlv */
    card->readerFlags&=~LC_READER_FLAGS_KEYPAD;
    cnt=rblen/sizeof(PCSC_TLV_STRUCTURE);
    tlv=(PCSC_TLV_STRUCTURE *)rbuffer;
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
        card->readerFlags|=LC_READER_FLAGS_KEYPAD;
      if (tlv[i].tag<LC_PCSC_MAX_FEATURES) {
        card->featureCode[tlv[i].tag]=v;
      }
    }
  }

  /* done */
  return 0;
}



int LC_Card_Open(LC_CARD *card)
{
  LONG rv;

  assert(card);

  rv=LC_Card_ReadFeatures(card);
  if (rv<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", (int) rv);
  }

  LC_Card_SetLastResult(card, 0, 0, -1, -1);
  if (!card->openFn) {
    DBG_DEBUG(LC_LOGDOMAIN, "No OpenFn set");
    return 0;
  }
  return card->openFn(card);
}



int LC_Card_Close(LC_CARD *card)
{
  int res;

  assert(card);
  LC_Card_SetLastResult(card, 0, 0, -1, -1);
  if (!card->closeFn) {
    DBG_DEBUG(LC_LOGDOMAIN, "No CloseFn set");
    res=0;
  }
  else
    res=card->closeFn(card);
  return res;
}



int _cardOpen(LC_CARD *card)
{
  return 0;
}



int _cardClose(LC_CARD *card)
{
  return 0;
}



int LC_Card_ExecApdu(LC_CARD *card,
                     const char *apdu,
                     unsigned int len,
                     GWEN_BUFFER *rbuf,
                     LC_CLIENT_CMDTARGET t)
{
  assert(card);
  assert(card->client);
  LC_Card_SetLastResult(card, 0, 0, -1, -1);
  return LC_Client_ExecApdu(card->client, card, apdu, len, rbuf, t);
}



int LC_Card_ExecCommand(LC_CARD *card,
                        const char *commandName,
                        GWEN_DB_NODE *cmdData,
                        GWEN_DB_NODE *rspData)
{
  int res;

  assert(card);
  assert(card->client);
  LC_Card_SetLastResult(card, 0, 0, -1, -1);
  res=LC_Client_ExecCommand(card->client, card, commandName, cmdData, rspData);
  return res;
}



GWEN_XMLNODE *LC_Card_FindCommand(LC_CARD *card, const char *commandName)
{
  GWEN_DB_NODE *db;
  GWEN_XMLNODE *node;

  assert(card);
  assert(commandName);

  db=card->dbCommandCache;
  if (card->driverType) {
    db=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, card->driverType);
    assert(db);
  }
  if (card->readerType) {
    db=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, card->readerType);
    assert(db);
  }

  node=(GWEN_XMLNODE *)GWEN_DB_GetPtrValue(db, commandName, 0, 0);
  if (node==NULL) {
    node=LC_Client_FindCardCommand(card->client, card, commandName);
    if (node)
      GWEN_DB_SetPtrValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, commandName, (void *) node);
  }
  else {
    DBG_INFO(LC_LOGDOMAIN, "Found command \"%s\" in cache", commandName);
  }

  return node;
}



int LC_Card_BuildApdu(LC_CARD *card,
                      const char *command,
                      GWEN_DB_NODE *cmdData,
                      GWEN_BUFFER *gbuf)
{
  assert(card);
  assert(card->client);
  return LC_Client_BuildApdu(card->client, card, command, cmdData, gbuf);

}



int LC_Card_SelectApp(LC_CARD *card, const char *appName)
{
  GWEN_XMLNODE *node;

  node=LC_Client_GetAppNode(card->client, appName);
  if (node==0) {
    DBG_INFO(LC_LOGDOMAIN, "App not found");
    return GWEN_ERROR_NOT_FOUND;
  }
  card->appNode=node;
  card->dfNode=NULL;
  card->efNode=NULL;
  return 0;
}



GWEN_XMLNODE *LC_Card_GetAppNode(const LC_CARD *card)
{
  assert(card);
  return card->appNode;
}



int LC_Card_SelectCard(LC_CARD *card, const char *s)
{
  assert(card);
  if (s==NULL)
    card->cardNode=NULL;
  else {
    GWEN_XMLNODE *node;

    node=LC_Client_GetCardNode(card->client, s);
    if (node==0) {
      DBG_INFO(LC_LOGDOMAIN, "Card type not found");
      return GWEN_ERROR_NOT_FOUND;
    }
    card->cardNode=node;
    DBG_INFO(LC_LOGDOMAIN, "Clearing command cache");
    GWEN_DB_ClearGroup(card->dbCommandCache, NULL);
  }
  return 0;
}



GWEN_XMLNODE *LC_Card_GetCardNode(const LC_CARD *card)
{
  assert(card);
  return card->cardNode;
}






LC_PININFO *LC_Card_GetPinInfoById(LC_CARD *card, uint32_t pid)
{
  GWEN_XMLNODE *n;

  n=card->efNode;
  if (!n)
    n=card->dfNode;
  if (!n)
    n=card->appNode;
  if (!n) {
    DBG_INFO(LC_LOGDOMAIN, "No XML node");
    return 0;
  }

  while (n) {
    GWEN_XMLNODE *nn;

    nn=GWEN_XMLNode_FindFirstTag(n, "pins", 0, 0);
    while (nn) {
      GWEN_XMLNODE *nnn;

      nnn=GWEN_XMLNode_FindFirstTag(nn, "pin", 0, 0);
      while (nnn) {
        const char *s;

        s=GWEN_XMLNode_GetProperty(nnn, "id", 0);
        if (s) {
          int i;

          if (sscanf(s, "%i", &i)==1) {
            if (i==(int)pid) {
              LC_PININFO *pi;

              pi=LC_PinInfo_new();
              LC_PinInfo_SetId(pi, pid);
              LC_PinInfo_SetName(pi, GWEN_XMLNode_GetProperty(nnn, "name", NULL));
              LC_PinInfo_SetMinLength(pi, GWEN_XMLNode_GetIntProperty(nnn, "minLen", 0));
              LC_PinInfo_SetMaxLength(pi, GWEN_XMLNode_GetIntProperty(nnn, "maxLen", 0));
              LC_PinInfo_SetAllowChange(pi, GWEN_XMLNode_GetIntProperty(nnn, "allowChange", 0));
              LC_PinInfo_SetFiller(pi, GWEN_XMLNode_GetIntProperty(nnn, "filler", 0));
              s=GWEN_XMLNode_GetProperty(nnn, "encoding", 0);
              if (s)
                LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_fromString(s));
              return pi;
            }
          }
        }
        nnn=GWEN_XMLNode_FindNextTag(nnn, "pin", 0, 0);
      }

      nn=GWEN_XMLNode_FindNextTag(nn, "pins", 0, 0);
    }

    n=GWEN_XMLNode_GetParent(n);
  }

  return 0;
}



LC_PININFO *LC_Card_GetPinInfoByName(LC_CARD *card, const char *name)
{
  GWEN_XMLNODE *n;

  assert(card);
  assert(card->usage);

  n=card->efNode;
  if (!n) {
    DBG_DEBUG(LC_LOGDOMAIN, "No EF node");
    n=card->dfNode;
  }
  if (!n) {
    DBG_DEBUG(LC_LOGDOMAIN, "No DF node");
    n=card->appNode;
  }
  if (!n) {
    DBG_INFO(LC_LOGDOMAIN, "No XML node");
    return 0;
  }

  while (n) {
    GWEN_XMLNODE *nn;

    DBG_DEBUG(LC_LOGDOMAIN, "Searching in \"%s\" (%s)",
              GWEN_XMLNode_GetProperty(n, "name", "(none)"),
              GWEN_XMLNode_GetData(n));

    nn=GWEN_XMLNode_FindFirstTag(n, "pins", 0, 0);
    while (nn) {
      GWEN_XMLNODE *nnn;

      nnn=GWEN_XMLNode_FindFirstTag(nn, "pin", 0, 0);
      while (nnn) {
        const char *s;
        int i;

        s=GWEN_XMLNode_GetProperty(nnn, "id", 0);
        if (s && sscanf(s, "%i", &i)==1) {
          s=GWEN_XMLNode_GetProperty(nnn, "name", 0);
          if (s && strcasecmp(s, name)==0) {
            LC_PININFO *pi;

            pi=LC_PinInfo_new();
            LC_PinInfo_SetId(pi, (uint32_t)i);

            LC_PinInfo_SetName(pi, GWEN_XMLNode_GetProperty(nnn, "name", NULL));
            LC_PinInfo_SetMinLength(pi, GWEN_XMLNode_GetIntProperty(nnn, "minLen", 0));
            LC_PinInfo_SetMaxLength(pi, GWEN_XMLNode_GetIntProperty(nnn, "maxLen", 0));
            LC_PinInfo_SetAllowChange(pi, GWEN_XMLNode_GetIntProperty(nnn, "allowChange", 0));
            LC_PinInfo_SetFiller(pi, GWEN_XMLNode_GetIntProperty(nnn, "filler", 0));

            s=GWEN_XMLNode_GetProperty(nnn, "encoding", 0);
            if (s)
              LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_fromString(s));
            return pi;
          }
        }
        nnn=GWEN_XMLNode_FindNextTag(nnn, "pin", 0, 0);
      }

      nn=GWEN_XMLNode_FindNextTag(nn, "pins", 0, 0);
    }

    n=GWEN_XMLNode_GetParent(n);
  }

  return 0;
}



int LC_Card_GetPinStatus(LC_CARD *card,
                         unsigned int pid,
                         int *maxErrors,
                         int *currentErrors)
{
  assert(card);
  if (card->getPinStatusFn) {
    return card->getPinStatusFn(card, pid, maxErrors, currentErrors);
  }
  else {
    DBG_INFO(LC_LOGDOMAIN, "no getPinStatus function set");
    return GWEN_ERROR_NOT_SUPPORTED;
  }
}



int LC_Card_GetInitialPin(LC_CARD *card,
                          int id,
                          unsigned char *buffer,
                          unsigned int maxLen,
                          unsigned int *pinLength)
{
  assert(card);
  if (card->getInitialPinFn) {
    return card->getInitialPinFn(card, id, buffer, maxLen, pinLength);
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "no getInitialPin function set");
    return GWEN_ERROR_NOT_SUPPORTED;
  }
}



LC_CARD_OPEN_FN LC_Card_GetOpenFn(const LC_CARD *card)
{
  assert(card);
  return card->openFn;
}



void LC_Card_SetOpenFn(LC_CARD *card, LC_CARD_OPEN_FN fn)
{
  assert(card);
  card->openFn=fn;
}



LC_CARD_CLOSE_FN LC_Card_GetCloseFn(const LC_CARD *card)
{
  assert(card);
  return card->closeFn;
}



void LC_Card_SetCloseFn(LC_CARD *card, LC_CARD_CLOSE_FN fn)
{
  assert(card);
  card->closeFn=fn;
}



void LC_Card_SetGetPinStatusFn(LC_CARD *card, LC_CARD_GETPINSTATUS_FN fn)
{
  assert(card);
  card->getPinStatusFn=fn;
}



void LC_Card_SetGetInitialPinFn(LC_CARD *card, LC_CARD_GETINITIALPIN_FN fn)
{
  assert(card);
  card->getInitialPinFn=fn;
}



void LC_Card_CreateResultString(const LC_CARD *card,
                                const char *lastCommand,
                                int res,
                                GWEN_BUFFER *buf)
{
  const char *s;

  switch (res) {
  case 0:
    s="Ok.";
    break;
  case GWEN_ERROR_TIMEOUT:
    s="Timeout.";
    break;
  case GWEN_ERROR_IO:
    s="IPC error.";
    break;
  case GWEN_ERROR_BAD_DATA:
    s="Data error.";
    break;
  case GWEN_ERROR_USER_ABORTED:
    s="Aborted.";
    break;
  case GWEN_ERROR_INVALID:
    s="Invalid argument to command.";
    break;
  case GWEN_ERROR_INTERNAL:
    s="Internal error.";
    break;
  case GWEN_ERROR_GENERIC:
    s="Generic error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  GWEN_Buffer_AppendString(buf, "Result of \"");
  GWEN_Buffer_AppendString(buf, lastCommand);
  GWEN_Buffer_AppendString(buf, "\": ");
  GWEN_Buffer_AppendString(buf, s);

  if (res<0 && card) {
    int sw1;
    int sw2;
    char numbuf[32];

    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    GWEN_Buffer_AppendString(buf, " (");
    if (sw1!=-1 && sw2!=-1) {
      GWEN_Buffer_AppendString(buf, " SW1=");
      snprintf(numbuf, sizeof(numbuf), "%02x", sw1);
      GWEN_Buffer_AppendString(buf, numbuf);
      GWEN_Buffer_AppendString(buf, " SW2=");
      snprintf(numbuf, sizeof(numbuf), "%02x", sw2);
      GWEN_Buffer_AppendString(buf, numbuf);
    }
    s=LC_Card_GetLastResult(card);
    if (s) {
      GWEN_Buffer_AppendString(buf, " result=");
      GWEN_Buffer_AppendString(buf, s);
    }
    s=LC_Card_GetLastText(card);
    if (s) {
      GWEN_Buffer_AppendString(buf, " text=");
      GWEN_Buffer_AppendString(buf, s);
    }
    GWEN_Buffer_AppendString(buf, " )");
  }
}



void LC_Card_PrintResult(const LC_CARD *card, const char *lastCommand, int res)
{
  GWEN_BUFFER *buf;

  buf=GWEN_Buffer_new(0, 256, 0, 1);
  LC_Card_CreateResultString(card, lastCommand, res, buf);
  fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(buf));
  GWEN_Buffer_free(buf);
}



int LC_Card_ReadBinary(LC_CARD *card,
                       int offset,
                       int size,
                       GWEN_BUFFER *buf)
{
  int t;
  int bytesRead=0;
  int res;

  while (size>0) {
    int sw1;
    int sw2;

    if (size>252)
      t=252;
    else
      t=size;
    res=LC_Card_IsoReadBinary(card, 0, offset, t, buf);
    if (res<0) {
      if (res==GWEN_ERROR_NO_DATA && bytesRead)
        return 0;
      return res;
    }

    size-=t;
    offset+=t;
    bytesRead+=t;

    /* check for EOF */
    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    if (sw1==0x62 && sw2==0x82) {
      DBG_DEBUG(LC_LOGDOMAIN, "EOF met after %d bytes (asked for %d bytes more)", bytesRead, size);
      break;
    }
  } /* while still data to read */

  return 0;
}




