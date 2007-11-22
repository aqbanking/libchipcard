bin_PROGRAMS=cardcommander

INCLUDES=@all_includes@

cardcommander_SOURCES = cardcommander.cpp
cardcommander_LDADD = ../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
cardcommander_LDFLAGS=@STRIPALL@

if IS_WINDOWS
cardcommander_LDFLAGS+=-mconsole
endif
