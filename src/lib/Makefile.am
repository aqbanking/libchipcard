DIST_SUBDIRS=sharedstuff client server

SUBDIRS=sharedstuff client

if !IS_WINDOWS
SUBDIRS+=server
endif

EXTRA_DIST=chipcard3.h.in version.h.in

iheaderdir=${includedir}/chipcard3
iheader_HEADERS=\
  chipcard3.h \
  version.h


