SUBDIRS=devicemanager cardmanager clientmanager servicemanager slavemanager

INCLUDES=-I.. -I$(srcdir)/.. @all_includes@

typefiles=lockrequest.xml
EXTRA_DIST=$(typefiles) \
  cs_init.c \
  cs_callbacks.c \
  cs_tools.c \
  cs_work.c

DEFS+=\
 -DDEF_DRIVER_EXECDIR=\"@lc_server_driver_dir@\" \
 -DDEF_DRIVER_INFODIR=\"@lc_server_data_dir@/drivers\" \
 -DDEF_SERVICE_EXECDIR=\"@lc_server_service_dir@\" \
 -DDEF_SERVER_DATADIR=\"@lc_server_data_dir@\" \
 -DDEF_SERVER_LOGDIR=\"@lc_server_log_dir@\" \
 -DDEF_SERVER_NEWCERTDIR=\"@lc_server_def_newcert_dir@\" \
 -DDEF_SERVER_TRUSTEDCERTDIR=\"@lc_server_def_cert_dir@\" \
 -DLCS_DEFAULT_DHDIR=\"@lc_server_def_dh_dir@\"


noinst_LTLIBRARIES=libchipcards.la

noinst_HEADERS=\
  connection_l.h \
  connection_p.h \
  server_l.h \
  server_p.h \
  lockrequest_l.h \
  lockrequest_p.h \
  lockmanager_l.h \
  lockmanager_p.h

libchipcards_la_SOURCES=\
  connection.c \
  lockrequest.c \
  lockmanager.c \
  server.c

libchipcards_la_LIBADD=\
  devicemanager/libdevmgr.la \
  cardmanager/libcardmgr.la \
  clientmanager/libclientmgr.la \
  servicemanager/libservicemgr.la \
  slavemanager/libslavemgr.la

types: $(typefiles)
	$(gwenhywfar_dir)/bin/typemaker $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

