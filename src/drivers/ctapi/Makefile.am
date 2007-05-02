INCLUDES=\
  -I$(top_srcdir)/src/lib/server/driver \
  -I$(top_builddir)/src/lib/server/driver \
  @all_includes@

EXTRA_DIST=ctapi.xml.in

driverdir=@lc_server_driver_dir@
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

ctapi_LDADD=../../lib/server/driver/libchipcardd.la \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ctapi_LDFLAGS=@STRIPALL@

