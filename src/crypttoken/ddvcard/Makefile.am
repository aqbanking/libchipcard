DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/crypttoken

crypttokenplugin_DATA=ddvcard.xml

crypttokenplugin_LTLIBRARIES=ddvcard.la
noinst_HEADERS=\
  ddvcard_p.h

iheaderdir=${includedir}/chipcard2-client/crypttoken
#iheader_HEADERS=

ddvcard_la_SOURCES=\
  ddvcard.c

ddvcard_la_LDFLAGS=-module
ddvcard_la_LIBADD=-L../../lib/chipcard2-client -lchipcard2c

MAINTAINERCLEANFILES=Makefile.in

