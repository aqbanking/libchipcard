
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

noinst_LTLIBRARIES=liblcct.la

iheaderdir=@libchipcard_headerdir_am@/ct
iheader_HEADERS=\
  ct_card.h

liblcct_la_SOURCES=\
  ct_card.c

sources:
	for f in $(liblcct_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
