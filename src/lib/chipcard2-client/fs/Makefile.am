AM_CFLAGS=\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../ -I../client

noinst_LTLIBRARIES=libfs.la
noinst_HEADERS=\
 fsnode_l.h \
 fsnode_p.h \
 fsmodule_l.h \
 fsmodule_p.h \
 fsclient_l.h \
 fsclient_p.h \
 fsmem_p.h \
 fs_p.h


iheaderdir=${includedir}/chipcard2-client/fs
iheader_HEADERS=\
 fsnode.h \
 fsmodule.h \
 fsclient.h \
 fs.h

libfs_la_SOURCES=\
 fsnode.c \
 fsmodule.c \
 fsclient.c \
 fsmem.c \
 fs.c


