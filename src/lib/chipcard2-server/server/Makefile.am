SUBDIRS=commands

INCLUDES=@all_includes@

AM_CFLAGS=\
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_LOWLEVELDRIVER_PATH=\"@LC_LOWLEVELDRIVER_PATH@\" \
  -DLC_SERVICE_PATH=\"@LC_SERVICE_PATH@\" \
  -DLC_DEFAULT_LOGDIR=\"@LC_DEFAULT_LOGDIR@\" \
  -DLC_DEFAULT_DHFILE=\"@LC_DEFAULT_DHFILE@\"\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2s_config_am@\"

noinst_LTLIBRARIES=libserver.la

noinst_HEADERS=\
  card_l.h \
  card_p.h \
  cardserver_l.h \
  cardserver_p.h \
  client_p.h \
  driver_p.h \
  reader_p.h \
  request_p.h \
  serverconn_l.h \
  serverconn_p.h \
  service_p.h \
  usbmonitor_p.h \
  usbttymonitor_p.h


iheaderdir=${includedir}/chipcard2-server/server
iheader_HEADERS=\
  card.h \
  cardserver.h \
  client.h \
  driver.h \
  reader.h \
  request.h \
  service.h \
  usbmonitor.h \
  usbttymonitor.h


libserver_la_SOURCES=\
  card.c \
  cardserver.c \
  client.c \
  driver.c \
  reader.c \
  request.c \
  serverconn.c \
  service.c \
  usbmonitor.c \
  usbttymonitor.c

libserver_la_LIBADD=\
  commands/libcommands.la

