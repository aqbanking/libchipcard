INCLUDES=@all_includes@

EXTRA_DIST=clientlcc_cmd.c clientlcc_reader.c clientlcc_notify.c

noinst_LTLIBRARIES=libio_lcc.la

DEFS += \
  -DBUILDING_CHIPCARD \
  -DLC_CLIENT_DEF_CERT_DIR=\"@lc_client_def_cert_dir@\" \
  -DLC_CLIENT_DEF_NEWCERT_DIR=\"@lc_client_def_newcert_dir@\" \
  -DLC_CLIENT_DEF_CERT_FILE=\"@lc_client_def_cert_file@\"

AM_CFLAGS=$(visibility_cflags)

noinst_HEADERS=\
  cardlcc_l.h cardlcc_p.h \
  clientlcc_l.h clientlcc_p.h \
  request_l.h request_p.h \
  server_l.h server_p.h

iheaderdir=${includedir}/chipcard/client/io/lcc
iheader_HEADERS=\
  clientlcc.h

libio_lcc_la_SOURCES=\
  cardlcc.c \
  clientlcc.c \
  request.c \
  server.c


