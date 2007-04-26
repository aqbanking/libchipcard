INCLUDES= \
  -I$(top_srcdir)/src/lib/chipcard2-server \
  -I$(top_srcdir)/src/lib/chipcard2-server/server2 \
  -I$(top_srcdir)/src/lib/chipcard2-server/server2/devicemanager \
  -I$(top_srcdir)/src/lib/chipcard2-server/common \
  @all_includes@

CLIENT_LIB=-lchipcard2c

noinst_PROGRAMS=lstest lctest

noinst_HEADERS=cbtest.h cbtest_p.h

lstest_SOURCES=lstest.c cbtest.c
lstest_LDADD=\
  ../lib/chipcard2-server/server2/libchipcard2s.la \
  ../lib/chipcard2-server/common/libcommon.la  \
  ../lib/sharedstuff/libsharedstuff.la @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@

lctest_SOURCES=lctest.c cbtest.c
lctest_LDADD=-L../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

MAINTAINERCLEANFILES=Makefile.in

