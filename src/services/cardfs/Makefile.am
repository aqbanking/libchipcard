INCLUDES=@all_includes@

AM_CFLAGS=-DLC_SERVICE_PATH=\"@LC_SERVICE_PATH@\"

servicedir=@LC_SERVICE_PATH@
service_PROGRAMS=cardfs

noinst_HEADERS=\
  cardfs_p.h

cardfs_SOURCES=\
  cardfs.c

cardfs_LDADD=\
 -L../../lib/chipcard2-server -lchipcard2sv \
 -L../../lib/chipcard2-client -lchipcard2c

