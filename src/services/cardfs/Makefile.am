INCLUDES=@all_includes@

servicedir=@LC_SERVICE_PATH@
service_PROGRAMS=cardfs

if IS_WINDOWS
  SERVICE_LIB=-lchipcard2sv32_$(LIBCHIPCARD_SO_EFFECTIVE)
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  SERVICE_LIB=-lchipcard2sv
  CLIENT_LIB=-lchipcard2c
endif

noinst_HEADERS=\
  cardfs_p.h

cardfs_SOURCES=\
  cardfs.c

cardfs_LDADD=\
 -L../../lib/chipcard2-service $(SERVICE_LIB) \
 -L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
cardfs_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

