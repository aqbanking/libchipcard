SUBDIRS=client cards mon

AM_CFLAGS=\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"


EXTRA_DIST=chipcard2.h.in version.h.in

lib_LTLIBRARIES=libchipcard2c.la

iheaderdir=${includedir}/chipcard2-client
iheader_HEADERS=\
  chipcard2.h \
  version.h

libchipcard2c_la_SOURCES=dummy.c
libchipcard2c_la_LIBADD= @gwenhywfar_libs@ \
  client/libclient.la \
  cards/libcards.la \
  mon/libmon.la


