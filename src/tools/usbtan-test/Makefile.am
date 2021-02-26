bin_PROGRAMS=usbtan-test

AM_CPPFLAGS=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes)

usbtan_test_SOURCES=\
  main.c 
usbtan_test_LDADD=../../lib/client/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
usbtan_test_LDFLAGS=@STRIPALL@

if IS_WINDOWS
usbtan_test_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

