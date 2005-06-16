INCLUDES=@all_includes@

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

noinst_PROGRAMS=lstest lctest

noinst_HEADERS=cbtest.h cbtest_p.h

lstest_SOURCES=lstest.c cbtest.c
lstest_LDADD=-L../lib/chipcard2-server -lchipcard2s @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@

lctest_SOURCES=lctest.c cbtest.c
lctest_LDADD=-L../lib/chipcard2-client $(CLIENT_LIB) @gwenhywfar_libs@

MAINTAINERCLEANFILES=Makefile.in

