# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([chipcard3-config.in.in])
AC_CONFIG_HEADER([config.h])



###-------------------------------------------------------------------------
#
# versions
#

CHIPCARD_VERSION_MAJOR=3
CHIPCARD_VERSION_MINOR=0
CHIPCARD_VERSION_PATCHLEVEL=4
CHIPCARD_VERSION_BUILD=0
CHIPCARD_VERSION_TAG="stable"

CHIPCARD_VERSION_FULL_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.$CHIPCARD_VERSION_PATCHLEVEL.$CHIPCARD_VERSION_BUILD${CHIPCARD_VERSION_TAG}"
CHIPCARD_VERSION_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.$CHIPCARD_VERSION_PATCHLEVEL"

#
# SO version
#
LIBCHIPCARD_SO_CURRENT=1
LIBCHIPCARD_SO_AGE=0
LIBCHIPCARD_SO_REVISION=3
LIBCHIPCARD_SO_EFFECTIVE="`echo \$(($LIBCHIPCARD_SO_CURRENT-$LIBCHIPCARD_SO_AGE))`"

#
# Create release string
#

case "$CHIPCARD_VERSION_TAG" in
  svn|cvs)
     rev=`LC_ALL="C" $SVN info|grep 'Last Changed Rev'|cut -d' ' -f4`
     if test "x$rev" != "x" ; then
       CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.${CHIPCARD_VERSION_PATCHLEVEL}${CHIPCARD_VERSION_TAG}-r${rev}"
     else
       CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_FULL_STRING"
     fi
    ;;
  stable)
    CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_STRING"
    ;;
  *)
    CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.${CHIPCARD_VERSION_PATCHLEVEL}${CHIPCARD_VERSION_TAG}"
    ;;
esac



AC_SUBST(CHIPCARD_VERSION_MAJOR)
AC_SUBST(CHIPCARD_VERSION_MINOR)
AC_SUBST(CHIPCARD_VERSION_PATCHLEVEL)
AC_SUBST(CHIPCARD_VERSION_BUILD)
AC_SUBST(CHIPCARD_VERSION_TAG)
AC_SUBST(CHIPCARD_VERSION_FULL_STRING)
AC_SUBST(CHIPCARD_VERSION_STRING)
AC_SUBST(CHIPCARD_VERSION_RELEASE_STRING)

AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_MAJOR,$CHIPCARD_VERSION_MAJOR, 
                   [major version])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_MINOR,$CHIPCARD_VERSION_MINOR, 
                   [minor version])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_PATCHLEVEL,$CHIPCARD_VERSION_PATCHLEVEL, 
                   [patchlevel])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_BUILD,$CHIPCARD_VERSION_BUILD, [build])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_STRING,"$CHIPCARD_VERSION_STRING", 
                   [version string])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_FULL_STRING,"$CHIPCARD_VERSION_FULL_STRING",
                   [full version string])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_RELEASE_STRING,"$CHIPCARD_VERSION_RELEASE_STRING", 
                   [release string])
AC_DEFINE_UNQUOTED(CHIPCARD_VERSION_TAG,$CHIPCARD_VERSION_TAG, [tag])

AC_SUBST(LIBCHIPCARD_SO_CURRENT)
AC_SUBST(LIBCHIPCARD_SO_REVISION)
AC_SUBST(LIBCHIPCARD_SO_AGE)
AC_SUBST(LIBCHIPCARD_SO_EFFECTIVE)


AM_INIT_AUTOMAKE(libchipcard3, $CHIPCARD_VERSION_RELEASE_STRING)



###-------------------------------------------------------------------------
# check for OS
#

AQ_CHECK_OS



###-------------------------------------------------------------------------
# prefix handling
#

AC_MSG_CHECKING(for installation prefix)
AC_PREFIX_DEFAULT(/usr)
if test "x$prefix" = "xNONE"; then
  prefix=$ac_default_prefix
  ac_configure_args="$ac_configure_args --prefix $prefix"
fi
AC_MSG_RESULT($prefix)
AC_SUBST(prefix)



###-------------------------------------------------------------------------
#
# static/shared handling
#
AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)



###-------------------------------------------------------------------------
# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_RC
AC_PROG_LIBTOOL
AC_CHECK_PROG(SED,sed,sed)



###-------------------------------------------------------------------------
# Checks for libraries.



###-------------------------------------------------------------------------
# Checks for header files.

AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([dlfcn.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/wait.h])
AC_CHECK_HEADERS([sys/time.h])



###-------------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST



###-------------------------------------------------------------------------
# Checks for library functions.

AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(strcasecmp strncasecmp)
AC_CHECK_FUNCS(fork)
AC_CHECK_FUNCS(time)
AC_CHECK_FUNCS(getpid)
AC_CHECK_FUNCS(sigaction)
AC_CHECK_FUNCS(random)
AC_CHECK_FUNCS(wait)
AC_CHECK_FUNCS(chmod)
AC_CHECK_FUNCS(symlink)



# ---------------------------------------------------------------------------
# check for endianness
#

AC_C_BIGENDIAN([aq_endian="big"], [aq_endian="little"])
if test "x$aq_endian" == "xlittle"; then
 AC_DEFINE(LC_ENDIAN_LITTLE, 1, [if system is little endian])
elif test "x$aq_endian" == "xbig"; then
 AC_DEFINE(LC_ENDIAN_BIG, 1, [if system is big endian])
else
  AC_MSG_ERROR([Unknown endian type $aq_endian])
fi



###-------------------------------------------------------------------------
# check for Distribution

AQ_DISTRIBUTION



# ---------------------------------------------------------------------------
# check for libusb
#

AC_LIBUSB
AC_MSG_CHECKING(whether libUSB might be used)
if test "$have_libusb" != "yes" ; then
    AC_MSG_WARN([
*** LibUSB not found, autoconfiguration will not be available.
*** If you are using USB devices under Linux, FreeBSD, NetBSD, OpenBSD or
*** Darwin you really should consider installing libusb.
*** You can get it from http://libusb.sourceforge.net.])
else
    all_includes="$all_includes $libusb_includes"
    all_libraries="$all_libraries $libusb_libs"
    AC_DEFINE(USE_LIBUSB, 1, [whether libUSB is to be used])
    AC_MSG_RESULT(yes)
fi



# ---------------------------------------------------------------------------
# check for libsysfs
#

AC_LIBSYSFS
if test "$have_libsysfs" == "yes" ; then
    all_includes="$all_includes $libsysfs_includes"
    all_libraries="$all_libraries $libsysfs_libs"
    AC_DEFINE(USE_LIBSYSFS, 1, [whether libsysfs is to be used])
else
    AC_MSG_WARN([
*** LibSysFS not found, autoconfiguration of PCMCIA and UsbTTY devices will 
*** not be available.
*** If you are using those devices under Linux you really should consider 
*** installing libSysFS.
*** You can get it from http://linux-diag.sourceforge.net/Sysfsutils.html.])
fi



# ---------------------------------------------------------------------------
# check for OpenSC
#

#PKG_CHECK_MODULES([OPENSC], [libopensc >= 0.9.4], [
#    all_includes="$all_includes $opensc_includes"
#    AC_DEFINE(USE_OPENSC, 1, [whether OpenSC is to be used])
#    have_opensc=yes
#    ], [
#    AC_MSG_WARN(OpenSC not found, OpenSC driver will not be available.)
#    have_opensc=no
#])
#if test "$have_opensc" = "yes"; then
#  opensc_version="`pkg-config --modversion libopensc`"
#  case "$opensc_version" in
#    0.9.*|0.10.*)
#      ;;
#    *)
#      AC_DEFINE(LCC_OPENSC11, 1, [OpenSC >=0.11.x ])
#      ;;
#  esac
#fi
#AC_SUBST(opensc_version)
#
#AM_CONDITIONAL(USE_OPENSC, [test "$have_opensc" = "yes"])



###-------------------------------------------------------------------------
# check for Gwenhywfar
#

AC_GWENHYWFAR(2,3,0,0)

all_includes="$all_includes $gwenhywfar_includes"


###-------------------------------------------------------------------------
# OS dependent settings
#

case "$OS_TYPE" in
    posix)
        lc_sys_is_windows=0
        
        lc_client_config_dir="\${sysconfdir}/chipcard3/client"
        lc_client_config_file="chipcardc3.conf"
        lc_client_def_cert_dir="\${sysconfdir}/chipcard3/client/certs"
        lc_client_def_newcert_dir="~/.libchipcard3/certs"
        lc_client_def_cert_file="~/.libchipcard3/user.cert"
        lc_client_xml_dir="\${datadir}/chipcard3/client"

        lc_server_config_dir="\${sysconfdir}/chipcard3/server"
        lc_server_data_dir="\${datadir}/chipcard3/server"
        lc_server_lowlevel_driver_dir="\${libdir}/chipcard3/server/lowlevel"
        lc_server_def_cert_dir="\${sysconfdir}/chipcard3/server/certs"
        lc_server_def_newcert_dir="\${localstatedir}/lib/chipcardd3/newcerts"
        lc_server_driver_dir="\${libdir}/chipcard3/server/drivers"
        lc_server_service_dir="\${libdir}/chipcard3/server/services"
        lc_server_log_dir="\${localstatedir}/log/chipcard3"
        lc_server_pid_dir="\${localstatedir}/run"
        lc_server_def_dh_dir="\${sysconfdir}/chipcard3/server/dh"

        libchipcard3_includes="-I\${prefix}/include"
        libchipcard3c_libs="-L\${libdir} -lchipcard3c"
        libchipcard3i_libs="-L\${libdir} -lchipcard3i"
        
        lc_dir_nonwindows_src="drivers services"
        lc_dir_nonwindows_tools="chipcardd"
        
    	;;
    windows)
        AC_DEFINE_UNQUOTED(BUILDING_LIBCHIPCARD2_DLL,1, [Define if DLL is built])
        ACX_WINDOWS_PATHS
        lc_sys_is_windows=1

        lc_client_config_dir="\${sysconfdir}/chipcard3/client"
        lc_client_config_file="chipcardc3.conf"
        lc_client_def_cert_dir="\${sysconfdir}/chipcard3/client/certs"
        lc_client_def_newcert_dir="~/.libchipcard3/certs"
        lc_client_def_cert_file="~/.libchipcard3/user.cert"
        lc_client_xml_dir="\${datadir}/chipcard3/client"

        lc_server_config_dir="\${sysconfdir}/chipcard3/server"
        lc_server_data_dir="\${datadir}/chipcard3/server"
        lc_server_lowlevel_driver_dir="\${libdir}/chipcard3/server/lowlevel"
        lc_server_def_cert_dir="\${sysconfdir}/chipcard3/server/certs"
        lc_server_def_newcert_dir="\${localstatedir}/lib/chipcardd3/newcerts"
        lc_server_driver_dir="\${libdir}/chipcard3/server/drivers"
        lc_server_service_dir="\${libdir}/chipcard3/server/services"
        lc_server_log_dir="\${localstatedir}/log/chipcard3"
        lc_server_pid_dir="\${localstatedir}/run"
        lc_server_def_dh_dir="\${sysconfdir}/chipcard3/server/dh"

        libchipcard3_includes="-I\${prefix}/include"
        libchipcard3c_libs="-L\${libdir} -lchipcard3c"
        libchipcard3i_libs="-L\${libdir} -lchipcard3i"
    	;;
esac

AM_CONDITIONAL(IS_WINDOWS, [test "$OS_TYPE" = "windows"])
AC_SUBST(lc_sys_is_windows)

AC_SUBST(lc_client_config_dir)
AC_SUBST(lc_client_config_file)
AC_SUBST(lc_client_def_cert_dir)
AC_SUBST(lc_client_def_newcert_dir)
AC_SUBST(lc_client_def_cert_file)
AC_SUBST(lc_client_xml_dir)

AC_SUBST(lc_server_config_dir)
AC_SUBST(lc_server_data_dir)
AC_SUBST(lc_server_lowlevel_driver_dir)
AC_SUBST(lc_server_def_cert_dir)
AC_SUBST(lc_server_def_newcert_dir)
AC_SUBST(lc_server_driver_dir)
AC_SUBST(lc_server_service_dir)

AC_SUBST(lc_server_log_dir)
AC_SUBST(lc_server_pid_dir)
AC_SUBST(lc_server_def_dh_dir)

AC_SUBST(libchipcard3d_libs)
AC_SUBST(libchipcard3c_libs)
AC_SUBST(libchipcard3_includes)

AC_SUBST(lc_dir_nonwindows_src)
AC_SUBST(lc_dir_nonwindows_tools)

AC_SUBST(all_includes)
AC_SUBST(all_libraries)



# ---------------------------------------------------------------------------
# check for PC/SC
#

if test "$OS_TYPE" != "windows"; then
  AC_CHECK_LIB(pcsclite, SCardDisconnect, [pcsc_libs="-lpcsclite"])
  AC_MSG_CHECKING(whether PC/SC is to be used)
  if test -z "$pcsc_libs"; then
      AC_MSG_WARN(No pcsc libraries found, SCard driver will not be available.)
      lc_pcsc_io_dir=""
      have_pcsc="no"
  else
      AC_DEFINE(HAVE_PCSC, 1, [whether PC/SC is to be used])
      lc_pcsc_io_dir="pcsc"
      have_pcsc="yes"
      AC_MSG_RESULT(yes)
  fi
else
  AC_MSG_CHECKING(whether PC/SC is to be used)
  AC_DEFINE(HAVE_PCSC, 1, [whether PC/SC is to be used])
  lc_pcsc_io_dir="pcsc"
  have_pcsc="yes"
  AC_MSG_RESULT([yes, using SCard])
fi
AM_CONDITIONAL(USE_PCSC, [test "$have_pcsc" = "yes"])
AC_SUBST(lc_pcsc_io_dir)
AC_SUBST(pcsc_libs)



###-------------------------------------------------------------------------
# check for release
#
AC_MSG_CHECKING(whether this is an official release)
AC_ARG_ENABLE(release,
  [  --enable-release         make this an official release (default=no)],
  [ case "${enableval}" in
    yes) enable_release="yes";;
    no)  enable_release="no";;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-release);;
    esac
  ],
  enable_release="no")

if test "$enable_release" = "yes"; then
  STRIPALL="-s"
else
  STRIPALL=""
fi
AC_SUBST(STRIPALL)
AC_MSG_RESULT($enable_release)

dnl check for the documentation output path
AQ_DOCPATH



###-------------------------------------------------------------------------
# check for doc type
#
AC_MSG_CHECKING(if full docu should be created)
AC_ARG_ENABLE(full-doc,
  [  --enable-full-doc         enable creating full apidoc (default=no)],
  [ case "${enableval}" in
    yes) enable_fulldoc="yes";;
    no)  enable_fulldoc="no";;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-full-doc);;
    esac
  ],
  enable_fulldoc="no")

if test "$enable_fulldoc" = "yes"; then
	DOXYGEN_INPUT="src/lib"
        DOXYGEN_DEFINE=""
else
	DOXYGEN_INPUT="chipcard3"
	DOXYGEN_DEFINE="DOXYGEN_HIDE"
fi
AC_SUBST(DOXYGEN_INPUT)
AC_SUBST(DOXYGEN_DEFINE)
AC_MSG_RESULT($enable_fulldoc)



###-------------------------------------------------------------------------
# GCC version (check for usability)
#

AC_MSG_CHECKING(if symbol visibility is supported)
visibility_cflags=""
case `basename $CC` in 
  gcc*)
     dnl Determine version number (watch out: the outside square
     dnl parenteses are vitally important here!)
     gccversion=["` ${CC} --version | head -1 | ${SED} 's/.*\([0-9]\.[0-9]\.[0-9]*\).*/\1/' `"]
     case $gccversion in
       4.*)
	 # Activate -fvisibility=hidden only for explicit 4.x gcc
	 # versions. We never now what future gcc's might do with
	 # that argument, and we also can't be sure that other
	 # gcc's happen to show very weird version
	 # numbers. Therefore leave the default at non-activated.
         visibility_supported="yes, gcc-${gccversion}"
         AC_DEFINE(GCC_WITH_VISIBILITY_ATTRIBUTE, 1, [visibility])
         visibility_cflags="-fvisibility=hidden"
         ;;
       *)
	 # Default: -fvisibility=hidden not activated.
         visibility_supported="no (needs gcc >=4.0, you have gcc-${gccversion})"
         ;;
      esac
      ;;
  *)
    visibility_supported="no (needs gcc >=4.0, you use $CC)"
    ;;
esac

# -fvisibility is at least not available on MinGW/gcc-3.4.4 (will
# give an "unrecognized cmdline option"). Also, unfortunately I
# don't know an easy way to ask the compiler here. Maybe
# http://autoconf-archive.cryp.to/ax_cflags_gcc_option.html
case "$OS_TYPE" in
  windows)
        visibility_supported="no (not yet on MinGW/Windows)"
        visibility_cflags=""
	;;
  *)
	;;
esac

AC_MSG_RESULT(${visibility_supported})
AC_SUBST(visibility_cflags)



###-------------------------------------------------------------------------
# search for tag files
#

AC_MSG_CHECKING(doxygen tag files)
DOXYGEN_TAGFILES=""
if test -d "${docpath}"; then
  DOXYGEN_TAGFILES="`cd ${docpath} && ls *.tag`"
  if test -n "${DOXYGEN_TAGFILES}"; then
   DOXYGEN_TAGFILES="`echo ${DOXYGEN_TAGFILES} | ${SED} -e s/${PACKAGE}.tag//`"
  fi
  realfiles=""
  for ff in ${DOXYGEN_TAGFILES}; do
    realfiles="${realfiles} ${docpath}/${ff}"
  done
  DOXYGEN_TAGFILES="${realfiles}"
fi
if test -z "${DOXYGEN_TAGFILES}"; then
  AC_MSG_RESULT(none)
else
  AC_MSG_RESULT(found)
fi
AC_SUBST(DOXYGEN_TAGFILES)



###-------------------------------------------------------------------------
# Search for gettext
#

AC_CHECK_HEADERS([libintl.h],[have_libintl_h=yes],[have_libintl_h=no])
LIBS=""
I18N_CATALOG=""
AC_CHECK_PROGS(XGETTEXT, xgettext, missing)
if test "$XGETTEXT" != "missing"; then
  AC_SEARCH_LIBS(gettext,intl c,[have_gettext_fn=yes], [have_gettext_fn=no])
  i18n_library=$LIBS
  LIBS=""
  AC_MSG_CHECKING(for working i18n environment)
  if test "$have_libintl_h" = "yes" -a "$have_gettext_fn" = "yes" -a "$XGETTEXT" != "missing"; then
    I18N_PATH="`eval echo $prefix/share`/locale"
    AC_DEFINE_UNQUOTED(I18N_PATH,"$I18N_PATH",[Path for locales])
    AC_DEFINE(HAVE_GETTEXT_ENVIRONMENT,1,[if gettext is available])
    I18N_CATALOG="catalog"
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
fi
AC_SUBST(I18N_CATALOG)



###-------------------------------------------------------------------------
# check for MEMTRACE
#

AC_MSG_CHECKING(if MEMTRACE is wanted)
AC_ARG_ENABLE(memtrace,
  [  --enable-memtrace             enable memory trace (default=no)],
  enable_memtrace="$enableval",
  enable_memtrace="no")
AC_MSG_RESULT($enable_memtrace)

if test "$enable_memtrace" != "no"; then
  AC_DEFINE(GWEN_MEMORY_DEBUG, 1, [if memory debugging is wanted])
fi



###-------------------------------------------------------------------------
# Create output files
#

AC_CONFIG_FILES([Makefile
                 chipcard3-config.in
                 Doxyfile
                 libchipcard3.spec
                 bin/Makefile
                 bin/prepare.sh.in
                 data/Makefile
                 data/drivers/Makefile
                 data/drivers/ccid_ifd/Makefile
                 data/drivers/cherry_ctapi/Makefile
                 data/drivers/cyberjack_ctapi/Makefile
                 data/drivers/kobil_ctapi/Makefile
                 data/drivers/omnikey_ifd/Makefile
                 data/drivers/omnikey_ifd/cm_2020_ifd/Makefile
                 data/drivers/omnikey_ifd/cm_4000_ifd/Makefile
                 data/drivers/orga_ctapi/Makefile
                 data/drivers/scmx32_ifd/Makefile
                 data/drivers/scm33x_ifd/Makefile
                 data/drivers/tow_ctapi/Makefile
                 data/drivers/o2micro_ctapi/Makefile
                 data/drivers/celectronic_ctapi/Makefile
                 doc/Makefile
                 etc/Makefile
                 src/Makefile
                 src/lib/Makefile
                 src/lib/version.h
                 src/lib/chipcard3.h
                 src/lib/sharedstuff/Makefile
                 src/lib/client/Makefile
                 src/lib/client/ressource.rc
                 src/lib/client/base/Makefile
                 src/lib/client/io/Makefile
                 src/lib/client/io/lcc/Makefile
                 src/lib/client/io/pcsc/Makefile
                 src/lib/client/cards/Makefile
                 src/lib/client/cards/ddvcard/Makefile
                 src/lib/client/cards/egkcard/Makefile
                 src/lib/client/cards/geldkarte/Makefile
                 src/lib/client/cards/memorycard/Makefile
                 src/lib/client/cards/kvkcard/Makefile
                 src/lib/client/cards/processorcard/Makefile
                 src/lib/client/cards/starcos/Makefile
                 src/lib/client/mon/Makefile
                 src/lib/client/crypttoken/Makefile
                 src/lib/client/service/Makefile
                 src/lib/server/Makefile
                 src/lib/server/ressource.rc
                 src/lib/server/common/Makefile
                 src/lib/server/driver/Makefile
                 src/lib/server/server/Makefile
                 src/lib/server/server/devicemanager/Makefile
                 src/lib/server/server/cardmanager/Makefile
                 src/lib/server/server/clientmanager/Makefile
                 src/lib/server/server/servicemanager/Makefile
                 src/lib/server/server/slavemanager/Makefile
                 src/crypttoken/Makefile
                 src/crypttoken/ddvcard/Makefile
                 src/crypttoken/ddvcard/ddvcard.xml
                 src/crypttoken/starcoscard/Makefile
                 src/crypttoken/starcoscard/starcoscard.xml
                 src/drivers/Makefile
                 src/drivers/ctapi/Makefile
                 src/drivers/ctapi/ctapi.xml
                 src/drivers/ifd/Makefile
                 src/drivers/ifd/ifd.xml
                 src/drivers/skeleton/Makefile
                 src/drivers/skeleton/SKEL1.xml
                 src/fake_ctapi/Makefile
                 src/fake_ctapi/ressource.rc
                 src/tools/Makefile
                 src/tools/chipcardd/Makefile
                 src/tools/geldkarte/Makefile
                 src/tools/memcard/Makefile
                 src/tools/chipcard-tool/Makefile
                 src/tools/kvkcard/Makefile
                 src/tools/cardcommander/Makefile
                 src/services/Makefile
                 src/services/kvks/Makefile
                 src/services/kvks/kvks.xml
                 tutorials/Makefile
                 m4/Makefile
                ])
AC_OUTPUT



###-------------------------------------------------------------------------
# create header directory for all, make symlinks
#

lprefix="${srcdir}"
lprefix=`( cd ${lprefix} ; pwd )`

rm -Rf chipcard3
mkdir chipcard3
AQ_HEADER_FILES="${lprefix}/src/lib/version.h ${lprefix}/src/lib/chipcard3.h"
ln -s "${lprefix}/src/lib/version.h" chipcard3/version.h
ln -s "${lprefix}/src/lib/chipcard3.h" chipcard3/chipcard3.h



###-------------------------------------------------------------------------
# create header directory for server, make symlinks
#

lprefix="${srcdir}"
lprefix=`( cd ${lprefix} ; pwd )`

# symlink all headers from src/lib/sharedstuff/*
mkdir chipcard3/sharedstuff
hfiles=`( cd "${srcdir}/src/lib/sharedstuff" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/sharedstuff/${f}"
  case ${f} in
    *_p.h)
          ;;
    *_l.h)
          ;;
    *)   
          ln -s "${lprefix}/src/lib//sharedstuff/${f}" "chipcard3/sharedstuff/${f}"
          ;;
  esac
done



###-------------------------------------------------------------------------
# create header directory for client, make symlinks
#

lc_modules="io cards crypttoken mon service"
#client cards mon fs crypttoken
lc_client_io_modules="lcc pcsc"

lc_client_card_modules="ddvcard egkcard geldkarte kvkcard starcos memorycard processorcard"


lprefix="${srcdir}"
lprefix=`( cd ${lprefix} ; pwd )`

rm -Rf chipcard3/client
mkdir chipcard3/client

# symlink all headers from src/lib/client
hfiles=`( cd "${srcdir}/src/lib/client" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/client/${f}"
  case ${f} in
    *_p.h | *version.h | *chipcard3.h)
              ;;
    *)
              ln -s "${lprefix}/src/lib/client/${f}" "chipcard3/client/${f}"
              ;;
  esac
done


# symlink all headers from src/lib/client/base
hfiles=`( cd "${srcdir}/src/lib/client/base" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/client/base/${f}"
  case ${f} in
    *_p.h | *_l.h)
      ;;
    *)   
      ln -s "${lprefix}/src/lib/client/base/${f}" "chipcard3/client/${f}"
      ;;
  esac
done


# symlink all headers from src/lib/client/*
for sm in $lc_modules; do
  mkdir chipcard3/client/${sm}
  hfiles=`( cd "${srcdir}/src/lib/client/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/client/${sm}/${f}"
    case ${f} in
      *_p.h | *_l.h)
        ;;
      *)   
        ln -s "${lprefix}/src/lib/client/${sm}/${f}" "chipcard3/client/${sm}/${f}"
        ;;
    esac
  done
done


# symlink all headers from src/lib/client/io/*
for sm in $lc_client_io_modules; do
  mkdir chipcard3/client/io/${sm}
  hfiles=`( cd "${srcdir}/src/lib/client/io/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/client/io/${sm}/${f}"
    case ${f} in
      *_p.h | *_l.h)
        ;;
      *)   
        ln -s "${lprefix}/src/lib/client/io/${sm}/${f}" "chipcard3/client/io/${sm}/${f}"
        ;;
    esac
  done
done


# symlink all headers from src/lib/client/cards/*
for sm in $lc_client_card_modules; do
  hfiles=`( cd "${srcdir}/src/lib/client/cards/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/client/cards/${f}"
    case ${f} in
      *_p.h | *_l.h)
        ;;
      *)   
        ln -s "${lprefix}/src/lib/client/cards/${sm}/${f}" "chipcard3/client/cards/${f}"
        ;;
    esac
  done
done


# symlink all headers from src/lib/server/driver/*
mkdir -p chipcard3/server/driver
hfiles=`( cd "${srcdir}/src/lib/server/driver" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/server/driver/${f}"
  case ${f} in
    *_p.h | *_l.h)
      ;;
    *)   
      ln -s "${lprefix}/src/lib/server/driver/${f}" "chipcard3/server/driver/${f}"
      ;;
  esac
done



###-------------------------------------------------------------------------
# Summary
#

echo
echo "Summary:"
echo "================================="
echo "Libchipcard version : $CHIPCARD_VERSION_FULL_STRING"
echo "Support for LibSysFS: $have_libsysfs"
echo "Support for LibUSB  : $have_libusb"
if test "$have_libsysfs" != "yes" -o "$have_libusb" != "yes"; then
  echo ""
  echo "HINT: You really should install libsysfs and libusb (including"
  echo "development packages), this will make Libchipcard much more usefull."
  echo "Otherwise autodetection of readers is not fully available."
fi
echo ""
echo "You can now build Libchipcard with 'make' (on *BSD use 'gmake')."


