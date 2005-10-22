INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=pcsc

pcsc_SOURCES=main.c

pcsc_LDADD=../../lib/chipcard2-server/driver/libchipcard2d.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
pcsc_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

