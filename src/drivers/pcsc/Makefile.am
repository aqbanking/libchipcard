INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=pcsc

pcsc_SOURCES=main.c

pcsc_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
pcsc_LDFLAGS=@STRIPALL@
