SUBDIRS=lowlevel apps

AM_CFLAGS=\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I..

noinst_LTLIBRARIES=libclient.la

noinst_HEADERS=\
  card_l.h \
  card_p.h \
  client_p.h \
  msgengine_l.h \
  msgengine_p.h \
  notifications_l.h \
  notifications_p.h \
  tlv_p.h

iheaderdir=${includedir}/chipcard2-client/client
iheader_HEADERS=\
  card.h \
  client.h \
  msgengine.h \
  notifications.h \
  tlv.h

libclient_la_SOURCES=\
  card.c \
  client.c \
  msgengine.c \
  notifications.c \
  tlv.c

libclient_la_LIBADD=\
  lowlevel/liblowlevel.la \
  apps/libapps.la

