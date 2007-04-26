INCLUDES=@all_includes@

EXTRA_DIST=ressource.rc.in

lib_LTLIBRARIES=libchipcard2sv.la

noinst_HEADERS=\
  client_p.h \
  service_p.h


iheaderdir=${includedir}/chipcard2-service
iheader_HEADERS=\
  client.h \
  service.h

libchipcard2sv_la_SOURCES=\
  client.c \
  service.c

libchipcard2sv_la_LIBADD= \
  ../chipcard2-client/libchipcard2c.la \
  @gwenhywfar_libs@
libchipcard2sv_la_LDFLAGS= \
  -no-undefined -Wl,--export-all-symbols \
  @STRIPALL@ -version-info \
   @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@



# dll stuff
if IS_WINDOWS

# you only need to setup these variables here, the rest is done automatically
LIBRARY_NAME = chipcard2sv32_$(LIBCHIPCARD_SO_EFFECTIVE)
DLLLDLIBS = -L/mingw/lib -L../chipcard2-client -lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE) @gwenhywfar_libs@
DLLLDFLAGS = $(STRIPALL) $(LDFLAGS)


###------------------------------------------------------------------------
# You should not have to make any modifications below this point
#

DLLNAME = $(LIBRARY_NAME).dll
DLLEXP_LIB = $(LIBRARY_NAME).lib
DLLEXP_DEF = $(LIBRARY_NAME).def
DLLSRC_LIB = $(lib_LTLIBRARIES:.la=.a)
DLLSRC_LIBLA = $(lib_LTLIBRARIES)

#all-local: dll

#install-exec-local: dll-install

DLLTOOL=dlltool
DLLWRAP=dllwrap
WINDRES=windres


dll: $(DLLNAME)

dll-install: dll
	$(INSTALL) -D $(DLLNAME) $(DESTDIR)$(WIN_PATH_WINDOWS_MINGW)/$(DLLNAME)

ressource.o: ressource.rc
	$(WINDRES) -i $< -o $@


$(DLLNAME): $(DLLSRC_LIBLA) ressource.o
	mkdir -p dlldir && \
	( \
	  if test -r $(DLLSRC_LIB); then \
	    SRCLIB=$(DLLSRC_LIB); \
	  else \
	    SRCLIB=.libs/$(DLLSRC_LIB); \
	  fi; \
	  ( cd dlldir && ar x "../$${SRCLIB}" && \
	   ( ls ../.libs/*.o && cp ../.libs/*.o .) ) && \
	 $(DLLTOOL) -e exports.o *.o && \
	 $(DLLWRAP) \
	   --export-all \
	   --output-def $(DLLEXP_DEF) \
	   --implib $(DLLEXP_LIB) \
	   --driver-name $(CC) \
	   -o $(DLLNAME) \
	     dlldir/*.o exports.o \
	     ressource.o $(DLLLDFLAGS) $(DLLLDLIBS) \
	) && rm -rf dlldir

CLEANFILES = $(DLLNAME) $(DLLEXP_LIB) $(DLLEXP_DEF)

endif
# IS_WINDOWS

MAINTAINERCLEANFILES=Makefile.in

