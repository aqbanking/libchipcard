bin_PROGRAMS=kvkcard2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

CLIENT_LIB=-lchipcard2c

kvkcard2_SOURCES=\
  main.c
kvkcard2_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB)
kvkcard2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

