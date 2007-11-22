INCLUDES=-I.. -I../.. -I../../.. -I$(top_builddir) @all_includes@

typefiles=pininfo.xml

DEFS += \
  -DBUILDING_CHIPCARD \
  -DLC_LOWLEVELDRIVER_PATH=\"@lc_server_lowlevel_driver_dir@\"

#AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=$(typefiles)

noinst_LTLIBRARIES=libsharedstuff.la

iheaderdir=${includedir}/chipcard/sharedstuff
iheader_HEADERS=\
  driverinfo.h \
  msgengine.h \
  misc.h \
  pininfo.h

noinst_HEADERS=\
  driverinfo_p.h \
  msgengine_l.h \
  msgengine_p.h \
  pininfo_l.h \
  pininfo_p.h

libsharedstuff_la_SOURCES=\
  driverinfo.c \
  misc.c \
  msgengine.c \
  pininfo.c


types: $(typefiles)
	$(gwenhywfar_dir)/bin/typemaker -d CHIPCARD_API $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

