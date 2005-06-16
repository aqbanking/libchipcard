bin_PROGRAMS=geldkarte2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

geldkarte2_SOURCES=\
  main.c
geldkarte2_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
geldkarte2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

