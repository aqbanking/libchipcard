INCLUDES=@all_includes@

EXTRA_DIST=chipcard2.h.in version.h.in

lib_LTLIBRARIES=libchipcard2sv.la

noinst_HEADERS=\
  client_p.h \
  service_p.h


iheaderdir=${includedir}/chipcard2-service
iheader_HEADERS=\
  client.h \
  service.h

libchipcard2sv_la_SOURCES=\
  client.c \
  service.c

libchipcard2sv_la_LIBADD= \
  ../chipcard2-client/libchipcard2c.la \
  @gwenhywfar_libs@
