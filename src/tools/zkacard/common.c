/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Fri Apr 10 2020
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

static int ZkaCardTool__GetPinName(GWEN_BUFFER *nameBuffer,
        GWEN_CRYPT_PINTYPE pt,
        const char *tname,
        const char *dname)
{
    GWEN_Buffer_AppendString(nameBuffer, "PASSWORD_");

    GWEN_Buffer_AppendString(nameBuffer, tname);
    GWEN_Buffer_AppendString(nameBuffer, "_");
    GWEN_Buffer_AppendString(nameBuffer, dname);
    if (pt==GWEN_Crypt_PinType_Manage)
        GWEN_Buffer_AppendString(nameBuffer, ":MANAGE");
}

static int ZkaCardTool__SetPinStatus(
        GWEN_CRYPT_PINTYPE pt,
        GWEN_CRYPT_PINENCODING pe,
        GWEN_UNUSED uint32_t flags,
        const unsigned char *buffer,
        unsigned int pinLength,
        int isOk,
        GWEN_BUFFER *nameBuffer,
        uint32_t gid)
{

    int rv;
    unsigned char ibuffer[256];



    if (pe!=GWEN_Crypt_PinEncoding_Ascii) {
        if (pinLength>=sizeof(ibuffer)) {
            DBG_ERROR(GWEN_LOGDOMAIN, "Pin too long");
            return GWEN_ERROR_BUFFER_OVERFLOW;
        }
        memset(ibuffer, 0, sizeof(ibuffer));
        memmove(ibuffer, buffer, pinLength);
        rv=GWEN_Crypt_TransformPin(pe,
                GWEN_Crypt_PinEncoding_Ascii,
                ibuffer,
                sizeof(ibuffer)-1,
                &pinLength);
        if (rv) {
            DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
            return rv;
        }
        buffer=ibuffer;
    }

    nameBuffer=GWEN_Buffer_new(0, 256, 0, 1);

    rv=GWEN_Gui_SetPasswordStatus(GWEN_Buffer_GetStart(nameBuffer),
            (const char *)buffer,
            isOk?GWEN_Gui_PasswordStatus_Ok:
                    GWEN_Gui_PasswordStatus_Bad, gid);
    memset(ibuffer, 0, sizeof(ibuffer));
    GWEN_Buffer_free(nameBuffer);
    return rv;

}

static uint32_t ZkaCardTool__BeginEnterPin(GWEN_UNUSED GWEN_CRYPT_PINTYPE pt,
        uint32_t gid)
{
    char buffer[512];

    buffer[0]=0;
    buffer[sizeof(buffer)-1]=0;

    snprintf(buffer, sizeof(buffer)-1, "%s",
            I18N("Please enter your PIN into the card reader."
                    "<html>"
                    "Please enter your PIN into the card reader."
                    "</html>"));
    return GWEN_Gui_ShowBox(GWEN_GUI_SHOWBOX_FLAGS_BEEP,
            I18N("Secure PIN Input"),
            buffer, gid);
}

static int ZkaCardTool__EndEnterPin(GWEN_UNUSED GWEN_CRYPT_PINTYPE pt,
        GWEN_UNUSED int ok,
        uint32_t id)
{

    GWEN_Gui_HideBox(id);

    return 0;
}

static int ZkaCardTool__GetPin(  GWEN_CRYPT_PINTYPE pt,
        GWEN_CRYPT_PINENCODING pe,
        uint32_t flags,
        unsigned char *pwbuffer,
        unsigned int minLength,
        unsigned int maxLength,
        unsigned int *pinLength,
        const char *tname,
        const char *dname,
        uint32_t gid)
{
    int rv;
    const char *mode;
    const char *numeric_warning = "";
    char buffer[512];
    GWEN_BUFFER *nameBuffer;


    if (pt==GWEN_Crypt_PinType_Access)
        mode=I18N("access password");
    else if (pt==GWEN_Crypt_PinType_Manage)
        mode=I18N("manager password");
    else
        mode=I18N("password");

    buffer[0]=0;
    buffer[sizeof(buffer)-1]=0;
    if (flags & GWEN_GUI_INPUT_FLAGS_NUMERIC) {
        numeric_warning = I18N("\nYou must only enter numbers, not letters.");
    }

    if (flags & GWEN_GUI_INPUT_FLAGS_CONFIRM) {
        snprintf(buffer, sizeof(buffer)-1,
                I18N("Please enter a new %s for \n"
                        "%s\n"
                        "The password must be at least %d characters long.%s"
                        "<html>"
                        "Please enter a new %s for <i>%s</i>. "
                        "The password must be at least %d characters long.%s"
                        "</html>"),
                        mode,
                        dname,
                        minLength,
                        numeric_warning,
                        mode,
                        dname,
                        minLength,
                        numeric_warning);
    }
    else {
        snprintf(buffer, sizeof(buffer)-1,
                I18N("Please enter the %s for \n"
                        "%s\n"
                        "%s<html>"
                        "Please enter the %s for <i>%s</i>.%s"
                        "</html>"),
                        mode,
                        dname,
                        numeric_warning,
                        mode,
                        dname,
                        numeric_warning);
    }

    nameBuffer=GWEN_Buffer_new(0, 256, 0, 1);


    ZkaCardTool__GetPinName(nameBuffer,pe,tname,dname);



    rv=GWEN_Gui_GetPassword(flags,
            GWEN_Buffer_GetStart(nameBuffer),
            I18N("Enter Password"),
            buffer,
            (char *)pwbuffer,
            minLength,
            maxLength,
            GWEN_Gui_PasswordMethod_Text, NULL,
            gid);
    GWEN_Buffer_free(nameBuffer);
    if (rv) {
        DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
        return rv;
    }

    *pinLength=strlen((char *)pwbuffer);

    if (pe!=GWEN_Crypt_PinEncoding_Ascii) {
        rv=GWEN_Crypt_TransformPin(GWEN_Crypt_PinEncoding_Ascii,
                pe,
                pwbuffer,
                maxLength,
                pinLength);
        if (rv) {
            DBG_INFO(GWEN_LOGDOMAIN, "here (%d)", rv);
            return rv;
        }
    }

    return 0;
}



static int ZkaCardTool__EnterPinWithPinInfo(LC_CARD *hcard,
        GWEN_CRYPT_PINTYPE pt,
        const LC_PININFO *pi,
        int16_t  reset,
        uint32_t guiid)
{
    LC_CLIENT_RESULT res;
    int maxErrors;
    int currentErrors;

    assert(hcard);
    assert(pi);



    res=LC_Card_GetPinStatus(hcard,
            LC_PinInfo_GetId(pi),
            &maxErrors,
            &currentErrors);
    if (res!=LC_Client_ResultNotSupported) {
        if (res!=LC_Client_ResultOk) {
            DBG_ERROR(LC_LOGDOMAIN,
                    "Unable to read status of pin %x (%d)",
                    LC_PinInfo_GetId(pi),
                    res);
            return LC_Crypt_Token_ResultToError(res);
        }

        if ((currentErrors!=maxErrors && reset==0)) {
            DBG_ERROR(LC_LOGDOMAIN,
                    "Bad pin entered at least once before, aborting");
            printf("Bad pin entered at least once before, aborting\n");
            return GWEN_ERROR_ABORTED;

        }
        else if ((currentErrors!=maxErrors && reset==1))
        {
            printf("Bad pin entered at least once before.\n");
            printf("Maximum number of tries:   %d\n",maxErrors);
            printf("Remaining number of tries: %d\n",currentErrors);
            printf("Enter pin to reset try counter...");
        }
        else if (reset == 1)
        {
            printf("PIN PTC ok. Nothing to be done.\n");


            DBG_ERROR(LC_LOGDOMAIN,
                    "Maximum pin try counter. Nothing to be done");
            return 0;
        }
    }
    else {
        DBG_INFO(LC_LOGDOMAIN,
                "Unable to read pin status for pin %02x (not supported)",
                LC_PinInfo_GetId(pi));
    }

    if ((pt!=GWEN_Crypt_PinType_Manage) &&
            (LC_Card_GetReaderFlags(hcard) & LC_READER_FLAGS_KEYPAD)) {
        uint32_t bid;
        int triesLeft=-1;

        DBG_INFO(LC_LOGDOMAIN, "Terminal has a keypad, will ask for pin.");

        /* tell the user about pin verification */
        bid=ZkaCardTool__BeginEnterPin(pt, guiid);
        if (bid==0) {
            DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
            return GWEN_ERROR_GENERIC;
        }

        res=LC_Card_IsoPerformVerification(hcard, 0, pi, &triesLeft);

        if (res!=LC_Client_ResultOk) {
            /* tell the user about end of pin verification */
            ZkaCardTool__EndEnterPin(pt, 0, bid);
            DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                    LC_Card_GetLastSW1(hcard),
                    LC_Card_GetLastSW2(hcard),
                    LC_Card_GetLastText(hcard));
            if (LC_Card_GetLastSW1(hcard)==0x90 &&
                    LC_Card_GetLastSW2(hcard)==0x00 && reset)
            {
                res=LC_Card_GetPinStatus(hcard,
                        LC_PinInfo_GetId(pi),
                        &maxErrors,
                        &currentErrors);
                printf("PIN verification succesfull\n");
                printf("Maximum number of tries:   %d\n",maxErrors);
                printf("Remaining number of tries: %d\n",currentErrors);
                return 0;
            }
            else if (LC_Card_GetLastSW1(hcard)==0x63) {
                switch (LC_Card_GetLastSW2(hcard)) {
                case 0xc0: /* no error left */
                    if (reset) printf("PIN verification failed. PIN is disabled.\n");
                    return GWEN_ERROR_BAD_PIN_0_LEFT;
                case 0xc1: /* one left */
                    if (reset)  printf("PIN verification failed. 1 try left.\n");
                    return GWEN_ERROR_BAD_PIN_1_LEFT;
                case 0xc2: /* two left */
                    printf("PIN verification failed. 1 try left.\n");
                    return GWEN_ERROR_BAD_PIN_2_LEFT;
                default:   /* unknown error */
                    if (reset) printf("Unkown error in PIN verification...\n");
                    return GWEN_ERROR_BAD_PIN;
                } // switch
            }
            else if (LC_Card_GetLastSW1(hcard)==0x69 &&
                    LC_Card_GetLastSW2(hcard)==0x83) {
                DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
                if (reset) printf("PIN is disabled.\n");
                return GWEN_ERROR_IO;
            }
            else if (LC_Card_GetLastSW1(hcard)==0x64 &&
                    LC_Card_GetLastSW2(hcard)==0x01) {
                DBG_ERROR(LC_LOGDOMAIN, "Aborted by user");
                if (reset) printf("Aborted by user.\n");
                return GWEN_ERROR_USER_ABORTED;
            }
            else {
                if (triesLeft>=0) {
                    switch (triesLeft) {
                    case 0: /* no error left */
                        if (reset) printf("PIN verification failed. PIN is disabled.\n");
                        return GWEN_ERROR_BAD_PIN_0_LEFT;
                    case 1: /* one left */
                        if (reset) printf("PIN verification failed. 1 try left.\n");
                        return GWEN_ERROR_BAD_PIN_1_LEFT;
                    case 2: /* two left */
                        if (reset) printf("PIN verification failed. 2 tries left.\n");
                        return GWEN_ERROR_BAD_PIN_2_LEFT;
                    default:   /* unknown count */
                        if (reset) printf("Unknown PIN verification error.\n");
                        return GWEN_ERROR_BAD_PIN;
                    } // switch
                }

                return GWEN_ERROR_IO;
            }
        } /* if not ok */
        else {
            /* PIN ok */
            DBG_INFO(LC_LOGDOMAIN, "Pin ok");
            ZkaCardTool__EndEnterPin(pt, 1, bid);
            if (reset)
            {
                res=LC_Card_GetPinStatus(hcard,
                        LC_PinInfo_GetId(pi),
                        &maxErrors,
                        &currentErrors);
                printf("PIN verification succesfull\n");
                printf("Maximum number of tries:   %d\n",maxErrors);
                printf("Remaining number of tries: %d\n",currentErrors);
            }
        }
    } /* if hasKeyPad */
    else {
#if 0
        DBG_ERROR(LC_LOGDOMAIN, "Card Reader without key pad!\nCould not verify pin!\n");
        return GWEN_ERROR_IO;
#else
        unsigned char pinBuffer[64];
        int mres;
        int pinMaxLen;
        unsigned int pinLength;
        unsigned int origPinLength;
        uint32_t pflags=0;
        GWEN_CRYPT_PINENCODING pe;
        int triesLeft=-1;
        GWEN_DB_NODE *dbCardData;;
        GWEN_BUFFER *nameBuffer;

        DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
        memset(pinBuffer, 0, sizeof(pinBuffer));

        pe=LC_PinInfo_GetEncoding(pi);
        if (pt==GWEN_Crypt_PinType_Manage)
            pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
        pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
        pinLength=0;
        pinMaxLen=LC_PinInfo_GetMaxLength(pi);
        if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer)-1)
            pinMaxLen=sizeof(pinBuffer)-1;

        dbCardData=LC_ZkaCard_GetCardDataAsDb(hcard);

        ZkaCardTool__GetPinName(nameBuffer,pe,"zkacard",GWEN_DB_GetCharValue(dbCardData, "cardNumber", 0, 0));

        mres= ZkaCardTool__GetPin(
                pt,
                pe,
                pflags,
                pinBuffer,
                LC_PinInfo_GetMinLength(pi),
                pinMaxLen,
                &pinLength,
                "zkacard",
                GWEN_DB_GetCharValue(dbCardData, "cardNumber", 0, 0),
                guiid);
        if (mres!=0) {
            DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
            memset(pinBuffer, 0, sizeof(pinBuffer));
            return mres;
        }
        origPinLength=pinLength;

        if (pinLength<pinMaxLen && LC_PinInfo_GetFiller(pi)) {
            int i;
            unsigned char c;

            c=(unsigned char)LC_PinInfo_GetFiller(pi);
            for (i=pinLength; i<pinMaxLen; i++)
                pinBuffer[i]=c;
            pinLength=pinMaxLen;
        }

        DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");
        res=LC_Card_IsoVerifyPin(hcard,
                0,
                pi,
                pinBuffer,
                pinLength,
                &triesLeft);
        if (res!=LC_Client_ResultOk) {
            DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                    LC_Card_GetLastSW1(hcard),
                    LC_Card_GetLastSW2(hcard),
                    LC_Card_GetLastText(hcard));

            if (LC_Card_GetLastSW1(hcard)==0x90 &&
                    LC_Card_GetLastSW2(hcard)==0x00 && reset)
            {
                res=LC_Card_GetPinStatus(hcard,
                        LC_PinInfo_GetId(pi),
                        &maxErrors,
                        &currentErrors);
                printf("PIN verification succesfull\n");
                printf("Maximum number of tries:   %d\n",maxErrors);
                printf("Remaining number of tries: %d\n",currentErrors);
                return 0;
            }
            else if (LC_Card_GetLastSW1(hcard)==0x63) {
                /* set pin status */
                ZkaCardTool__SetPinStatus(
                        pt,
                        pe,
                        pflags,
                        pinBuffer,
                        origPinLength,
                        0,
                        nameBuffer,
                        guiid);

                switch (LC_Card_GetLastSW2(hcard)) {
                case 0xc0: /* no error left */
                    if (reset) printf("PIN verification failed. PIN is disabled.\n");
                    return GWEN_ERROR_BAD_PIN_0_LEFT;
                case 0xc1: /* one left */
                    printf("PIN verification failed. 1 try left.\n");
                    return GWEN_ERROR_BAD_PIN_1_LEFT;
                case 0xc2: /* two left */
                    printf("PIN verification failed. 1 try left.\n");
                    return GWEN_ERROR_BAD_PIN_2_LEFT;
                default:
                    if (reset) printf("Unkown error in PIN verification...\n");
                    return GWEN_ERROR_BAD_PIN;
                } // switch
            }
            else if (LC_Card_GetLastSW1(hcard)==0x69 &&
                    LC_Card_GetLastSW2(hcard)==0x83) {
                /* set pin status */
                ZkaCardTool__SetPinStatus(
                        pt,
                        pe,
                        pflags,
                        pinBuffer,
                        origPinLength,
                        0,
                        nameBuffer,
                        guiid);
                DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
                if (reset) printf("PIN is disabled.\n");
                return GWEN_ERROR_IO;
            }
            else if (LC_Card_GetLastSW1(hcard)==0x64 &&
                    LC_Card_GetLastSW2(hcard)==0x01) {
                if (reset) printf("Aborted by user.\n");
                return GWEN_ERROR_USER_ABORTED;
            }
            else {
                if (triesLeft>=0) {
                    /* set pin status */
                    ZkaCardTool__SetPinStatus(
                            pt,
                            pe,
                            pflags,
                            pinBuffer,
                            origPinLength,
                            0,
                            nameBuffer,
                            guiid);
                    switch (triesLeft) {
                    case 0: /* no error left */
                        if (reset) printf("PIN verification failed. PIN is disabled.\n");
                        return GWEN_ERROR_BAD_PIN_0_LEFT;
                    case 1: /* one left */
                        if (reset) printf("PIN verification failed. 1 try left.\n");
                        return GWEN_ERROR_BAD_PIN_1_LEFT;
                    case 2: /* two left */
                        if (reset) printf("PIN verification failed. 2 tries left.\n");
                        return GWEN_ERROR_BAD_PIN_2_LEFT;
                    default:   /* unknown count */
                        if (reset) printf("Unknown PIN verification error.\n");
                        return GWEN_ERROR_BAD_PIN;
                    } // switch
                }
                DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
                return GWEN_ERROR_IO;
            }
        } // if not ok
        else {
            DBG_INFO(LC_LOGDOMAIN, "PIN ok");
            /* set pin status */
            ZkaCardTool__SetPinStatus(
                    pt,
                    pe,
                    pflags,
                    pinBuffer,
                    origPinLength,
                    1,
                    nameBuffer,
                    guiid);
            if (reset)
            {
                res=LC_Card_GetPinStatus(hcard,
                        LC_PinInfo_GetId(pi),
                        &maxErrors,
                        &currentErrors);
                printf("PIN verification succesfull\n");
                printf("Maximum number of tries:   %d\n",maxErrors);
                printf("Remaining number of tries: %d\n",currentErrors);
            }
        }
#endif
    } // if no keyPad

    return 0;
}

static
int ZkaCardTool__EnterPin(
        LC_CARD *hcard,
        GWEN_CRYPT_PINTYPE pt,
        uint16_t reset,
        uint32_t guiid)
{
    LC_PININFO *pi;
    int rv;

    assert(hcard);

    if (pt==GWEN_Crypt_PinType_Manage) {
        pi=LC_Card_GetPinInfoByName(hcard, "eg_pin");
    }
    else {
        pi=LC_Card_GetPinInfoByName(hcard, "ch_pin");
    }
    assert(pi);

    rv=ZkaCardTool__EnterPinWithPinInfo(hcard, pt, pi, reset, guiid);
    LC_PinInfo_free(pi);

    return rv;
}

int ZkaCardTool_EnsureAccessPin(LC_CARD *hcard, uint32_t guiid, uint16_t reset)
{

    const LC_PININFO *pi;
    uint16_t haveAccessPin=0;

    if (hcard==0) {
        DBG_ERROR(LC_LOGDOMAIN, "No card.");
        return GWEN_ERROR_NOT_OPEN;
    }

    pi=LC_ZkaCard_GetPinInfo(hcard, 3);

    while (!haveAccessPin) {
        int rv;

        /* enter pin */
        if (pi)
            rv=ZkaCardTool__EnterPinWithPinInfo(hcard,
                    GWEN_Crypt_PinType_Access,
                    pi,
                    reset,
                    guiid);
        else
            rv=ZkaCardTool__EnterPin(hcard,
                    GWEN_Crypt_PinType_Access,
                    reset,
                    guiid);
        if (rv) {
            DBG_ERROR(LC_LOGDOMAIN, "Error in PIN input");
            return rv;
        }
        else
            haveAccessPin=1;
    } /* while !havepin */

    return 0;
}

int16_t ZkaCardTool_OpenCard(LC_CLIENT **lc,LC_CARD **hcard)
{

    int16_t res = 0;
    uint16_t i;

    *lc=LC_Client_new("zkacard", ZKACARDTOOL_PROGRAM_VERSION);
    res=LC_Client_Init(*lc);
    if (res!=LC_Client_ResultOk) {
        showError(0, res, "Init");
        return RETURNVALUE_SETUP;
    }

    res=LC_Client_Start(*lc);
    if (res!=LC_Client_ResultOk) {
        showError(*hcard, res, "StartWait");
        return RETURNVALUE_WORK;
    }

    for (i=0;; i++) {
        const GWEN_STRINGLIST *sl;
        GWEN_STRINGLISTENTRY *se;
        uint8_t found = 0;

        res=LC_Client_GetNextCard(*lc, hcard, 20);
        if (res!=LC_Client_ResultOk) {
            showError(*hcard, res, "GetNextCard");
            return RETURNVALUE_WORK;
        }
        else {
            break;
        }

        res=LC_Client_ReleaseCard(*lc, *hcard);
        if (res!=LC_Client_ResultOk) {
            showError(*hcard, res, "ReleaseCard");
            return RETURNVALUE_WORK;
        }
        LC_Card_free(*hcard);

        if (i>15) {
            fprintf(stderr, "ERROR: No card found.\n");
            return RETURNVALUE_WORK;
        }
    } /* for */
    res=LC_ZkaCard_ExtendCard(*hcard);
    if (res!=LC_Client_ResultOk) {
        fprintf(stderr,
                "ERROR: Error extending zkacard (%d).\n",
                res);
        return RETURNVALUE_WORK;
    }
    /* open card */
    res=LC_Card_Open(*hcard);
    if (res!=LC_Client_ResultOk) {
        fprintf(stderr,
                "ERROR: Error opening zkcard (%d).\n",
                res);
        return RETURNVALUE_WORK;
    }

    return res;
}

int16_t ZkaCardTool_CloseCard(LC_CLIENT *lc,LC_CARD *hcard)
{
    uint16_t res = 0;
    /* close card */

    res=LC_Card_Close(hcard);
    if (res!=LC_Client_ResultOk) {
        showError(hcard, res, "CardClose");
        return RETURNVALUE_WORK;
    }

    res=LC_Client_ReleaseCard(lc, hcard);
    if (res!=LC_Client_ResultOk) {
        showError(hcard, res, "ReleaseCard");
        return RETURNVALUE_WORK;
    }
    LC_Card_free(hcard);

    res=LC_Client_Fini(lc);
    if (res!=LC_Client_ResultOk) {
        showError(0, res, "Init");
        return RETURNVALUE_WORK;
    }

    LC_Client_free(lc);
    return res;

}

