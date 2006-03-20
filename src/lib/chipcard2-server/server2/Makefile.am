SUBDIRS=devicemanager cardmanager clientmanager commandmanager servicemanager

INCLUDES=-I.. -I$(srcdir)/.. @all_includes@

typefiles=lockrequest.xml
EXTRA_DIST=$(typefiles)

DEFS+=\
 -DDEF_DRIVER_EXECDIR=\"@LC_DEVICEDRIVER_PATH@\" \
 -DDEF_DRIVER_INFODIR=\"@libchipcard2s_config_am@/drivers\" \
 -DDEF_SERVICE_EXECDIR=\"@LC_SERVICE_PATH@\" \
 -DDEF_SERVER_DATADIR=\"@libchipcard2s_config_am@\" \
 -DDEF_SERVER_LOGDIR=\"@LC_DEFAULT_LOGDIR@\" \
 -DDEF_SERVER_NEWCERTDIR=\"@LC_DEFAULT_NEWCERTDIR@\" \
 -DDEF_SERVER_TRUSTEDCERTDIR=\"@libchipcard2d_config_am@/certificates/valid\"


noinst_LTLIBRARIES=libchipcard2s.la

noinst_HEADERS=\
  connection_l.h \
  connection_p.h \
  server_l.h \
  server_p.h \
  fullserver_l.h \
  fullserver_p.h \
  lockrequest_l.h \
  lockrequest_p.h \
  lockmanager_l.h \
  lockmanager_p.h

libchipcard2s_la_SOURCES=\
  connection.c \
  lockrequest.c \
  lockmanager.c \
  cs_init.c \
  cs_callbacks.c \
  cs_tools.c \
  cs_work.c \
  fullserver.c

libchipcard2s_la_LIBADD=\
  devicemanager/libdevmgr.la \
  cardmanager/libcardmgr.la \
  clientmanager/libclientmgr.la \
  commandmanager/libcommandmgr.la \
  servicemanager/libservicemgr.la 

types: $(typefiles)
	typemaker $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

