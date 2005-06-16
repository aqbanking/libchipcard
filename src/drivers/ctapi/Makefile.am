INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ctapi

noinst_HEADERS=\
  driverctapi_p.h \
  driverctapi.h \
  readerctapi_p.h \
  readerctapi.h

ctapi_SOURCES=\
  driverctapi.c \
  readerctapi.c \
  main.c

ctapi_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ctapi_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

