INCLUDES=\
  -I$(top_srcdir)/src/lib/server/driver \
  -I$(top_builddir)/src/lib/server/driver \
  @all_includes@

EXTRA_DIST=SKEL1.xml.in

driverdir=@lc_server_driver_dir@
driver_PROGRAMS=SKEL1
driver_DATA=SKEL1.xml

noinst_HEADERS=\
  driverSKEL1_p.h \
  driverSKEL1.h \
  readerSKEL1_p.h \
  readerSKEL1.h

SKEL1_SOURCES=\
  driverSKEL1.c \
  readerSKEL1.c \
  main.c

SKEL1_LDADD=../../lib/server/driver/libchipcard3d.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
SKEL1_LDFLAGS=@STRIPALL@

