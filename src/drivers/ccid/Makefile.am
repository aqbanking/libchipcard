INCLUDES=\
  -I$(top_srcdir)/src/lib/chipcard2-server/driver \
  -I$(top_builddir)/src/lib/chipcard2-server/driver \
  @all_includes@

EXTRA_DIST=ccid.xml.in

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ccid
driver_DATA=ccid.xml

noinst_HEADERS=\
  driverccid_p.h \
  driverccid.h \
  readerccid_p.h \
  readerccid_l.h

ccid_SOURCES=\
  driverccid.c \
  readerccid.c \
  main.c

ccid_LDADD=../../lib/chipcard2-server/driver/libchipcard2d.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ccid_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

