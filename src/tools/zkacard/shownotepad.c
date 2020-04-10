/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Sat Jun 23 2018
 copyright   : (C) 2018 by Martin Preuss, Stefan Bayer
 email       : martin@libchipcard.de, stefan.bayer@stefanbayer.net


 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "global.h"

#include <gwenhywfar/text.h>
#include <gwenhywfar/args.h>
#include <gwenhywfar/gui.h>
#include <chipcard/client.h>
#include <chipcard/cards/zkacard.h>
#include <chipcard/ct/ct_card.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


int showNotepad(GWEN_DB_NODE *dbArgs, int argc, char **argv)
{

    GWEN_DB_NODE *db=NULL;
    GWEN_DB_NODE *dbNotePad;
    GWEN_DB_NODE *dbRecord;

    int rv;
    int keyNumber;
    int j;
    const char *s;
    int i;
    LC_CLIENT_RESULT res;
    GWEN_BUFFER *mbuf;
    const GWEN_CRYPT_TOKEN_CONTEXT *cctx;
    uint8_t cnt;
    LC_CARD *hcard=0;
    LC_CLIENT *lc;
    uint8_t v=0;
    uint8_t guiid=0;

    const GWEN_ARGS args[]= {
            {
                    GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
                    GWEN_ArgsType_Int,            /* type */
                    "help",                       /* name */
                    0,                            /* minnum */
                    0,                            /* maxnum */
                    "h",                          /* short option */
                    "help",                       /* long option */
                    "Show this help screen",      /* short description */
                    "Show this help screen"       /* long description */
            }
    };

    db=GWEN_DB_GetGroup(dbArgs, GWEN_DB_FLAGS_DEFAULT, "local");
    rv=GWEN_Args_Check(argc, argv, 1,
            0 /*GWEN_ARGS_MODE_ALLOW_FREEPARAM*/,
            args,
            db);

    if (rv==GWEN_ARGS_RESULT_ERROR) {
        fprintf(stderr, "ERROR: Could not parse arguments\n");
        return 1;
    }
    else if (rv==GWEN_ARGS_RESULT_HELP) {
        GWEN_BUFFER *ubuf;

        ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
        if (GWEN_Args_Usage(args, ubuf, GWEN_ArgsOutType_Txt)) {
            fprintf(stderr, "ERROR: Could not create help string\n");
            return 1;
        }
        fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
        GWEN_Buffer_free(ubuf);
        return 0;
    }

    /* get card */

    res= ZkaCardTool_OpenCard(&lc,&hcard);

    if (res!=LC_Client_ResultOk) {
        showError(hcard, res, "StartWait");
        ZkaCardTool_CloseCard(lc,hcard);
        return RETURNVALUE_WORK;
    }

    res = ZkaCardTool_EnsureAccessPin(hcard, guiid, 0);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in PIN verification (%d)", res);
      ZkaCardTool_CloseCard(lc,hcard);
      return GWEN_ERROR_NOT_OPEN;
    }



    res=LC_Card_SelectMf(hcard);
    if (res!=LC_Client_ResultOk) {
        DBG_ERROR(LC_LOGDOMAIN, "Error selecting MF (%d)", res);
        ZkaCardTool_CloseCard(lc,hcard);
        return GWEN_ERROR_NOT_OPEN;
    }

    res=LC_Card_SelectDf(hcard, "DF_NOTEPAD");
    if (res!=LC_Client_ResultOk) {
        DBG_ERROR(LC_LOGDOMAIN, "Error selecting DF_NOTEPAD (%d)", res);
        ZkaCardTool_CloseCard(lc,hcard);
        return GWEN_ERROR_NOT_OPEN;
    }

    res=LC_Card_SelectEf(hcard, "EF_NOTEPAD");
    if (res!=LC_Client_ResultOk) {
        DBG_ERROR(LC_LOGDOMAIN, "Error selecting EF_NOTEPAD (%d)", res);
        ZkaCardTool_CloseCard(lc,hcard);
        return GWEN_ERROR_NOT_OPEN;
    }

    dbNotePad=GWEN_DB_Group_new("notepad");
    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    for (i=1; i< ZKACARD_NUM_CONTEXT; i++) {
        /* read record */
        DBG_INFO(LC_LOGDOMAIN, "Reading entry %d", i);
        res=LC_Card_IsoReadRecord(hcard, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, i, mbuf);
        GWEN_Buffer_Rewind(mbuf);
        if (res!=LC_Client_ResultOk) {
            if (LC_Card_GetLastSW1(hcard)==0x6a &&
                    LC_Card_GetLastSW2(hcard)==0x83) {
                DBG_INFO(LC_LOGDOMAIN, "All records read (%d)", i-1);
                break;
            }
            else {
                DBG_ERROR(LC_LOGDOMAIN, "Error reading record %d of EF_NOTEPAD (%d)", i, res);
            }
        }
        else {
            GWEN_DB_NODE *dbEntry;

            /* parse record */
            if (GWEN_Buffer_GetBytesLeft(mbuf)) {
                DBG_INFO(LC_LOGDOMAIN, "Parsing entry %d", i);
                dbEntry=GWEN_DB_Group_new("entry");
                GWEN_Buffer_Rewind(mbuf);
                res=LC_Card_ParseRecord(hcard, i, mbuf, dbEntry);
                if (res!=LC_Client_ResultOk) {
                    DBG_ERROR(LC_LOGDOMAIN, "Error parsing record %d of EF_NOTEPAD (%d)", i, res);
                    GWEN_DB_Group_free(dbEntry);
                }

                /* add new entry */
                DBG_INFO(LC_LOGDOMAIN, "Adding entry %d", i);
                GWEN_DB_AddGroup(dbNotePad, dbEntry);
            }
            else {
                DBG_INFO(LC_LOGDOMAIN, "Entry %d is empty", i);
            }
        }
        GWEN_Buffer_Reset(mbuf);
    }
    GWEN_Buffer_free(mbuf);
    cnt=1;
    dbRecord=GWEN_DB_GetFirstGroup(dbNotePad);
    printf("Notepad\n\n");

    while (dbRecord && cnt< ZKACARD_NUM_CONTEXT) {
        GWEN_DB_NODE *dbCtx;

        dbCtx=GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry/hbciData");
        if (dbCtx) {
            printf("HBCI Context #%d\n\n", cnt);
            GWEN_DB_Dump(dbCtx, 2);
            printf("\n\n");
            cnt++;
        }
        else {
            printf("Other Application Data\n");
            GWEN_DB_Dump(dbRecord, 2);
            printf("\n\n");
        }
        dbRecord=GWEN_DB_GetNextGroup(dbRecord);
    }


    GWEN_DB_Group_free(dbNotePad);

    /*close card */
    res= ZkaCardTool_CloseCard(lc,hcard);


    return 0;




}


