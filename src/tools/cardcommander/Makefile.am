bin_PROGRAMS=cardcommander2

INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

CLIENT_LIB=-lchipcard2c

cardcommander2_SOURCES = cardcommander.cpp
cardcommander2_LDADD = -L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
cardcommander2_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

