DIST_SUBDIRS=sharedstuff client server

SUBDIRS=sharedstuff client

if !IS_WINDOWS
SUBDIRS+=server
endif

EXTRA_DIST=chipcard.h.in version.h.in

iheaderdir=${includedir}/chipcard
iheader_HEADERS=\
  chipcard.h \
  version.h


