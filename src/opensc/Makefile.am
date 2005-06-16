MAINTAINERCLEANFILES=Makefile.in

INCLUDES=@all_includes@

DEFS=@DEFS@ -DOPENSC_VERSION=\"@opensc_version@\"

if IS_WINDOWS
  CLIENT_LIB=-lchipcard2c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard2c
endif

lib_LTLIBRARIES=reader-libchipcard2.la

noinst_HEADERS=reader-libchipcard2_p.h
reader_libchipcard2_la_SOURCES=reader-libchipcard2.c
reader_libchipcard2_la_LIBADD=-L../lib/chipcard2-client $(CLIENT_LIB) $(OPENSC_LIBS) @gwenhywfar_libs@
reader_libchipcard2_la_LDFLAGS=\
  -module @STRIPALL@ -version-info \
   @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@

# dll stuff
if IS_WINDOWS

# you only need to setup these variables here, the rest is done automatically
LIBRARY_NAME = reader-libchipcard2
DLLLDLIBS = -L../lib/chipcard2-client $(CLIENT_LIB) -L/mingw/lib  $(OPENSC_LIBS) @gwenhywfar_libs@
DLLLDFLAGS = $(STRIPALL) $(LDFLAGS)


###------------------------------------------------------------------------
# You should not have to make any modifications below this point
#

DLLNAME = $(LIBRARY_NAME).dll
DLLEXP_LIB = $(LIBRARY_NAME).lib
DLLEXP_DEF = $(LIBRARY_NAME).def
DLLSRC_LIB = $(lib_LTLIBRARIES:.la=.a)
DLLSRC_LIBLA = $(lib_LTLIBRARIES)

all-local: dll

install-exec-local: dll-install

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
