AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=README examplecard.xml tutorials.h

CLIENT_LIB=../src/lib/client/libchipcard.la

noinst_PROGRAMS=\
 tutorial1a tutorial1b tutorial1c \
 tutorial2 \
 tutorial3

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif


appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=examplecard.xml

noinst_HEADERS=examplecard.h examplecard_p.h

tutorial1a_SOURCES=tutorial1a.c
tutorial1a_LDADD=../src/libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)

tutorial1b_SOURCES=tutorial1b.c
tutorial1b_LDADD=../src/libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)

tutorial1c_SOURCES=tutorial1c.c
tutorial1c_LDADD=../src/libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)

tutorial2_SOURCES=tutorial2.c
tutorial2_LDADD=../src/libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)

tutorial3_SOURCES=tutorial3.c examplecard.c
tutorial3_LDADD=../src/libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)


typefiles:

typedefs:


