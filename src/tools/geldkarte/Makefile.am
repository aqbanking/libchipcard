bin_PROGRAMS=geldkarte

AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

noinst_HEADERS=global.h

geldkarte_SOURCES=\
  main.c
geldkarte_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)
geldkarte_LDFLAGS=@STRIPALL@

if IS_WINDOWS
geldkarte_LDFLAGS+=-mconsole
endif

sources:
	for f in $(geldkarte_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
