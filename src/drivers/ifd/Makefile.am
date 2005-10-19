INCLUDES=@all_includes@

EXTRA_DIST=ifd.xml.in

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ifd
driver_DATA=ifd.xml

noinst_HEADERS=\
  driverifd_p.h \
  driverifd.h

ifd_SOURCES=\
  driverifd.c \
  main.c

ifd_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifd_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

