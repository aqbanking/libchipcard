
noinst_PROGRAMS=lstest lctest

noinst_HEADERS=cbtest.h cbtest_p.h

lstest_SOURCES=lstest.c cbtest.c
lstest_LDADD=-L../lib/chipcard2-server -lchipcard2s @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@

lctest_SOURCES=lctest.c cbtest.c
lctest_LDADD=-L../lib/chipcard2-client -lchipcard2c
