/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef CHIPCARD_CLIENT_CLIENT_XML_H
#define CHIPCARD_CLIENT_CLIENT_XML_H


int LC_Client_ReadXmlFiles(GWEN_XMLNODE *root, const char *basedir, const char *tPlural, const char *tSingular);
GWEN_XMLNODE *LC_Client_GetAppNode(LC_CLIENT *cl, const char *appName);
GWEN_XMLNODE *LC_Client_GetCardNode(LC_CLIENT *cl, const char *cardName);



#endif

