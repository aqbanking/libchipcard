bin_PROGRAMS=chipcard-tool

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

chipcard_tool_SOURCES=\
  main.c \
  checkreaders.c \
  getatr.c \
  listreaders.c
chipcard_tool_LDADD=-L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
chipcard_tool_LDFLAGS=@STRIPALL@
