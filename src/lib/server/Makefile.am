SUBDIRS=common server driver

EXTRA_DIST=ressource.rc.in

noinst_LTLIBRARIES=libchipcardi.la

libchipcardi_la_SOURCES=dummy.c
libchipcardi_la_LIBADD=common/libcommon.la ../sharedstuff/libsharedstuff.la @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
libchipcardi_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
   @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@



