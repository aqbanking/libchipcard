INCLUDES=@all_includes@

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=ifd ifdold

noinst_HEADERS=\
  driverifd_p.h \
  driverifd.h

ifd_SOURCES=\
  driverifd.c \
  main.c

ifd_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifd_LDFLAGS=@STRIPALL@

ifdold_SOURCES=\
  driverifd-old.c \
  main.c

ifdold_LDADD=-L../../lib/chipcard2-server -lchipcard2d \
  @gwenhywfar_libs@ @libusb_libs@ @libsysfs_libs@
ifdold_LDFLAGS=@STRIPALL@


