SUBDIRS=ddvcard starcoscard

INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

noinst_LTLIBRARIES=liblcc_ct.la
liblcc_ct_la_SOURCES=lcc_plugins.c
liblcc_ct_la_LIBADD=\
 ddvcard/libct_ddvcard.la \
 starcoscard/libct_starcoscard.la

iheaderdir=${includedir}/chipcard
iheader_HEADERS=lcc_plugins.h

