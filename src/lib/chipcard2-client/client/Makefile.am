SUBDIRS=lowlevel apps

DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I..


noinst_LTLIBRARIES=libclient.la

noinst_HEADERS=\
  card_l.h \
  card_p.h \
  client_l.h \
  client_p.h \
  notifications_l.h \
  notifications_p.h \
  tlv_p.h

iheaderdir=${includedir}/chipcard2-client/client
iheader_HEADERS=\
  card.h \
  client.h \
  notifications.h \
  tlv.h

libclient_la_SOURCES=\
  card.c \
  client.c \
  notifications.c \
  tlv.c

libclient_la_LIBADD=\
  lowlevel/liblowlevel.la \
  apps/libapps.la


MAINTAINERCLEANFILES=Makefile.in

