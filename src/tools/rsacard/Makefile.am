bin_PROGRAMS=rsacard2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

rsacard2_SOURCES=\
  main.c \
  generatekey.c \
  activatekey.c \
  deactivatekey.c \
  exportkey.c \
  importkey.c \
  exportbankinfo.c \
  importbankinfo.c \
  initialpin.c
rsacard2_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
rsacard2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

