SUBDIRS=server driver

AM_CFLAGS=\
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2d_config_am@\"


#EXTRA_DIST=

lib_LTLIBRARIES=libchipcard2d.la
noinst_LTLIBRARIES=libchipcard2s.la

libchipcard2s_la_SOURCES=dummy.c
libchipcard2s_la_LIBADD=server/libserver.la


libchipcard2d_la_SOURCES=dummy.c
libchipcard2d_la_LIBADD= @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@\
  driver/libdriver.la

