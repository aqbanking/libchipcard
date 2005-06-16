DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../ -I../client

noinst_LTLIBRARIES=libmon.la
noinst_HEADERS=\
 client_p.h \
 driver_l.h \
 driver_p.h \
 monitor_l.h \
 monitor_p.h \
 reader_l.h \
 reader_p.h \
 server_l.h \
 server_p.h \
 service_l.h \
 service_p.h

iheaderdir=${includedir}/chipcard2-client/mon
iheader_HEADERS=\
 client.h \
 driver.h \
 monitor.h \
 reader.h \
 server.h \
 service.h

libmon_la_SOURCES=\
 mclient.c \
 mdriver.c \
 monitor.c \
 mreader.c \
 mserver.c \
 mservice.c


MAINTAINERCLEANFILES=Makefile.in

