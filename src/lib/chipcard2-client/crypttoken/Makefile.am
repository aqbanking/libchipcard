
DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I..

noinst_LTLIBRARIES=libctcard.la

iheaderdir=${includedir}/chipcard2-client/client
iheader_HEADERS=\
  ct_card.h

libctcard_la_SOURCES=\
  ct_card.c

MAINTAINERCLEANFILES=Makefile.in

