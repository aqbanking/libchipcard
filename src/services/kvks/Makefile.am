INCLUDES=@all_includes@

EXTRA_DIST=kvks.xml.in kvks.xml

servicedir=@LC_SERVICE_PATH@
service_PROGRAMS=kvks
service_DATA=kvks.xml


SERVICE_LIB=-lchipcard2sv
CLIENT_LIB=-lchipcard2c

noinst_HEADERS=\
  kvks_p.h \
  kvkscard_p.h \
  kvkscard.h

kvks_SOURCES=\
  kvks.c \
  kvkscard.c

kvks_LDADD=\
 -L../../lib/chipcard2-service $(SERVICE_LIB) \
 -L../../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@
kvks_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

