INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ifdold

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

