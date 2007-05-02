SUBDIRS=common server driver

EXTRA_DIST=ressource.rc.in

noinst_LTLIBRARIES=libchipcard3i.la

libchipcard3i_la_SOURCES=dummy.c
libchipcard3i_la_LIBADD=common/libcommon.la ../sharedstuff/libsharedstuff.la @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
libchipcard3i_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
   @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@



