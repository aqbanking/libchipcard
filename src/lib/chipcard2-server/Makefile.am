SUBDIRS=server driver

AM_CFLAGS=\
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2d_config_am@\"


EXTRA_DIST=chipcard2.h.in version.h.in

lib_LTLIBRARIES=libchipcard2s.la libchipcard2d.la

libchipcard2s_la_SOURCES=dummy.c
libchipcard2s_la_LIBADD= @gwenhywfar_libs@ @libusb_libs@ \
  server/libserver.la


libchipcard2d_la_SOURCES=dummy.c
libchipcard2d_la_LIBADD= @gwenhywfar_libs@ @libusb_libs@\
  driver/libdriver.la

