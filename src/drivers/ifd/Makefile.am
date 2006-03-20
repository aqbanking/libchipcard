INCLUDES=\
  -I$(top_srcdir)/src/lib/chipcard2-server/driver \
  -I$(top_builddir)/src/lib/chipcard2-server/driver \
  @all_includes@

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

ifd_LDADD=../../lib/chipcard2-server/driver/libchipcard2d.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifd_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

