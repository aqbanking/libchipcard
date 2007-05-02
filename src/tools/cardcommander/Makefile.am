bin_PROGRAMS=cardcommander

INCLUDES=@all_includes@

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

cardcommander_SOURCES = cardcommander.cpp
cardcommander_LDADD = ../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
cardcommander_LDFLAGS=@STRIPALL@

