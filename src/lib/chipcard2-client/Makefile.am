SUBDIRS=client cards mon fs

AM_CFLAGS=\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"


lib_LTLIBRARIES=libchipcard2c.la

libchipcard2c_la_SOURCES=dummy.c
libchipcard2c_la_LIBADD= @gwenhywfar_libs@ \
  client/libclient.la \
  cards/libcards.la \
  mon/libmon.la \
  fs/libfs.la


