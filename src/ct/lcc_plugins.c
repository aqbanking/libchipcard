/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gwenhywfar/plugin.h>



GWEN_PLUGIN *ct_ddvcard_factory(GWEN_PLUGIN_MANAGER *pm,
				const char *modName,
				const char *fileName);

GWEN_PLUGIN *ct_starcoscard_factory(GWEN_PLUGIN_MANAGER *pm,
				    const char *modName,
				    const char *fileName);



int LC_Plugins_Init() {
#ifdef LC_ENABLE_INIT_PLUGINS
  GWEN_PLUGIN_MANAGER *pm;
  GWEN_PLUGIN *p;

  pm=GWEN_PluginManager_FindPluginManager("ct");
  if (pm) {
    p=ct_ddvcard_factory(pm, "ddvcard", NULL);
    if (p)
      GWEN_PluginManager_AddPlugin(pm, p);
    p=ct_starcoscard_factory(pm, "starcoscard", NULL);
    if (p)
      GWEN_PluginManager_AddPlugin(pm, p);
  }

#endif
  return 0;
}




