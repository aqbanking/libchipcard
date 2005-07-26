DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../ -I../client -I../cards

crypttokenplugindir=$(gwenhywfar_plugins)/crypttoken

crypttokenplugin_DATA=ddv.xml

crypttokenplugin_LTLIBRARIES=ddv.la
noinst_HEADERS=\
  ct_ddv_p.h

iheaderdir=${includedir}/chipcard2-client/crypttoken
#iheader_HEADERS=

ddv_la_SOURCES=\
  ct_ddv.c

ddv_la_LDFLAGS=-module
ddv_la_LIBADD=-L../lib/chipcard2-client -lchipcard2c

MAINTAINERCLEANFILES=Makefile.in

