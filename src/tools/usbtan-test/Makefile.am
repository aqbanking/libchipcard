bin_PROGRAMS=usbtan-test

AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

usbtan_test_SOURCES=\
  main.c 
usbtan_test_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
usbtan_test_LDFLAGS=@STRIPALL@

if IS_WINDOWS
usbtan_test_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

