/***************************************************************************
    begin       : Sun Apr 13 2008
    copyright   : (C) 2008 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef LCC_PLUGINS_H
#define LCC_PLUGINS_H

/**
 * This function should be called when the plugins are statically linked into an
 * application. It registers all included plugins so that they are available for
 * the function @ref GWEN_PluginManager_GetPlugin.
 */

int LC_Plugins_Init();

#endif




