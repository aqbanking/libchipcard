# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT
AC_CANONICAL_TARGET([])
AC_CONFIG_SRCDIR([chipcard2-server-config.in.in])
AC_CONFIG_HEADER([config.h])


###-------------------------------------------------------------------------
#
# versions
#
CHIPCARD_VERSION_MAJOR=1
CHIPCARD_VERSION_MINOR=9
CHIPCARD_VERSION_PATCHLEVEL=7
CHIPCARD_VERSION_BUILD=4
CHIPCARD_VERSION_TAG="cvs"
CHIPCARD_VERSION_FULL_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.$CHIPCARD_VERSION_PATCHLEVEL.$CHIPCARD_VERSION_BUILD${CHIPCARD_VERSION_TAG}"
CHIPCARD_VERSION_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR.$CHIPCARD_VERSION_PATCHLEVEL${CHIPCARD_VERSION_TAG}"

#
# SO version
#
LIBCHIPCARD_SO_CURRENT=0
LIBCHIPCARD_SO_REVISION=0
LIBCHIPCARD_SO_AGE=0

#
# Create release string
#
case "$CHIPCARD_VERSION_TAG" in
  cvs)
     CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_FULL_STRING"
     ;;
  stable)
     CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR"
     # should we add patchlevel ?
     if test "$CHIPCARD_VERSION_PATCHLEVEL" != "0"; then
       CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_RELEASE_STRING.$CHIPCARD_VERSION_PATCHLEVEL"
     fi
     ;;
  *)
     CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_MAJOR.$CHIPCARD_VERSION_MINOR"
     # should we add patchlevel ?
     if test "$CHIPCARD_VERSION_PATCHLEVEL" != "0"; then
       CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_RELEASE_STRING.$CHIPCARD_VERSION_PATCHLEVEL"
     fi
     
     # add TAG
     CHIPCARD_VERSION_RELEASE_STRING="$CHIPCARD_VERSION_RELEASE_STRING${CHIPCARD_VERSION_TAG}"
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


AM_INIT_AUTOMAKE(libchipcard2, $CHIPCARD_VERSION_RELEASE_STRING)


###-------------------------------------------------------------------------
dnl prefix handling
AC_PREFIX_DEFAULT(/usr)
if test "x$prefix" = "xNONE"; then
  prefix=$ac_default_prefix
  ac_configure_args="$ac_configure_args --prefix $prefix"
fi
AC_SUBST(prefix)


###-------------------------------------------------------------------------
#
# static/shared handling
#
AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)



###-------------------------------------------------------------------------
# Checks for programs.
#AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
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


###-------------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST


###-------------------------------------------------------------------------
# Checks for library functions.

AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(fork)
AC_CHECK_FUNCS(time)
AC_CHECK_FUNCS(getpid)
AC_CHECK_FUNCS(sigaction)
AC_CHECK_FUNCS(random)
AC_CHECK_FUNCS(wait)
AC_CHECK_FUNCS(chmod)
AC_CHECK_FUNCS(symlink)


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
# check for PC/SC
#

#AQ_CHECK_PCSC
#AC_MSG_CHECKING(if PC/SC might be used)
#if test "$have_pcsc" != "yes" ; then
#    AC_MSG_WARN(No pcsc libraries found, SCard driver will not be available.)
#else
#    all_includes="$all_includes $pcsc_includes"
#    AC_DEFINE(USE_PCSC, 1, [whether PC/SC is to be used])
#    AC_MSG_RESULT(yes)
#fi



###-------------------------------------------------------------------------
# check for Gwenhywfar
#

AC_GWENHYWFAR(1,4,0,0)

all_includes="$all_includes $gwenhywfar_includes"


###-------------------------------------------------------------------------
dnl OS dependant settings

AQ_CHECK_OS

case "$OS_TYPE" in
    posix)
        libchipcard2d_libs="-L\$(prefix)/lib -lchipcard2d"
        
        libchipcard2s_libs="-L\$(prefix)/lib -lchipcard2s"
        libchipcard2s_config_am="\$(sysconfdir)/chipcard2-server"

        libchipcard2sv_libs="-L\$(prefix)/lib -lchipcard2sv"
        libchipcard2c_libs="-L`eval echo $prefix/lib` -lchipcard2c"
        libchipcard2c_config_am="\$(sysconfdir)/chipcard2-client"
        chipcard_client_includes="-I`eval echo $prefix/include`"

        libchipcard2_includes="-I\$(prefix)/include"
        LC_DEVICEDRIVER_PATH="\$(prefix)/lib/chipcard2-server/drivers"
        LC_LOWLEVELDRIVER_PATH="\$(prefix)/lib/chipcard2-server/lowlevel"
        LC_SERVICE_PATH="\$(prefix)/lib/chipcard2-server/services"
        LC_DEFAULT_DHFILE="chipcardd-dh.pem"
        LC_DEFAULT_PIDDIR="\$(localstatedir)/run/chipcard2"
        LC_DEFAULT_LOGDIR="\$(localstatedir)/log/chipcard2"
        LC_DEFAULT_NEWCERTDIR="\$(localstatedir)/lib/chipcard2/newcerts"
        LC_DEFAULT_LIBDIR="\$(prefix)/lib/chipcard2"
    	;;
    windows)
        AC_DEFINE_UNQUOTED(BUILDING_LIBCHIPCARD2_DLL,1, [Define if DLL is built])
        
        libchipcard2d_libs="-L\$(prefix)/lib -lchipcard2d32"
        
        libchipcard2s_libs="-L\$(prefix)/lib -lchipcard2s32"
        libchipcard2s_config_am="\$(sysconfdir)/chipcard2-server"

        libchipcard2sv_libs="-L\$(prefix)/lib -lchipcard2sv"
        
        libchipcard2c_libs="-L`eval echo $prefix/lib` -lchipcard2c"
        libchipcard2c_config_am="\$(sysconfdir)/chipcard2-client"
        libchipcard2_includes="-I\$(prefix)/include"
        LC_DEVICEDRIVER_PATH="\$(prefix)/lib/chipcard2-server/drivers"
        LC_LOWLEVELDRIVER_PATH="\$(prefix)/lib/chipcard2-server/lowlevel"
        LC_SERVICE_PATH="\$(prefix)/lib/chipcard2-server/services"
        LC_DEFAULT_DHFILE="chipcardd-dh.pem"
        LC_DEFAULT_PIDDIR="\$(localstatedir)/run/chipcard2"
        LC_DEFAULT_LOGDIR="\$(localstatedir)/log/chipcard2"
        LC_DEFAULT_NEWCERTDIR="\$(localstatedir)/lib/chipcard2/newcerts"
        LC_DEFAULT_LIBDIR="\$(prefix)/lib/chipcard2"
        
        LIBCHIPCARD_MAKE_DLL_TARGET="dll"
        LIBCHIPCARD_INSTALL_DLL_TARGET="dll-install"
    	;;
esac

AC_SUBST(libchipcard2d_libs)
AC_SUBST(libchipcard2s_config_am)
AC_SUBST(libchipcard2s_libs)
AC_SUBST(libchipcard2sv_libs)
AC_SUBST(libchipcard2c_config)
AC_SUBST(libchipcard2c_config_am)
AC_SUBST(libchipcard2c_libs)
AC_SUBST(libchipcard2_includes)
AC_SUBST(LIBCHIPCARD_MAKE_DLL_TARGET)
AC_SUBST(LIBCHIPCARD_INSTALL_DLL_TARGET)
AC_SUBST(LC_DEVICEDRIVER_PATH)
AC_SUBST(LC_LOWLEVELDRIVER_PATH)
AC_SUBST(LC_SERVICE_PATH)
AC_SUBST(LC_DEFAULT_NEWCERTDIR)
AC_SUBST(LC_DEFAULT_DHFILE)
AC_SUBST(LC_DEFAULT_LIBDIR)
AC_SUBST(LC_DEFAULT_PIDDIR)
AC_SUBST(LC_DEFAULT_LOGDIR)
AC_SUBST(all_includes)
AC_SUBST(all_libraries)


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
	DOXYGEN_INPUT="chipcard2-client chipcard2-server"
	DOXYGEN_DEFINE="DOXYGEN_HIDE"
fi
AC_SUBST(DOXYGEN_INPUT)
AC_SUBST(DOXYGEN_DEFINE)
AC_MSG_RESULT($enable_fulldoc)


###-------------------------------------------------------------------------
#
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
#
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
#
# create header directory for server, make symlinks
#
lc_modules="server driver service"
lc_server_modules="commands"

lprefix="${srcdir}"
lprefix=`( cd ${lprefix} ; pwd )`

rm -Rf chipcard2-server
mkdir chipcard2-server
AQ_HEADER_FILES="${lprefix}/src/lib/chipcard2-server/version.h ${lprefix}/src/lib/chipcard2-server/chipcard2.h"

# symlink all headers from src/lib/chipcard2-server
hfiles=`( cd "${srcdir}/src/lib/chipcard2-server" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-server/${f}"
  case ${f} in
    *_p.h | *version.h | *chipcard2.h)
              ;;
    *)
              ln -s "${lprefix}/src/lib/chipcard2-server/${f}" "chipcard2-server/${f}"
              ;;
  esac
done

# symlink all headers from src/lib/chipcard2-server/*
for sm in $lc_modules; do
  mkdir chipcard2-server/${sm}
  hfiles=`( cd "${srcdir}/src/lib/chipcard2-server/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-server/${sm}/${f}"
    case ${f} in
      *_p.h)
            ;;
      *_l.h)
            ;;
      *)   
            ln -s "${lprefix}/src/lib/chipcard2-server/${sm}/${f}" "chipcard2-server/${sm}/${f}"
            ;;
    esac
  done
done
ln -s "${lprefix}/src/lib/chipcard2-server/version.h" chipcard2-server/version.h
ln -s "${lprefix}/src/lib/chipcard2-server/chipcard2.h" chipcard2-server/chipcard2.h

# symlink all headers from src/lib/chipcard/server/*
for sm in $lc_server_modules; do
  mkdir chipcard2-server/server/${sm}
  hfiles=`( cd "${srcdir}/src/lib/chipcard2-server/server/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-server/server/${sm}/${f}"
    case ${f} in
      *_p.h)
            ;;
      *_l.h)
            ;;
      *)   
            ln -s "${lprefix}/src/lib/chipcard2-server/server/${sm}/${f}" "chipcard2-server/server/${sm}/${f}"
            ;;
    esac
  done
done



###-------------------------------------------------------------------------
#
# create header directory for client, make symlinks
#
lc_modules="client cards mon fs"
lc_client_modules="apps lowlevel"

lprefix="${srcdir}"
lprefix=`( cd ${lprefix} ; pwd )`

rm -Rf chipcard2-client
mkdir chipcard2-client
AQ_HEADER_FILES="${lprefix}/src/lib/chipcard2-client/version.h ${lprefix}/src/lib/chipcard2-client/chipcard2.h"

# symlink all headers from src/lib/chipcard2-client
hfiles=`( cd "${srcdir}/src/lib/chipcard2-client" && ls *.h 2>/dev/null )`
for f in ${hfiles}; do
  AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-client/${f}"
  case ${f} in
    *_p.h | *version.h | *chipcard2.h)
              ;;
    *)
              ln -s "${lprefix}/src/lib/chipcard2-client/${f}" "chipcard2-client/${f}"
              ;;
  esac
done

# symlink all headers from src/lib/chipcard2-client/*
for sm in $lc_modules; do
  mkdir chipcard2-client/${sm}
  hfiles=`( cd "${srcdir}/src/lib/chipcard2-client/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-client/${sm}/${f}"
    case ${f} in
      *_p.h)
            ;;
      *_l.h)
            ;;
      *)   
            ln -s "${lprefix}/src/lib/chipcard2-client/${sm}/${f}" "chipcard2-client/${sm}/${f}"
            ;;
    esac
  done
done
ln -s "${lprefix}/src/lib/chipcard2-client/version.h" chipcard2-client/version.h
ln -s "${lprefix}/src/lib/chipcard2-client/chipcard2.h" chipcard2-client/chipcard2.h

# symlink all headers from src/lib/chipcard2-client/client/*
for sm in $lc_client_modules; do
  mkdir chipcard2-client/client/${sm}
  hfiles=`( cd "${srcdir}/src/lib/chipcard2-client/client/${sm}" && ls *.h 2>/dev/null )`
  for f in ${hfiles}; do
    AQ_HEADER_FILES="${AQ_HEADER_FILES} ${lprefix}/src/lib/chipcard2-client/client/${sm}/${f}"
    case ${f} in
      *_p.h)
            ;;
      *_l.h)
            ;;
      *)   
            ln -s "${lprefix}/src/lib/chipcard2-client/client/${sm}/${f}" "chipcard2-client/client/${sm}/${f}"
            ;;
    esac
  done
done




###-------------------------------------------------------------------------
AC_CONFIG_FILES([Makefile
                 chipcard2-client-config.in
                 chipcard2-server-config.in
                 bin/Makefile
                 bin/prepare.sh.in
                 data/Makefile
                 data/apps/Makefile
                 data/cards/Makefile
                 data/drivers/Makefile
                 data/drivers/ccid_ifd/Makefile
                 data/drivers/cyberjack_ctapi/Makefile
                 data/drivers/cyberjack2_ctapi/Makefile
                 data/drivers/kobil_ctapi/Makefile
                 data/drivers/omnikey_ifd/Makefile
                 data/drivers/orga_ctapi/Makefile
                 data/drivers/scmx32_ifd/Makefile
                 data/drivers/tow_ctapi/Makefile
                 doc/Makefile
                 src/Makefile
                 src/lib/Makefile
                 src/lib/chipcard2-client/Makefile
                 src/lib/chipcard2-client/cards/Makefile
                 src/lib/chipcard2-client/client/Makefile
                 src/lib/chipcard2-client/client/apps/Makefile
                 src/lib/chipcard2-client/client/lowlevel/Makefile
                 src/lib/chipcard2-client/mon/Makefile
                 src/lib/chipcard2-client/fs/Makefile
                 src/lib/chipcard2-client/chipcard2.h
                 src/lib/chipcard2-client/version.h
                 src/lib/chipcard2-server/Makefile
                 src/lib/chipcard2-server/driver/Makefile
                 src/lib/chipcard2-server/server/Makefile
                 src/lib/chipcard2-server/server/commands/Makefile
                 src/lib/chipcard2-server/service/Makefile
                 src/lib/chipcard2-server/chipcard2.h
                 src/lib/chipcard2-server/version.h
                 src/drivers/Makefile
                 src/drivers/ctapi/Makefile
                 src/drivers/ifd/Makefile
                 src/drivers/pcsc/Makefile
                 src/tools/Makefile
                 src/tools/chipcardd2/Makefile
                 src/tools/rsacard/Makefile
                 src/tools/geldkarte/Makefile
                 src/tools/memcard/Makefile
                 src/tools/chipcard-tool/Makefile
                 src/tools/kvkcard/Makefile
                 src/tools/cardcommander/Makefile
                 src/test/Makefile
                 tutorials/Makefile
                 m4/Makefile
                ])
AC_OUTPUT
