INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ifd

noinst_HEADERS=\
  driverifd_p.h \
  driverifd.h

ifd_SOURCES=\
  driverifd.c \
  main.c

ifd_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifd_LDFLAGS=@STRIPALL@
