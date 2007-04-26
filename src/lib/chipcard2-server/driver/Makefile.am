INCLUDES=@all_includes@

lib_LTLIBRARIES=libchipcard2d.la

noinst_HEADERS=\
  driver_l.h \
  driver_p.h \
  reader_l.h \
  reader_p.h \
  slot_l.h \
  slot_p.h


libchipcard2d_la_SOURCES=\
  driver.c \
  reader.c \
  slot.c

libchipcard2d_la_LIBADD= \
  ../common/libcommon.la \
  ../../sharedstuff/libsharedstuff.la @gwenhywfar_libs@
libchipcard2d_la_LDFLAGS= \
  -no-undefined -Wl,--export-all-symbols \
  @STRIPALL@ -version-info \
   @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@


MAINTAINERCLEANFILES=Makefile.in

