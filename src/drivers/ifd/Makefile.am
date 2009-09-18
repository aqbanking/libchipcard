INCLUDES=\
  -I$(top_srcdir)/src/lib/server/driver \
  -I$(top_builddir)/src/lib/server/driver \
  @all_includes@

EXTRA_DIST=ifd.xml.in

driverdir=@lc_server_driver_dir@
driver_PROGRAMS=ifd
driver_DATA=ifd.xml

noinst_HEADERS=\
  driverifd_p.h \
  driverifd.h \
  readerifd_p.h \
  readerifd_l.h

ifd_SOURCES=\
  driverifd.c \
  readerifd.c \
  main.c


ifd_LDADD= ../../lib/server/driver/libchipcardd.la \
  @gwenhywfar_libs@
ifd_LDFLAGS=@STRIPALL@ -export-dynamic

