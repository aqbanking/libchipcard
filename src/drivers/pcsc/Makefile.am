INCLUDES=@all_includes@

AM_CFLAGS=-DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\"

driverdir=@LC_DEVICEDRIVER_PATH@
driver_PROGRAMS=pcsc

pcsc_SOURCES=main.c

pcsc_LDADD=-L../../lib/chipcard2-server -lchipcard2d

