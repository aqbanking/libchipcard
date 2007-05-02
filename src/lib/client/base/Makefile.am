INCLUDES=-I.. -I$(srcdir)/.. -I$(top_srcdir) @gwenhywfar_includes@

noinst_LTLIBRARIES=libbase.la

EXTRA_DIST=\
  card_iso.c \
  client_cmd.c \
  client_xml.c


DEFS += \
  -DBUILDING_CHIPCARD \
  -DLC_CLIENT_CONFIG_DIR=\"@lc_client_config_dir@\" \
  -DLC_CLIENT_CONFIG_FILE=\"@lc_client_config_file@\" \
  -DLC_CLIENT_XML_DIR=\"@lc_client_xml_dir@\"

AM_CFLAGS=$(visibility_cflags)

iheaderdir=${includedir}/chipcard/client
iheader_HEADERS=\
 client.h client_imp.h \
 card.h card_imp.h card_l.h \
 notifications.h \
 tlv.h

noinst_HEADERS=\
 client_l.h client_p.h \
 card_p.h \
 notifications_l.h \
 notifications_p.h \
 tlv_p.h

libbase_la_SOURCES=\
 client.c \
 card.c \
 notifications.c \
 tlv.c



