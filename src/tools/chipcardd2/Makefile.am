
INCLUDES=@all_includes@ -I../../lib/chipcard2-server/server

DEFS=@DEFS@ \
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_DEFAULT_DHFILE=\"@LC_DEFAULT_DHFILE@\"\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2s_config_am@\" \
  -DLC_DEFAULT_PIDDIR=\"@LC_DEFAULT_PIDDIR@\" \
  -DLC_DEFAULT_LOGDIR=\"@LC_DEFAULT_LOGDIR@\"


sbin_PROGRAMS=chipcardd2
noinst_HEADERS=chipcardd2_p.h

chipcardd2_SOURCES=\
  addreader.c \
  delreader.c \
  testreader.c \
  init.c \
  mkcert.c \
  chipcardd2.c
chipcardd2_LDADD=-L../../lib/chipcard2-server -lchipcard2s @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
chipcardd2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

