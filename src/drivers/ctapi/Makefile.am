INCLUDES=-I../../lib/chipcard2-server/driver @all_includes@

EXTRA_DIST=ctapi.xml.in

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ctapi
driver_DATA=ctapi.xml

noinst_HEADERS=\
  driverctapi_p.h \
  driverctapi.h \
  readerctapi_p.h \
  readerctapi.h

ctapi_SOURCES=\
  driverctapi.c \
  readerctapi.c \
  main.c

ctapi_LDADD=../../lib/chipcard2-server/driver/libchipcard2d.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ctapi_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

