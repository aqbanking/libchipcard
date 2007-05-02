INCLUDES=-I.. -I../.. -I../../.. -I$(top_builddir) @all_includes@

typefiles=pininfo.xml

DEFS += \
  -DBUILDING_CHIPCARD

#AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=$(typefiles)

noinst_LTLIBRARIES=libsharedstuff.la

iheaderdir=${includedir}/chipcard/sharedstuff
iheader_HEADERS=\
  msgengine.h \
  misc.h \
  pininfo.h

noinst_HEADERS=\
  msgengine_l.h \
  msgengine_p.h \
  pininfo_l.h \
  pininfo_p.h

libsharedstuff_la_SOURCES=\
  misc.c \
  msgengine.c \
  pininfo.c


types: $(typefiles)
	typemaker -d CHIPCARD_API $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

