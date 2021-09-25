bin_PROGRAMS=cardcommander

AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

cardcommander_SOURCES = cardcommander.cpp
cardcommander_LDADD = ../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS) -lstdc++
cardcommander_LDFLAGS=@STRIPALL@

if IS_WINDOWS
cardcommander_LDFLAGS+=-mconsole
endif


sources:
	for f in $(cardcommander_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
