bin_PROGRAMS=cardcommander2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

cardcommander2_SOURCES = cardcommander.cpp
cardcommander2_LDADD = -L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
cardcommander2_LDFLAGS=@STRIPALL@
