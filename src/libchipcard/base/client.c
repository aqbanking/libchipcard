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

#include "client_p.h"
#include "client_xml.h"
#include "libchipcard/base/msgengine.h"
#include "libchipcard/base/driverinfo.h"

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/i18n.h>

#include <winscard.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif


#define I18N(msg) GWEN_I18N_Translate(PACKAGE, msg)


#ifndef MAX_ATR_SIZE
# define MAX_ATR_SIZE 33
#endif


#ifndef OS_WIN32
# ifndef SCARD_E_NO_READERS_AVAILABLE
#  define SCARD_E_NO_READERS_AVAILABLE 0x8010002e
# endif
#endif



static int lc_client__initcounter=0;
static GWEN_XMLNODE *lc_client__card_nodes=NULL;
static GWEN_XMLNODE *lc_client__app_nodes=NULL;
static GWEN_DB_NODE *lc_client__driver_db=NULL;


GWEN_INHERIT_FUNCTIONS(LC_CLIENT)



/* ------------------------------------------------------------------------------------------------
 * forward declarations
 * ------------------------------------------------------------------------------------------------
 */

static int _initCommon();
static void _finiCommon();



/* ------------------------------------------------------------------------------------------------
 * code
 * ------------------------------------------------------------------------------------------------
 */


int _initCommon()
{
  if (lc_client__initcounter==0) {
    int rv;
    GWEN_STRINGLIST *paths;

    rv=GWEN_Init();
    if (rv) {
      DBG_ERROR_ERR(LC_LOGDOMAIN, rv);
      return rv;
    }

    if (!GWEN_Logger_IsOpen(LC_LOGDOMAIN)) {
      const char *s;

      /* only set our logger if it not already has been */
      GWEN_Logger_Open(LC_LOGDOMAIN, "libchipcard", 0,
                       GWEN_LoggerType_Console,
                       GWEN_LoggerFacility_User);
      GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevel_Warning);

      s=getenv("LC_LOGLEVEL");
      if (s) {
        GWEN_LOGGER_LEVEL ll;

        ll=GWEN_Logger_Name2Level(s);
        if (ll!=GWEN_LoggerLevel_Unknown) {
          GWEN_Logger_SetLevel(LC_LOGDOMAIN, ll);
          DBG_WARN(LC_LOGDOMAIN, "Overriding loglevel for Libchipcard-Client with \"%s\"", s);
        }
        else {
          DBG_ERROR(0, "Unknown loglevel \"%s\"", s);
        }
      }
      else {
        GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevel_Warning);
      }
    }

    /* setup locale */
    rv=GWEN_I18N_BindTextDomain_Dir(PACKAGE, LC_CLIENT_LOCALE_DIR);
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN, "Could not bind textdomain (%d)", rv);
    }
    else {
      rv=GWEN_I18N_BindTextDomain_Codeset(PACKAGE, "UTF-8");
      if (rv) {
	DBG_ERROR(LC_LOGDOMAIN, "Could not set codeset (%d)", rv);
      }
    }


    /* define data path */
    GWEN_PathManager_DefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
#if defined(OS_WIN32) || defined(ENABLE_LOCAL_INSTALL)
    /* add folder relative to EXE */
    GWEN_PathManager_AddRelPath(LCC_PM_LIBNAME,
                                LCC_PM_LIBNAME,
                                LCC_PM_DATADIR,
                                LC_CLIENT_XML_DIR,
                                GWEN_PathManager_RelModeExe);
#else
    /* add absolute folder */
    GWEN_PathManager_AddPath(LCC_PM_LIBNAME,
                             LCC_PM_LIBNAME,
                             LCC_PM_DATADIR,
                             LC_CLIENT_XML_DIR);
#endif

    /* load XML files */
    paths=GWEN_PathManager_GetPaths(LCC_PM_LIBNAME, LCC_PM_DATADIR);
    if (paths) {
      GWEN_XMLNODE *n;
      GWEN_DB_NODE *db;
      GWEN_BUFFER *fbuf;
      uint32_t bpos;

      fbuf=GWEN_Buffer_new(0, 256, 0, 1);
      rv=GWEN_Directory_FindPathForFile(paths, "cards/card.xml", fbuf);
      GWEN_StringList_free(paths);
      if (rv) {
        DBG_ERROR(LC_LOGDOMAIN, "Data files not found (%d)", rv);
        /* undo all init stuff so far */
        GWEN_Buffer_free(fbuf);
        GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
        return rv;
      }

      /* load card files */
      n=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "cards");
      if (LC_Client_ReadXmlFiles(n, GWEN_Buffer_GetStart(fbuf), "cards", "card")) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not read card files");
        GWEN_XMLNode_free(n);
        /* undo all init stuff so far */
        GWEN_Buffer_free(fbuf);
        GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
        return GWEN_ERROR_GENERIC;
      }
      lc_client__card_nodes=n;

      /* load app files */
      n=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "apps");
      if (LC_Client_ReadXmlFiles(n, GWEN_Buffer_GetStart(fbuf), "apps", "app")) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not read app files");
        GWEN_XMLNode_free(n);
        /* undo all init stuff so far */
        GWEN_XMLNode_free(lc_client__card_nodes);
        lc_client__card_nodes=NULL;
        GWEN_Buffer_free(fbuf);
        GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
        return GWEN_ERROR_GENERIC;
      }
      lc_client__app_nodes=n;
      /*GWEN_XMLNode_WriteFile(n, "/tmp/apps", GWEN_XML_FLAGS_DEFAULT);*/

      /* load driver files (if any) */
      bpos=GWEN_Buffer_GetPos(fbuf);
      GWEN_Buffer_AppendString(fbuf, DIRSEP "drivers");
      db=GWEN_DB_Group_new("drivers");
      rv=LC_DriverInfo_ReadDrivers(GWEN_Buffer_GetStart(fbuf), db, 0, 1);
      if (rv) {
        DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
        GWEN_DB_Group_free(db);
        /* undo all init stuff so far */
        GWEN_XMLNode_free(lc_client__app_nodes);
        lc_client__app_nodes=NULL;
        GWEN_XMLNode_free(lc_client__card_nodes);
        lc_client__card_nodes=NULL;
        GWEN_Buffer_free(fbuf);
        GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
        return rv;
      }
      lc_client__driver_db=db;
      GWEN_Buffer_Crop(fbuf, 0, bpos);

      /* insert more loading here */
      GWEN_Buffer_free(fbuf);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No data files found.");
      /* undo all init stuff so far */
      GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);
      return GWEN_ERROR_GENERIC;
    }
  }

  lc_client__initcounter++;
  return 0;
}



void _finiCommon()
{
  if (lc_client__initcounter==1) {
    GWEN_DB_Group_free(lc_client__driver_db);
    lc_client__driver_db=NULL;
    GWEN_XMLNode_free(lc_client__app_nodes);
    lc_client__app_nodes=0;
    GWEN_XMLNode_free(lc_client__card_nodes);
    lc_client__card_nodes=0;

    GWEN_PathManager_UndefinePath(LCC_PM_LIBNAME, LCC_PM_DATADIR);

    GWEN_Logger_Close(LC_LOGDOMAIN);
    GWEN_Fini();
  }
  if (lc_client__initcounter>0)
    lc_client__initcounter--;
}




LC_CLIENT *LC_Client_new(const char *programName, const char *programVersion)
{
  LC_CLIENT *cl;

  assert(programName);
  assert(programVersion);

  if (_initCommon()) {
    DBG_ERROR(0, "Unable to initialize, aborting");
    return NULL;
  }

  GWEN_NEW_OBJECT(LC_CLIENT, cl);
  GWEN_INHERIT_INIT(LC_CLIENT, cl);
  cl->programName=strdup(programName);
  cl->programVersion=strdup(programVersion);

  cl->cardNodes=lc_client__card_nodes;
  cl->appNodes=lc_client__app_nodes;
  cl->msgEngine=LC_MsgEngine_new();

  return cl;
}



void LC_Client_free(LC_CLIENT *cl)
{
  if (cl) {
    GWEN_INHERIT_FINI(LC_CLIENT, cl);
    free(cl->programVersion);
    free(cl->programName);
    GWEN_MsgEngine_free(cl->msgEngine);

    GWEN_FREE_OBJECT(cl);

    _finiCommon();
  }
}





int LC_Client_Init(LC_CLIENT *cl)
{
  LONG rv;

  assert(cl);

  /* establish context */
  rv=SCardEstablishContext(SCARD_SCOPE_SYSTEM,    /* scope */
                           NULL,                  /* reserved1 */
                           NULL,                  /* reserved2 */
                           &(cl->scardContext));  /* ptr to context */
  if (rv!=SCARD_S_SUCCESS) {
    if (rv == SCARD_E_NO_SERVICE) {
      DBG_ERROR(LC_LOGDOMAIN,
                "SCardEstablishContext: "
                "Error SCARD_E_NO_SERVICE: "
                "The Smartcard resource manager is not running. "
                "Maybe you have to start the Smartcard service manually?");
      GWEN_Gui_ProgressLog(0, GWEN_LoggerLevel_Error,
                           I18N("The PC/SC service is not running.\n"
                                "Please make sure that the package \"pcscd\" is\n"
                                "installed along with the appropriate driver.\n"
                                "For cyberJack devices you will need to install\n"
                                "the package \"ifd-cyberjack\" (Debian) or\n"
                                "\"cyberjack-ifd\" (SuSE).\n"
                                "For most other readers the package \"libccid\"\n"
                                "needs to be installed."
                                "<html>"
                                "<p>The PC/SC service is not running.</p>"
                                "<p>Please make sure that the package <b>pcscd</b> is "
                                "installed along with the appropriate driver.</p>"
                                "<p>For cyberJack devices you will need to install "
                                "the package <b>ifd-cyberjack</b> (Debian) or "
                                "<b>cyberjack-ifd</b> (SuSE).</p>"
                                "<p>For most other readers the package <b>libccid</b> "
                                "needs to be installed.</p>"
                                "</html>"));
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "SCardEstablishContext: %ld (%04lx)", (long int) rv, rv);
    }
    _finiCommon();
    return GWEN_ERROR_IO;
  }

  return 0;
}



int LC_Client_Fini(LC_CLIENT *cl)
{
  LONG rv;

  rv=SCardReleaseContext(cl->scardContext);
  if (rv!=SCARD_S_SUCCESS) {
    DBG_ERROR(LC_LOGDOMAIN, "SCardReleaseContext: %04lx", (long unsigned int) rv);
    _finiCommon();
    return GWEN_ERROR_IO;
  }

  return 0;
}



const char *LC_Client_GetProgramName(const LC_CLIENT *cl)
{
  assert(cl);
  return cl->programName;
}



const char *LC_Client_GetProgramVersion(const LC_CLIENT *cl)
{
  assert(cl);
  return cl->programVersion;
}



GWEN_XMLNODE *LC_Client_GetAppNodes(const LC_CLIENT *cl)
{
  assert(cl);
  return cl->appNodes;
}



GWEN_XMLNODE *LC_Client_GetCardNodes(const LC_CLIENT *cl)
{
  assert(cl);
  return cl->cardNodes;
}



GWEN_MSGENGINE *LC_Client_GetMsgEngine(const LC_CLIENT *cl)
{
  assert(cl);
  return cl->msgEngine;
}



int LC_Client_GetReaderAndDriverType(const LC_CLIENT *cl,
                                     const char *readerName,
                                     GWEN_BUFFER *driverType,
                                     GWEN_BUFFER *readerType,
                                     uint32_t *pReaderFlags)
{
  GWEN_DB_NODE *dbDriver;

  dbDriver=GWEN_DB_FindFirstGroup(lc_client__driver_db, "driver");
  while (dbDriver) {
    const char *sDriverName;

    sDriverName=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, NULL);
    if (sDriverName) {
      GWEN_DB_NODE *dbReader;

      dbReader=GWEN_DB_FindFirstGroup(dbDriver, "reader");
      while (dbReader) {
        const char *sReaderName;
        const char *sTmpl;

        sReaderName=GWEN_DB_GetCharValue(dbReader, "readerType", 0, NULL);
        sTmpl=GWEN_DB_GetCharValue(dbReader, "devicePathTmpl", 0, NULL);
        if (sReaderName && sTmpl) {
          if (-1!=GWEN_Text_ComparePattern(readerName, sTmpl, 1)) {
            /* reader found */
            GWEN_Buffer_AppendString(driverType, sDriverName);
            GWEN_Buffer_AppendString(readerType, sReaderName);
            *pReaderFlags=LC_ReaderFlags_fromDb(dbReader, "flags");
            DBG_INFO(LC_LOGDOMAIN,
                     "Reader [%s] is [%s]/[%s], %08x",
                     readerName, sDriverName, sReaderName, *pReaderFlags);
            return 0;
          }
        }
        else {
          DBG_INFO(LC_LOGDOMAIN, "Either reader name or template missing");
        }
        dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
      }
    }
    else {
      DBG_INFO(LC_LOGDOMAIN, "Driver name is missing");
    }
    dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
  }

  return GWEN_ERROR_NOT_FOUND;
}




