DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/crypttoken

crypttokenplugin_DATA=starcoscard.xml

crypttokenplugin_LTLIBRARIES=starcoscard.la
noinst_HEADERS=\
  starcoscard_p.h

iheaderdir=${includedir}/chipcard2-client/crypttoken
#iheader_HEADERS=

starcoscard_la_SOURCES=\
  starcoscard.c

starcoscard_la_LDFLAGS=-module
starcoscard_la_LIBADD=-L../../lib/chipcard2-client -lchipcard2c

MAINTAINERCLEANFILES=Makefile.in

