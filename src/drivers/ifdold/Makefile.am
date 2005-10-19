INCLUDES=@all_includes@

EXTRA_DIST=ifdold.xml.in

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ifdold
driver_DATA=ifdold.xml

noinst_HEADERS=\
  driverifdold_p.h \
  driverifdold.h

ifdold_SOURCES=\
  driverifdold.c \
  main.c

ifdold_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifdold_LDFLAGS=@STRIPALL@


MAINTAINERCLEANFILES=Makefile.in

