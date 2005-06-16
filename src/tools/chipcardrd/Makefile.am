
INCLUDES=@all_includes@

DEFS=@DEFS@ \
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_DEFAULT_DHFILE=\"@LC_DEFAULT_DHFILE@\"\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2s_config_am@\" \
  -DLC_DEFAULT_PIDDIR=\"@LC_DEFAULT_PIDDIR@\" \
  -DLC_DEFAULT_LOGDIR=\"@LC_DEFAULT_LOGDIR@\"


sbin_PROGRAMS=chipcardrd
noinst_HEADERS=chipcardrd_p.h

chipcardrd_SOURCES=\
  chipcardrd.c
chipcardrd_LDADD=@gwenhywfar_libs@
chipcardrd_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

