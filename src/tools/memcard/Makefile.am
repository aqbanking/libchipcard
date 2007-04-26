bin_PROGRAMS=memcard2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

CLIENT_LIB=-lchipcard2c

memcard2_SOURCES=\
  main.c
memcard2_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
memcard2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

