INCLUDES=@all_includes@

AM_CFLAGS=-DLC_SERVICE_PATH=\"@LC_SERVICE_PATH@\"

servicedir=@LC_SERVICE_PATH@
service_PROGRAMS=kvks

noinst_HEADERS=\
  kvks_p.h \
  kvkscard_p.h \
  kvkscard.h

kvks_SOURCES=\
  kvks.c \
  kvkscard.c

kvks_LDADD=\
 -L../../lib/chipcard2-service -lchipcard2sv \
 -L../../lib/chipcard2-client -lchipcard2c

