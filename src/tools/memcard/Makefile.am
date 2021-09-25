bin_PROGRAMS=memcard

AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

noinst_HEADERS=global.h

memcard_SOURCES=\
  main.c
memcard_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)
memcard_LDFLAGS=@STRIPALL@

if IS_WINDOWS
memcard_LDFLAGS+=-mconsole
endif

sources:
	for f in $(memcard_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
