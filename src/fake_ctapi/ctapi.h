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


#ifndef CHIPCARD_CTAPI_H
#define CHIPCARD_CTAPI_H


#define CT_API_LOGDOMAIN "ctapi"

#define CT_API_AD_HOST		2
#define CT_API_AD_REMOTE	5

#define CT_API_AD_CT		1
#define CT_API_AD_ICC1		0
#define CT_API_AD_ICC2		2
#define CT_API_AD_ICC3		3
#define CT_API_AD_ICC4		4
#define CT_API_AD_ICC5		5
#define CT_API_AD_ICC6		6
#define CT_API_AD_ICC7		7
#define CT_API_AD_ICC8		8
#define CT_API_AD_ICC9		9
#define CT_API_AD_ICC10		10
#define CT_API_AD_ICC11		11
#define CT_API_AD_ICC12		12
#define CT_API_AD_ICC13		13
#define CT_API_AD_ICC14		14


#define CT_API_RV_OK		0
#define CT_API_RV_ERR_INVALID	-1
#define CT_API_RV_ERR_CT	-8
#define CT_API_RV_ERR_TRANS	-10
#define CT_API_RV_ERR_MEMORY	-11
#define CT_API_RV_ERR_HOST	-127
#define CT_API_RV_ERR_HTSI	-128

#define CT			1
#define HOST			2


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char CT_init(unsigned short ctn, unsigned short pn);
char CT_data(unsigned short ctn,
             unsigned char *dad,
             unsigned char *sad,
             unsigned short lenc,
             unsigned char *command,
             unsigned short *lenr,
             unsigned char *response);
char CT_close(unsigned short ctn);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

