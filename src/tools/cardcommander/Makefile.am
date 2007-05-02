bin_PROGRAMS=cardcommander3

INCLUDES=@all_includes@

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

cardcommander3_SOURCES = cardcommander.cpp
cardcommander3_LDADD = ../../lib/client/libchipcard3c.la $(mingw_ldflags) @gwenhywfar_libs@
cardcommander3_LDFLAGS=@STRIPALL@

