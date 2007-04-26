bin_PROGRAMS=rsacard2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

CLIENT_LIB=-lchipcard2c

rsacard2_SOURCES=\
  main.c
rsacard2_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
rsacard2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

