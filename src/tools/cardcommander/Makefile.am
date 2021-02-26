bin_PROGRAMS=cardcommander

AM_CPPFLAGS=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes)

cardcommander_SOURCES = cardcommander.cpp
cardcommander_LDADD = ../../lib/client/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@ -lstdc++
cardcommander_LDFLAGS=@STRIPALL@

if IS_WINDOWS
cardcommander_LDFLAGS+=-mconsole
endif
