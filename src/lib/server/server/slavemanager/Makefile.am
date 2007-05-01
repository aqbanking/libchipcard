INCLUDES= \
  -I.. -I../.. \
  -I$(srcdir)/.. -I$(srcdir)/../.. \
  @all_includes@

DEFS+=\
 -DDEF_SERVER_DATADIR=\"@lc_server_data_dir@\" \
 -DDEF_SERVER_LOGDIR=\"@lc_server_log_dir@\" \
 -DDEF_SERVER_NEWCERTDIR=\"@lc_server_def_newcert_dir@\" \
 -DDEF_SERVER_TRUSTEDCERTDIR=\"@lc_server_def_cert_dir@\"

EXTRA_DIST=\
  slr_startreader.c \
  slr_stopreader.c \
  slr_command.c \
  slr_reset.c \
  slr_suspendcheck.c \
  slr_resumecheck.c

noinst_LTLIBRARIES=libslavemgr.la

noinst_HEADERS=\
  sl_reader_l.h \
  sl_reader_p.h \
  sl_request_l.h \
  sl_request_p.h \
  slavemanager_l.h \
  slavemanager_p.h

libslavemgr_la_SOURCES=\
  sl_reader.c \
  sl_request.c \
  slavemanager.c


