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


#include "commandmanager_p.h"
#include "server_l.h" /* for some defines */
#include "msgengine.h"
#include "cmd_card_l.h"
#include "cmdrequest_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/text.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



static GWEN_TYPE_UINT32 lccmd__next_request_id=0;



LCCMD_COMMANDMANAGER *LCCMD_CommandManager_new() {
  LCCMD_COMMANDMANAGER *mgr;

  GWEN_NEW_OBJECT(LCCMD_COMMANDMANAGER, mgr);
  DBG_MEM_INC("LCCMD_COMMANDMANAGER", 0);

  return mgr;
}



void LCCMD_CommandManager_free(LCCMD_COMMANDMANAGER *mgr) {
  if (mgr) {
    DBG_MEM_DEC("LCCMD_COMMANDMANAGER");
    LCCMD_CmdRequest_List_free(mgr->requestList);
    GWEN_MsgEngine_free(mgr->msgEngine);
    GWEN_XMLNode_free(mgr->xmlCards);
    GWEN_FREE_OBJECT(mgr);
  }
}



int LCCMD_CommandManager_Init(LCCMD_COMMANDMANAGER *mgr, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *dbT;
  int rv;

  DBG_INFO(0, "Initialising command manager");
  assert(mgr);

  mgr->xmlCards=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "cards");
  mgr->msgEngine=LC_MsgEngine_new();
  mgr->requestList=LCCMD_CmdRequest_List_new();

  rv=LCCMD_CommandManager__LoadAllCards(mgr);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  if (GWEN_Logger_GetLevel(0)>GWEN_LoggerLevel_Debug) {
    GWEN_XMLNode_WriteFile(mgr->xmlCards, "/tmp/cards.xml",
			   GWEN_XML_FLAGS_DEFAULT | GWEN_XML_FLAGS_SIMPLE);
  }

  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "CommandManager");
  if (dbT) {
    /* nothing special for the command manager for now */
  }

  return 0;
}



int LCCMD_CommandManager_Fini(LCCMD_COMMANDMANAGER *mgr, GWEN_DB_NODE *db) {

  LCCMD_CmdRequest_List_free(mgr->requestList);
  mgr->requestList=0;
  GWEN_MsgEngine_free(mgr->msgEngine);
  mgr->msgEngine=0;
  GWEN_XMLNode_free(mgr->xmlCards);
  mgr->xmlCards=0;
  return 0;
}



int LCCMD_CommandManager_MergeXMLDefs(LCCMD_COMMANDMANAGER *mgr,
                                      GWEN_XMLNODE *destNode,
                                      GWEN_XMLNODE *node) {
  GWEN_XMLNODE *nsrc, *ndst;

  assert(mgr);
  assert(node);

  nsrc=GWEN_XMLNode_GetChild(node);
  while(nsrc) {
    /* merge 1st level */
    if (GWEN_XMLNode_GetType(nsrc)==GWEN_XMLNodeTypeTag) {
      ndst=GWEN_XMLNode_FindFirstTag(destNode,
                                     GWEN_XMLNode_GetData(nsrc),
                                     "name",
                                     GWEN_XMLNode_GetProperty(nsrc,
                                                              "name",
                                                              ""));
      if (ndst) {
        GWEN_XMLNODE *nsrc2, *ndst2;

        /* merge 2nd level */
        nsrc2=GWEN_XMLNode_GetChild(nsrc);
        while(nsrc2) {
          if (GWEN_XMLNode_GetType(nsrc2)==GWEN_XMLNodeTypeTag) {
            ndst2=GWEN_XMLNode_FindNode(ndst,
                                        GWEN_XMLNodeTypeTag,
                                        GWEN_XMLNode_GetData(nsrc2));
            if (ndst2) {
              const char *dname;

              dname=GWEN_XMLNode_GetData(nsrc2);

              if (strcasecmp(dname, "cardInfo")==0) {
                GWEN_XMLNODE *nsrc3;

                nsrc3=GWEN_XMLNode_FindFirstTag(nsrc2, "ATRS", 0, 0);
                if (nsrc3) {
                  GWEN_XMLNODE *ndst3;

                  ndst3=GWEN_XMLNode_FindFirstTag(ndst2, "ATRS", 0, 0);
                  if (!ndst3) {
                    ndst3=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "ATRS");
                    GWEN_XMLNode_AddChild(ndst2, ndst3);
                  }
                  GWEN_XMLNode_AddChildrenOnly(ndst3, nsrc3, 1);
                }
              }
              else {
                /* node found, copy branch */
                GWEN_XMLNode_AddChildrenOnly(ndst2, nsrc2, 1);
              }
            }
            else {
              GWEN_XMLNODE *newNode;

              newNode=GWEN_XMLNode_dup(nsrc2);
              GWEN_XMLNode_AddChild(ndst, newNode);
            }
          } /* if TAG */
          nsrc2=GWEN_XMLNode_Next(nsrc2);
        } /* while there are 2nd level source tags */
      }
      else {
	GWEN_XMLNODE *newNode;

        newNode=GWEN_XMLNode_dup(nsrc);
        GWEN_XMLNode_AddChild(destNode, newNode);
      }
    } /* if TAG */
    nsrc=GWEN_XMLNode_Next(nsrc);
  } /* while */

  return 0;
}



int LCCMD_CommandManager__LoadCardFile(LCCMD_COMMANDMANAGER *mgr,
                                       const char *fname){
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *nn;

  assert(mgr);
  assert(fname);

  DBG_DEBUG(0, "Loading card file \"%s\"", fname);

  /* name matches */
  n=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "card");
  if (GWEN_XML_ReadFile(n,
                        fname,
                        GWEN_XML_FLAGS_DEFAULT)) {
    DBG_ERROR(0, "Could not read XML file \"%s\"",
              fname);
    GWEN_XMLNode_free(n);
    return -1;
  }

  nn=GWEN_XMLNode_FindNode(n, GWEN_XMLNodeTypeTag, "cards");
  if (!nn) {
    DBG_DEBUG(0, "File \"%s\" does not contain <cards>", fname);
    GWEN_XMLNode_free(n);
    return -1;
  }
  if (LCCMD_CommandManager_MergeXMLDefs(mgr, mgr->xmlCards, nn)) {
    DBG_ERROR(0, "Could not merge file \"%s\"", fname);
    GWEN_XMLNode_free(n);
    return -1;
  }
  GWEN_MsgEngine_SetDefinitions(mgr->msgEngine,
                                mgr->xmlCards,
                                0); /* dont take over */
  GWEN_XMLNode_free(n);

  return 0;
}





int LCCMD_CommandManager___LoadCardFiles(LCCMD_COMMANDMANAGER *mgr,
                                         const char *where,
                                         GWEN_STRINGLIST *folders) {
  GWEN_BUFFER *buf;
  GWEN_DIRECTORYDATA *d;
  unsigned int dpos;
  int loadedFiles=0;

  buf=GWEN_Buffer_new(0, 256, 0, 1);

  d=GWEN_Directory_new();
  GWEN_Buffer_AppendString(buf, where);
  //GWEN_Buffer_AppendString(buf, DIRSEP "cards");
  DBG_DEBUG(0, "Sampling files in \"%s\"",
            GWEN_Buffer_GetStart(buf));
  dpos=GWEN_Buffer_GetPos(buf);
  if (!GWEN_Directory_Open(d, GWEN_Buffer_GetStart(buf))) {
    char buffer[256];
    unsigned int i;

    while (!GWEN_Directory_Read(d, buffer, sizeof(buffer))){
      if (strcmp(buffer, ".")!=0 &&
          strcmp(buffer, "..")!=0) {
        struct stat st;
  
        GWEN_Buffer_Crop(buf, 0, dpos);
        GWEN_Buffer_SetPos(buf, dpos);
        GWEN_Buffer_AppendByte(buf, '/');
        GWEN_Buffer_AppendString(buf, buffer);
  
        if (stat(GWEN_Buffer_GetStart(buf), &st)) {
          DBG_ERROR(0, "stat(%s): %s",
                    GWEN_Buffer_GetStart(buf),
                    strerror(errno));
        }
        else {
          if (!S_ISDIR(st.st_mode)) {
            i=strlen(buffer);
            if (i>3) {
              if (strcasecmp(buffer+i-4, ".xml")==0) {
                int rv;
  
                rv=
                  LCCMD_CommandManager__LoadCardFile(mgr,
                                                     GWEN_Buffer_GetStart(buf));
                if (rv==0)
                  loadedFiles++;
              } /* if name ends in ".xml" */
            } /* if name longer than 3 chars */
          } /* if it is not a folder */
          else {
            DBG_VERBOUS(0, "Adding folder \"%s\"",
                        GWEN_Buffer_GetStart(buf));
            GWEN_StringList_AppendString(folders,
                                         GWEN_Buffer_GetStart(buf),
                                         0, 1);
          }
        } /* if stat succeeded */
      }
    } /* while */
    GWEN_Directory_Close(d);
  } /* if open succeeded */
  else {
    DBG_INFO(0, "Could not open dir \"%s\"",
             GWEN_Buffer_GetStart(buf));
  }
  GWEN_Directory_free(d);
  GWEN_Buffer_free(buf);

  if (loadedFiles==0) {
    return -1;
  }

  return 0;
}



int LCCMD_CommandManager__LoadCardFiles(LCCMD_COMMANDMANAGER *mgr,
                                        const char *where) {
  GWEN_STRINGLIST *folders;
  GWEN_STRINGLISTENTRY *se;
  int loadedDirs=0;

  folders=GWEN_StringList_new();
  GWEN_StringList_AppendString(folders, where, 0, 1);
  se=GWEN_StringList_FirstEntry(folders);
  while(se) {
    const char *s;
    int rv;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    rv=LCCMD_CommandManager___LoadCardFiles(mgr, s, folders);
    if (rv==0)
      loadedDirs++;
    se=GWEN_StringListEntry_Next(se);
  }
  GWEN_StringList_free(folders);

  if (loadedDirs==0) {
    return -1;
  }

  return 0;
}



int LCCMD_CommandManager__LoadAllCards(LCCMD_COMMANDMANAGER *mgr) {
  GWEN_STRINGLIST *sl;
  GWEN_STRINGLIST *slT;
  GWEN_STRINGLISTENTRY *se;
  GWEN_BUFFER *tbuf;
  int loadedFiles=0;

  sl=GWEN_StringList_new();

  /* add "cards" subfolder of all data dirs */
  slT=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                LCS_PATH_SERVER_DATADIR);
  assert(slT);
  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  se=GWEN_StringList_FirstEntry(slT);
  while(se) {
    const char *s;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    GWEN_Buffer_AppendString(tbuf, s);
    GWEN_Buffer_AppendString(tbuf, DIRSEP);
    GWEN_Buffer_AppendString(tbuf, "cards");
    GWEN_StringList_AppendString(sl, GWEN_Buffer_GetStart(tbuf), 0, 1);
    GWEN_Buffer_Reset(tbuf);
    se=GWEN_StringListEntry_Next(se);
  }
  GWEN_StringList_free(slT);
  GWEN_Buffer_free(tbuf);

  /* add all driver folders */
  slT=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                                LCS_PATH_DRIVER_INFODIR);
  assert(slT);
  se=GWEN_StringList_FirstEntry(slT);
  while(se) {
    const char *s;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    GWEN_StringList_AppendString(sl, s, 0, 1);
    se=GWEN_StringListEntry_Next(se);
  }
  GWEN_StringList_free(slT);

  /* actually load the files */
  se=GWEN_StringList_FirstEntry(sl);
  while(se) {
    const char *s;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    if (LCCMD_CommandManager__LoadCardFiles(mgr, s)==0)
      loadedFiles++;
    se=GWEN_StringListEntry_Next(se);
  }

  GWEN_StringList_free(sl);

  if (loadedFiles==0) {
    DBG_ERROR(0, "No card files loaded");
    return -1;
  }

  return 0;
}



int LCCMD_CommandManager__AddCardTypesByAtr(LCCMD_COMMANDMANAGER *mgr,
                                            LCCO_CARD *card){
  GWEN_XMLNODE *cardNode;
  const char *atr;
  unsigned int atrLen;
  GWEN_BUFFER *hexAtr;
  int types=0;
  int done;

  assert(mgr);
  DBG_DEBUG(0, "Adding card types...");

  /* get ATR, convert it to hex */
  atr=LCCO_Card_GetAtr(card, &atrLen);
  if (atr==0 || atrLen==0) {
    DBG_INFO(0, "No ATR");
    return 1;
  }
  hexAtr=GWEN_Buffer_new(0, 256, 0, 1);
  if (GWEN_Text_ToHexBuffer(atr, atrLen, hexAtr, 0, 0, 0)) {
    DBG_ERROR(0, "Internal error");
    abort();
  }

  cardNode=GWEN_XMLNode_FindFirstTag(mgr->xmlCards, "card", 0, 0);
  if (!cardNode) {
    DBG_ERROR(0, "No card nodes.");
    return -1;
  }
  while(cardNode) {
    const char *name;
    const char *tp;
    int sameBaseType=0;

    name=GWEN_XMLNode_GetProperty(cardNode, "name", 0);
    assert(name);
    tp=GWEN_XMLNode_GetProperty(cardNode, "type", 0);

    DBG_VERBOUS(0, "Checking card \"%s\"", name);
    if (tp) {
      if (LCCO_Card_GetCardType(card)==LC_CardTypeProcessor)
        sameBaseType=(strcasecmp(tp, "processor")==0);
      else if (LCCO_Card_GetCardType(card)==LC_CardTypeMemory)
        sameBaseType=(strcasecmp(tp, "memory")==0);
    }
    if (sameBaseType) {
      GWEN_XMLNODE *nAtrs;

      nAtrs=GWEN_XMLNode_FindFirstTag(cardNode, "cardinfo", 0, 0);
      if (nAtrs)
        nAtrs=GWEN_XMLNode_FindFirstTag(nAtrs, "atrs", 0, 0);
      if (nAtrs) {
        GWEN_XMLNODE *nAtr;
  
        nAtr=GWEN_XMLNode_GetFirstTag(nAtrs);
        while(nAtr) {
          GWEN_XMLNODE *nData;
  
          nData=GWEN_XMLNode_GetFirstData(nAtr);
          if (nData) {
            const char *p;
  
            p=GWEN_XMLNode_GetData(nData);
            if (p) {
              GWEN_BUFFER *dbuf;
  
              dbuf=GWEN_Buffer_new(0, 256, 0, 1);
              while(*p) {
                if (!isspace(*p))
                  GWEN_Buffer_AppendByte(dbuf, *p);
                p++;
              } /* while */
              if (-1!=GWEN_Text_ComparePattern(GWEN_Buffer_GetStart(hexAtr),
                                               GWEN_Buffer_GetStart(dbuf),
                                               0)) {
                DBG_DEBUG(0, "Card \"%s\" matches ATR", name);
                if (LCCO_Card_AddType(card, name)) {
                  DBG_INFO(0, "Added card type \"%s\"", name);
                  types++;
                }
              }
              GWEN_Buffer_free(dbuf);
            } /* if data */
          } /* if data node */
          nAtr=GWEN_XMLNode_GetNextTag(nAtr);
        } /* while */
      } /* if atrs */
    } /* if sameBaseType */
    cardNode=GWEN_XMLNode_FindNextTag(cardNode, "card", 0, 0);
  } /* while */
  GWEN_Buffer_free(hexAtr);

  /* add all cards whose base types are contained in the list.
   * repeat this as long as we added cards */
  done=0;
  while(!done) {
    done=1;
    cardNode=GWEN_XMLNode_FindFirstTag(mgr->xmlCards, "card", 0, 0);
    while(cardNode) {
      const char *name;
      const char *extends;

      name=GWEN_XMLNode_GetProperty(cardNode, "name", 0);
      assert(name);
      extends=GWEN_XMLNode_GetProperty(cardNode, "extends", 0);
      if (extends) {
        if (GWEN_StringList_HasString(LCCO_Card_GetTypes(card), extends)) {
          if (LCCO_Card_AddType(card, name)) {
            DBG_DEBUG(0, "Added card type \"%s\"", name);
            types++;
            done=0;
          }
        }
      }
      cardNode=GWEN_XMLNode_FindNextTag(cardNode, "card", 0, 0);
    }
  } /* while */

  return (types!=0)?0:1;
}



GWEN_XMLNODE *LCCMD_CommandManager___FindCommand(GWEN_XMLNODE *node,
                                                 const char *commandName,
                                                 const char *driverType,
                                                 const char *readerType){
  GWEN_XMLNODE *cmds;
  GWEN_XMLNODE *n;

  DBG_DEBUG(0, "Searching in \"%s\"",
            GWEN_XMLNode_GetProperty(node, "name", "(noname)"));

  cmds=GWEN_XMLNode_FindNode(node,
                             GWEN_XMLNodeTypeTag,
                             "commands");
  if (!cmds) {
    DBG_INFO(0, "No commands in card data");
    return 0;
  }

  /* first try exact match */
  if (driverType && readerType) {
    DBG_DEBUG(0, "Searching for %s/%s/%s",
              driverType, readerType, commandName);
    n=GWEN_XMLNode_FindFirstTag(cmds,
                                "command",
                                "name",
                                commandName);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0 &&
          strcasecmp(GWEN_XMLNode_GetProperty(n, "reader", ""),
                     readerType)==0) {
        DBG_DEBUG(0, "Found command in %s/%s", driverType, readerType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
    } /* while */
  }

  if (driverType) {
    /* try match of driver only */
    DBG_DEBUG(0, "Searching for %s/%s",
              driverType, commandName);
    n=GWEN_XMLNode_FindFirstTag(cmds,
                                "command",
                                "name",
                                commandName);
    while(n) {
      if (strcasecmp(GWEN_XMLNode_GetProperty(n, "driver", ""),
                     driverType)==0) {
        DBG_DEBUG(0, "Found command in %s", driverType);
        return n;
      }
      n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
    } /* while */
  }

  /* try match of command name only */
  DBG_DEBUG(0, "Searching for %s", commandName);
  n=GWEN_XMLNode_FindFirstTag(cmds,
                              "command",
                              "name",
                              commandName);
  while(n) {
    if (!GWEN_XMLNode_GetProperty(n, "driver", 0))
      return n;
    n=GWEN_XMLNode_FindNextTag(n, "command", "name", commandName);
  } /* while */

  return n;
}



GWEN_XMLNODE *LCCMD_CardCommander__FindCommand(LCCMD_COMMANDMANAGER *mgr,
                                               GWEN_XMLNODE *node,
                                               const char *commandName,
                                               const char *driverType,
                                               const char *readerType){

  assert(node);

  while(node) {
    GWEN_XMLNODE *n;
    const char *parent;

    n=LCCMD_CommandManager___FindCommand(node, commandName,
                                         driverType, readerType);
    if (n) {
      return n;
    }
    parent=GWEN_XMLNode_GetProperty(node, "extends", 0);
    if (!parent)
      break;
    node=GWEN_XMLNode_FindFirstTag(mgr->xmlCards,
                                   "card",
                                   "name",
                                   parent);
    if (!node) {
      DBG_WARN(0, "Extended card \"%s\" not found", parent);
      break;
    }
    DBG_DEBUG(0, "Searching in parent \"%s\"", parent);
  } /* while */

  DBG_DEBUG(0, "Command \"%s\" not found", commandName);
  return 0;
}



GWEN_XMLNODE *LCCMD_CommandManager___FindResult(GWEN_XMLNODE *cmd,
                                                int sw1, int sw2) {
  GWEN_XMLNODE *rnode;
  GWEN_XMLNODE *n;
  int lsw1, lsw2;

  DBG_DEBUG(0, "Searching for result type of %02x/%02x", sw1, sw2);
  while(cmd) {
    rnode=GWEN_XMLNode_FindNode(cmd,
                                GWEN_XMLNodeTypeTag,
                                "results");
    if (rnode) {
      /* first try exact match */
      n=GWEN_XMLNode_GetFirstTag(rnode);
      while(n) {
        if (1==sscanf(GWEN_XMLNode_GetProperty(n, "sw1", "-1"),
                      "%i", &lsw1) &&
            1==sscanf(GWEN_XMLNode_GetProperty(n, "sw2", "-1"),
                      "%i", &lsw2)) {
          DBG_VERBOUS(0, "Checking %02x/%02x", lsw1, lsw2);
          if (lsw1==sw1 && lsw2==sw2) {
            return n;
          }
        }
        else {
          DBG_WARN(0, "Bad SW1 or SW2 value");
        }
        n=GWEN_XMLNode_GetNextTag(n);
      } /* while */

      /* try SW1 only */
      n=GWEN_XMLNode_GetFirstTag(rnode);
      while(n) {
        if (1==sscanf(GWEN_XMLNode_GetProperty(n, "sw1", "-1"),
                      "%i", &lsw1) &&
            1==sscanf(GWEN_XMLNode_GetProperty(n, "sw2", "-1"),
                      "%i", &lsw2)) {
          if (lsw1==sw1 && lsw2==-1) {
            return n;
          }
        }
        else {
          DBG_WARN(0, "Bad SW1 or SW2 value");
        }
        n=GWEN_XMLNode_GetNextTag(n);
      } /* while */
    } /* if rnode */

    /* select parent */
    cmd=GWEN_XMLNode_GetParent(cmd);
  }

  return 0;
}



GWEN_XMLNODE *LCCMD_CommandManager__FindResult(LCCMD_COMMANDMANAGER *mgr,
                                               LCCO_CARD *card,
                                               GWEN_XMLNODE *cmd,
                                               int sw1,
                                               int sw2) {
  GWEN_XMLNODE *tmpNode;
  GWEN_XMLNODE *rnode;

  tmpNode=cmd;
  rnode=0;

  /* find in command */
  rnode=LCCMD_CommandManager___FindResult(cmd, sw1, sw2);
  if (rnode)
    return rnode;
  rnode=LCCMD_CommandManager___FindResult(cmd, -1, -1);
  if (rnode)
    return rnode;

  tmpNode=LCCMD_Card_GetCardNode(card);
  for(;;) {
    const char *parent;

    parent=GWEN_XMLNode_GetProperty(tmpNode, "extends", 0);
    if (!parent) {
      break;
    }
    tmpNode=GWEN_XMLNode_FindFirstTag(mgr->xmlCards,
                                      "card",
                                      "name",
                                      parent);
    if (!tmpNode) {
      break;
    }

    rnode=LCCMD_CommandManager___FindResult(tmpNode, sw1, sw2);
    if (rnode) {
      break;
    }
    rnode=LCCMD_CommandManager___FindResult(tmpNode, -1, -1);
    if (rnode) {
      break;
    }
  } /* for */

  return rnode;
}



GWEN_XMLNODE *LCCMD_CommandManager__FindResponse(LCCMD_COMMANDMANAGER *mgr,
                                                 GWEN_XMLNODE *cmd,
                                                 const char *typ) {
  GWEN_XMLNODE *rnode;
  GWEN_XMLNODE *n;
  const char *ltyp;

  DBG_DEBUG(0, "Searching for response type \"%s\"", typ);
  rnode=GWEN_XMLNode_FindNode(cmd,
                              GWEN_XMLNodeTypeTag,
                              "responses");
  if (!rnode) {
    DBG_DEBUG(0, "No <responses> tag in command definition");
    return 0;
  }

  /* first try exact match */
  n=GWEN_XMLNode_GetFirstTag(rnode);
  while(n) {
    ltyp=GWEN_XMLNode_GetProperty(n, "type", 0);
    if (ltyp) {
      if (strcasecmp(ltyp, typ)==0)
        return n;
    }
    n=GWEN_XMLNode_GetNextTag(n);
  } /* while */

  /* then try a response without any type */
  n=GWEN_XMLNode_GetFirstTag(rnode);
  while(n) {
    ltyp=GWEN_XMLNode_GetProperty(n, "type", 0);
    if (!ltyp)
      return n;
    n=GWEN_XMLNode_GetNextTag(n);
  } /* while */

  return 0;
}







int LCCMD_CommandManager__BuildApdu(LCCMD_COMMANDMANAGER *mgr,
                                    GWEN_XMLNODE *node,
                                    GWEN_DB_NODE *cmdData,
                                    GWEN_BUFFER *gbuf) {
  GWEN_XMLNODE *sendNode;
  GWEN_XMLNODE *dataNode;
  GWEN_XMLNODE *apduNode;
  GWEN_BUFFER *dataBuffer;
  unsigned int i;
  int j;

  assert(mgr);

  sendNode=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "send");
  if (!sendNode) {
    DBG_ERROR(0, "No <send> tag in command definition");
    abort();
  }

  apduNode=GWEN_XMLNode_FindNode(sendNode,
                                 GWEN_XMLNodeTypeTag, "apdu");
  if (!apduNode) {
    DBG_ERROR(0, "No <apdu> tag in command definition");
    abort();
  }

  dataBuffer=GWEN_Buffer_new(0, 256, 0, 1);
  dataNode=GWEN_XMLNode_FindNode(sendNode,
                                 GWEN_XMLNodeTypeTag, "data");
  if (dataNode) {
    /* there is a data node, sample data */
    if (GWEN_MsgEngine_CreateMessageFromNode(mgr->msgEngine,
                                             dataNode,
                                             dataBuffer,
                                             cmdData)) {
      DBG_ERROR(0, "Error creating data for APDU");
      GWEN_Buffer_free(dataBuffer);
      GWEN_Buffer_AppendString(gbuf, "Error creating APDU data from command");
      return -1;
    }
  }

  if (GWEN_MsgEngine_CreateMessageFromNode(mgr->msgEngine,
                                           apduNode,
                                           gbuf,
                                           cmdData)) {
    DBG_ERROR(0, "Error creating APDU");
    GWEN_Buffer_free(dataBuffer);
    GWEN_Buffer_AppendString(gbuf, "Error creating APDU from command");
    return -1;
  }

  i=GWEN_Buffer_GetUsedBytes(dataBuffer);
  if (i) {
    GWEN_Buffer_AppendByte(gbuf, (unsigned char)i);
    GWEN_Buffer_AppendBuffer(gbuf, dataBuffer);
  }
  GWEN_Buffer_free(dataBuffer);

  j=0;
  if (1!=sscanf(GWEN_XMLNode_GetProperty(apduNode, "lr", "0"),
                "%i", &j))
    j=0;

  if (j!=-1) {
    j=GWEN_DB_GetIntValue(cmdData, "lr", 0, -1);
    if (j==-1) {
      if (1!=sscanf(GWEN_XMLNode_GetProperty(apduNode, "lr", "-1"),
                    "%i", &j))
        j=-1;
    }
  }
  if (j>=0)
    GWEN_Buffer_AppendByte(gbuf, (unsigned char)j);

  DBG_INFO(0, "APDU is:");
  GWEN_Text_LogString(GWEN_Buffer_GetStart(gbuf),
                      GWEN_Buffer_GetUsedBytes(gbuf),
                      0,
                      GWEN_LoggerLevelInfo);
  return 0;
}



int LCCMD_CommandManager__ParseResult(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      GWEN_XMLNODE *node,
                                      GWEN_BUFFER *gbuf,
                                      GWEN_DB_NODE *rspData){
  unsigned int i;
  int sw1, sw2;
  GWEN_DB_NODE *dbTmp;
  GWEN_XMLNODE *rnode;

  GWEN_Buffer_Rewind(gbuf); /* just in case ... */
  i=GWEN_Buffer_GetUsedBytes(gbuf);
  if (i<2) {
    DBG_ERROR(0, "Answer too small (less than 2 bytes)");
    return -1;
  }
  DBG_INFO(0, "Driver response:");
  GWEN_Text_LogString(GWEN_Buffer_GetStart(gbuf), i,
                      0, GWEN_LoggerLevelInfo);
  sw1=(unsigned char)(GWEN_Buffer_GetStart(gbuf)[i-2]);
  sw2=(unsigned char)(GWEN_Buffer_GetStart(gbuf)[i-1]);
  GWEN_Buffer_Crop(gbuf, 0, i-2);
  /* store result */
  dbTmp=GWEN_DB_GetGroup(rspData,
                         GWEN_DB_FLAGS_DEFAULT |
                         GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                         "result");
  assert(dbTmp);
  GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                      "sw1", sw1);
  GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                      "sw2", sw2);

  rnode=LCCMD_CommandManager__FindResult(mgr, card, node, sw1, sw2);
  if (rnode) {
    const char *t;
    GWEN_XMLNODE *tnode;
    GWEN_BUFFER *txtbuf;
    int first;

    t=GWEN_XMLNode_GetProperty(rnode, "type", "success");
    DBG_INFO(0, "Result is: %s", t);
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "type", t);
    /* get text */
    txtbuf=GWEN_Buffer_new(0, 256, 0, 1);
    first=1;
    tnode=GWEN_XMLNode_GetFirstData(rnode);
    while(tnode) {
      const char *p;

      p=GWEN_XMLNode_GetData(tnode);
      if (p) {
        if (!first)
          GWEN_Buffer_AppendByte(txtbuf, ' ');
        GWEN_Buffer_AppendString(txtbuf, p);
      }
      if (first)
        first=0;
      tnode=GWEN_XMLNode_GetNextData(tnode);
    } /* while */

    if (GWEN_Buffer_GetUsedBytes(txtbuf))
      GWEN_DB_SetCharValue(dbTmp, GWEN_DB_FLAGS_DEFAULT, "text",
                           GWEN_Buffer_GetStart(txtbuf));
    GWEN_Buffer_free(txtbuf);
  }
  else {
    DBG_ERROR(0,
              "Result for %02x/%02x not found, assuming error",
              sw1, sw2);
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "type", "error");
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "text", "Result not found");
  }

  return 0;
}



int LCCMD_CommandManager__ParseResponse(LCCMD_COMMANDMANAGER *mgr,
                                        LCCO_CARD *card,
                                        GWEN_XMLNODE *node,
                                        GWEN_BUFFER *gbuf,
                                        GWEN_DB_NODE *rspData){
  GWEN_DB_NODE *dbTmp;
  GWEN_XMLNODE *rnode;
  const char *p;

  assert(mgr);

  GWEN_Buffer_Rewind(gbuf); /* just in case ... */

  p=GWEN_DB_GetCharValue(rspData, "result/type", 0, 0);
  if (!p) {
    DBG_ERROR(0, "No result type given");
    return -1;
  }
  dbTmp=GWEN_DB_GetGroup(rspData,
                         GWEN_DB_FLAGS_DEFAULT |
                         GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                         "response");
  if (!dbTmp) {
    DBG_ERROR(0, "No matching response tag found");
    return -1;
  }

  rnode=LCCMD_CommandManager__FindResponse(mgr, node, p);
  if (!rnode) {
    DBG_DEBUG(0, "Did not find response");
    if (GWEN_Buffer_GetUsedBytes(gbuf)) {
      GWEN_DB_SetBinValue(dbTmp,
                          GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "data",
                          GWEN_Buffer_GetStart(gbuf),
                          GWEN_Buffer_GetUsedBytes(gbuf));
    }
  }
  else {
    if (GWEN_MsgEngine_ParseMessage(mgr->msgEngine,
                                    rnode,
                                    gbuf,
                                    dbTmp,
                                    GWEN_MSGENGINE_READ_FLAGS_DEFAULT)){
      DBG_ERROR(0, "Error parsing response");
      return -1;
    }
  }

  return 0;
}



int LCCMD_CommandManager__ParseAnswer(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      LCCMD_CMDREQUEST *req,
                                      GWEN_BUFFER *gbuf,
                                      GWEN_DB_NODE *rspData){
  assert(mgr);
  assert(req);

  if (LCCMD_CommandManager__ParseResult(mgr,
                                        card,
                                        LCCMD_CmdRequest_GetCmdNode(req),
                                        gbuf,
                                        rspData)) {
    DBG_INFO(0, "Error parsing result");
    return -1;
  }

  if (LCCMD_CommandManager__ParseResponse(mgr,
                                          card,
                                          LCCMD_CmdRequest_GetCmdNode(req),
                                          gbuf, rspData)){
    DBG_INFO(0, "Error parsing response");
    return -1;
  }

  return 0;
}







int LCCMD_CommandManager_BuildCommand(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      const char *cmdName,
                                      GWEN_DB_NODE *dbCmd,
                                      GWEN_BUFFER *apdu,
                                      const char **target,
                                      GWEN_TYPE_UINT32 *rqid){
  GWEN_XMLNODE *n;
  int rv;

  assert(mgr);
  assert(card);
  assert(cmdName);
  assert(dbCmd);
  assert(apdu);
  assert(target);
  assert(rqid);

  n=LCCMD_Card_GetCardNode(card);
  if (!n) {
    DBG_ERROR(0, "No card type selected");
    GWEN_Buffer_AppendString(apdu, "No card type selected");
    return -LC_ERROR_INVALID;
  }

  n=LCCMD_CardCommander__FindCommand(mgr, n,
                                     cmdName,
                                     LCCO_Card_GetDriverTypeName(card),
                                     LCCO_Card_GetReaderTypeName(card));
  if (!n) {
    DBG_ERROR(0, "Command \"%s\" not found", cmdName);
    GWEN_Buffer_AppendString(apdu, "Command not found");
    return -LC_ERROR_NO_DATA;
  }

  if (atoi(GWEN_XMLNode_GetProperty(n, "noop", "0"))) {
    DBG_INFO(0, "Command \"%s\" is a no-operation command", cmdName);
    rv=0;
  }
  else {
    /* really build APDU */
    rv=LCCMD_CommandManager__BuildApdu(mgr, n, dbCmd, apdu);
  }

  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }
  else {
    LCCMD_CMDREQUEST *req;
    GWEN_TIME *ti;

    req=LCCMD_CmdRequest_new();
    ti=GWEN_CurrentTime();
    assert(ti);
    LCCMD_CmdRequest_SetRequestId(req, ++lccmd__next_request_id);
    LCCMD_CmdRequest_SetRequestTime(req, ti);
    LCCMD_CmdRequest_SetCmdNode(req, n);
    GWEN_Time_free(ti);
    LCCMD_CmdRequest_List_Add(req, mgr->requestList);
    *rqid=LCCMD_CmdRequest_GetRequestId(req);
    *target=GWEN_XMLNode_GetProperty(n, "target", "card");
  }

  return 0;
}



int LCCMD_CommandManager_ParseAnswer(LCCMD_COMMANDMANAGER *mgr,
                                     LCCO_CARD *card,
                                     GWEN_TYPE_UINT32 rqid,
                                     GWEN_BUFFER *gbuf,
                                     GWEN_DB_NODE *dbRsp) {
  LCCMD_CMDREQUEST *req;
  int rv;

  /* find request */
  req=LCCMD_CmdRequest_List_First(mgr->requestList);
  while(req) {
    if (rqid==LCCMD_CmdRequest_GetRequestId(req))
      break;
    req=LCCMD_CmdRequest_List_Next(req);
  }
  if (!req) {
    DBG_ERROR(0, "Request \"%08x\" not found", rqid);
    return -LC_ERROR_INVALID;
  }

  if (atoi(GWEN_XMLNode_GetProperty(LCCMD_CmdRequest_GetCmdNode(req),
                                    "noop", "0"))) {
    GWEN_DB_NODE *dbTmp;

    /* noop, fake result */
    dbTmp=GWEN_DB_GetGroup(dbRsp,
                           GWEN_DB_FLAGS_DEFAULT |
                           GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                           "result");
    assert(dbTmp);
    GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                        "sw1", 0x90);
    GWEN_DB_SetIntValue(dbTmp, GWEN_DB_FLAGS_DEFAULT,
                        "sw2", 0x00);
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "type", "success");
    GWEN_DB_SetCharValue(dbTmp,
                         GWEN_DB_FLAGS_DEFAULT,
                         "text", "Noop, returning success indication");
    rv=0;
  }
  else {
    /* parse the response with help of the data in this request */
    rv=LCCMD_CommandManager__ParseAnswer(mgr, card, req, gbuf, dbRsp);
  }

  /* dequeue and free request */
  LCCMD_CmdRequest_List_Del(req);
  LCCMD_CmdRequest_free(req);

  /* check result */
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  /* done */
  return 0;
}









void LCCMD_CommandManager_NewCard(LCCMD_COMMANDMANAGER *mgr,
                                  LCCO_CARD *card){
  assert(mgr);
  assert(card);

  LCCMD_Card_extend(card);
  LCCMD_CommandManager__AddCardTypesByAtr(mgr, card);
}



int LCCMD_CommandManager_SelectCardType(LCCMD_COMMANDMANAGER *mgr,
                                        LCCO_CARD *card,
                                        const char *cardName) {
  GWEN_XMLNODE *cardNode;

  assert(mgr);
  assert(card);
  assert(cardName);

  cardNode=GWEN_XMLNode_FindFirstTag(mgr->xmlCards,
                                     "card",
                                     "name",
                                     cardName);
  assert(cardNode);
  DBG_NOTICE(0, "Card type \"%s\" selected", cardName);

  LCCMD_Card_SetCardNode(card, cardNode);

  return 0;
}







