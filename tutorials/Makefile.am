INCLUDES=@all_includes@

EXTRA_DIST=README examplecard.xml

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

noinst_PROGRAMS=\
 tutorial1a tutorial1b tutorial1c \
 tutorial2 \
 tutorial3


appdatadir=@libchipcard2c_config_am@/apps
appdata_DATA=examplecard.xml

noinst_HEADERS=examplecard.h examplecard_p.h

tutorial1a_SOURCES=tutorial1a.c
tutorial1a_LDADD=-L../src/lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

tutorial1b_SOURCES=tutorial1b.c
tutorial1b_LDADD=-L../src/lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

tutorial1c_SOURCES=tutorial1c.c
tutorial1c_LDADD=-L../src/lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

tutorial2_SOURCES=tutorial2.c
tutorial2_LDADD=-L../src/lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

tutorial3_SOURCES=tutorial3.c examplecard.c
tutorial3_LDADD=-L../src/lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

