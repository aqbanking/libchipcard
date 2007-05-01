INCLUDES=@all_includes@

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

noinst_LTLIBRARIES=libservice.la

noinst_HEADERS=\
  serviceclient_p.h \
  service_p.h

iheaderdir=${includedir}/chipcard3/client/service
iheader_HEADERS=\
  serviceclient.h \
  service.h

libservice_la_SOURCES=\
  serviceclient.c \
  service.c

