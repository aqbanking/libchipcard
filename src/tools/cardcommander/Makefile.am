bin_PROGRAMS=cardcommander

AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

cardcommander_SOURCES = cardcommander.cpp
cardcommander_LDADD = ../../libchipcard/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@ -lstdc++
cardcommander_LDFLAGS=@STRIPALL@

if IS_WINDOWS
cardcommander_LDFLAGS+=-mconsole
endif


sources:
	for f in $(cardcommander_SOURCES) $(EXTRA_DIST); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

