# $Id: chipcard.m4 79 2005-05-31 22:50:34Z aquamaniac $
# (c) 2004-2006 Martin Preuss<martin@libchipcard.de>
# This function checks for chipcard-client and chipcard-server

AC_DEFUN([AC_CHIPCARD_CLIENT], [
dnl searches for chipcard_client
dnl Arguments: 
dnl   $1: major version minimum
dnl   $2: minor version minimum
dnl   $3: patchlevel version minimum
dnl   $4: build version minimum
dnl Returns: chipcard_client_dir
dnl          chipcard_client_datadir
dnl          chipcard_client_libs
dnl          chipcard_client_includes
dnl          have_chipcard_client

if test -z "$1"; then vma="0"; else vma="$1"; fi
if test -z "$2"; then vmi="1"; else vmi="$2"; fi
if test -z "$3"; then vpl="0"; else vpl="$3"; fi
if test -z "$4"; then vbld="0"; else vbld="$4"; fi

AC_MSG_CHECKING(if chipcard_client support desired)
AC_ARG_ENABLE(chipcard-client,
  [  --enable-chipcard-client      enable chipcard_client support (default=yes)],
  enable_chipcard_client="$enableval",
  enable_chipcard_client="yes")
AC_MSG_RESULT($enable_chipcard_client)

have_chipcard_client="no"
chipcard_client_dir=""
chipcard_client_datadir=""
chipcard_client_libs=""
chipcard_client_includes=""
chipcard_client_servicedir=""
if test "$enable_chipcard_client" != "no"; then
  AC_MSG_CHECKING(for chipcard_client)
  AC_ARG_WITH(chipcard-client-dir,
    [  --with-chipcard-client-dir=DIR    obsolete - set PKG_CONFIG_PATH environment variable instead],
    [AC_MSG_RESULT([obsolete configure option '--with-chipcard-client-dir' used])
     AC_MSG_ERROR([
*** Configure switch '--with-chipcard-client-dir' is obsolete.
*** If you want to use libchipcardc from a non-system location
*** then locate the file 'libchipcard-client.pc' and add its parent directory
*** to environment variable PKG_CONFIG_PATH. For example
*** configure <options> PKG_CONFIG_PATH="<path-to-libchipcard-client.pc's-dir>:\${PKG_CONFIG_PATH}"])],
    [])

  $PKG_CONFIG --exists libchipcard-client
  result=$?
  if test $result -ne 0; then
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR([
*** Package libchipcard-client was not found in the pkg-config search path.
*** Perhaps you should add the directory containing `libchipcard-client.pc'
*** to the PKG_CONFIG_PATH environment variable])
  else
      chipcard_client_dir="`$PKG_CONFIG --variable=prefix libchipcard-client`"
      AC_MSG_RESULT($chipcard_client_dir)
  fi

  AC_MSG_CHECKING(for chipcard-client libs)
  chipcard_client_libs="`$PKG_CONFIG --libs libchipcard-client`"
  AC_MSG_RESULT($chipcard_client_libs)

  AC_MSG_CHECKING(for chipcard-client includes)
  chipcard_client_includes="`$PKG_CONFIG --cflags libchipcard-client`"
  AC_MSG_RESULT($chipcard_client_includes)

  AC_MSG_CHECKING(for chipcard-client datadir)
  chipcard_client_datadir="`$PKG_CONFIG --variable=pkgdatadir libchipcard-client`"
  AC_MSG_RESULT($chipcard_client_datadir)

  AC_MSG_CHECKING(if chipcard_client test desired)
  AC_ARG_ENABLE(chipcard-client-test,
    [  --enable-chipcard-client-test   enable chipcard_client-test (default=yes)],
     enable_chipcard_client_test="$enableval",
     enable_chipcard_client_test="yes")
  AC_MSG_RESULT($enable_chipcard_client_test)
  AC_MSG_CHECKING(for Chipcard-Client version >=$vma.$vmi.$vpl.$vbld)
  if test "$enable_chipcard_client_test" != "no"; then
    lcc_vmajor="`$PKG_CONFIG --variable=vmajor libchipcard-client`"
    lcc_vminor="`$PKG_CONFIG --variable=vminor libchipcard-client`"
    lcc_vpatchlevel="`$PKG_CONFIG --variable=vpatchlevel libchipcard-client`"
    lcc_vstring="`$PKG_CONFIG --variable=vstring libchipcard-client`"
    lcc_vbuild="`$PKG_CONFIG --variable=vbuild libchipcard-client`"
    lcc_versionstring="$lcc_vstring.$lcc_vbuild"
    AC_MSG_RESULT([found $lcc_versionstring])
    if test "$vma" -gt "$lcc_vmajor"; then
      AC_MSG_ERROR([Your Chipcard-Client version is way too old.
      Please update from https://www.aquamaniac.de])
    elif test "$vma" = "$lcc_vmajor"; then
      if test "$vmi" -gt "$lcc_vminor"; then
        AC_MSG_ERROR([Your Chipcard-Client version is too old.
          Please update from https://www.aquamaniac.de])
      elif test "$vmi" = "$lcc_vminor"; then
          if test "$vpl" -gt "$lcc_vpatchlevel"; then
            AC_MSG_ERROR([Your Chipcard-Client version is a little bit too old.
            Please update from https://www.aquamaniac.de])
          elif test "$vpl" = "$lcc_vpatchlevel"; then
            if test "$vbld" -gt "$lcc_vbuild"; then
              AC_MSG_ERROR([Your Chipcard-Client version is a little bit too old.
  Please update to the latest git version. Instructions for accessing
  git can be found on https://www.aquamaniac.de])
             fi
           fi
      fi
    fi
    have_chipcard_client="yes"
    #AC_MSG_RESULT(yes)
  else
    have_chipcard_client="yes"
    AC_MSG_RESULT(assuming yes)
  fi
dnl end of "if enable-chipcard-client"
fi

AC_SUBST(chipcard_client_dir)
AC_SUBST(chipcard_client_datadir)
AC_SUBST(chipcard_client_libs)
AC_SUBST(chipcard_client_includes)
])





AC_DEFUN([AC_CHIPCARD_SERVER], [
dnl searches for chipcard_server
dnl Arguments: 
dnl   $1: major version minimum
dnl   $2: minor version minimum
dnl   $3: patchlevel version minimum
dnl   $4: build version minimum
dnl Returns: chipcard_server_datadir
dnl          chipcard_server_driverdir
dnl          chipcard_server_servicedir
dnl          have_chipcard_server

if test -z "$1"; then vma="0"; else vma="$1"; fi
if test -z "$2"; then vmi="1"; else vmi="$2"; fi
if test -z "$3"; then vpl="0"; else vpl="$3"; fi
if test -z "$4"; then vbld="0"; else vbld="$4"; fi

AC_MSG_CHECKING(if chipcard_server support desired)
AC_ARG_ENABLE(chipcard-server,
  [  --enable-chipcard-server      enable chipcard_server support (default=yes)],
  enable_chipcard_server="$enableval",
  enable_chipcard_server="yes")
AC_MSG_RESULT($enable_chipcard_server)

have_chipcard_server="no"
chipcard_server_dir=""
chipcard_server_servicedir=""
chipcard_server_driverdir=""
chipcard_server_datadir=""
if test "$enable_chipcard_server" != "no"; then
  AC_MSG_CHECKING(for chipcard_server)
  AC_ARG_WITH(chipcard-server-dir,
    [  --with-chipcard-server-dir=DIR    obsolete - set PKG_CONFIG_PATH environment variable instead],
    [AC_MSG_RESULT([obsolete configure option '--with-chipcard-server-dir' used])
     AC_MSG_ERROR([
*** Configure switch '--with-chipcard-server-dir' is obsolete.
*** If you want to use libchipcards from a non-system location
*** then locate the file 'libchipcard-server.pc' and add its parent directory
*** to environment variable PKG_CONFIG_PATH. For example
*** configure <options> PKG_CONFIG_PATH="<path-to-libchipcard-server.pc's-dir>:\${PKG_CONFIG_PATH}"])],
    [])

  $PKG_CONFIG --exists libchipcard-server
  result=$?
  if test $result -ne 0; then
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR([
*** Package libchipcard-server was not found in the pkg-config search path.
*** Perhaps you should add the directory containing `libchipcard-server.pc'
*** to the PKG_CONFIG_PATH environment variable])
  else
      chipcard_server_dir="`$PKG_CONFIG --variable=prefix libchipcard-server`"
      AC_MSG_RESULT($chipcard_server_dir)
  fi

  AC_MSG_CHECKING(for chipcard-server datadir)
  chipcard_server_datadir="`$PKG_CONFIG --variable=pkgdatadir libchipcard-server`"
  AC_MSG_RESULT($chipcard_server_datadir)

  AC_MSG_CHECKING(for chipcard-server driver dir)
  chipcard_server_driverdir="`$PKG_CONFIG --driverdir libchipcard-server`"
  AC_MSG_RESULT($chipcard_server_driverdir)

  AC_MSG_CHECKING(for chipcard-server service dir)
  chipcard_server_servicedir="`$PKG_CONFIG --servicedir libchipcard-server`"
  AC_MSG_RESULT($chipcard_server_servicedir)

  AC_MSG_CHECKING(if chipcard_server test desired)
  AC_ARG_ENABLE(chipcard-server-test,
    [  --enable-chipcard-server-test   enable chipcard_server-test (default=yes)],
     enable_chipcard_server_test="$enableval",
     enable_chipcard_server_test="yes")
  AC_MSG_RESULT($enable_chipcard_server_test)
  AC_MSG_CHECKING(for Chipcard-Server version >=$vma.$vmi.$vpl.$vbld)
  if test "$enable_chipcard_server_test" != "no"; then
    lcs_vmajor="`$PKG_CONFIG --variable=vmajor libchipcard-server`"
    lcs_vminor="`$PKG_CONFIG --variable=vminor libchipcard-server`"
    lcs_vpatchlevel="`$PKG_CONFIG --variable=vpatchlevel libchipcard-server`"
    lcs_vstring="`$PKG_CONFIG --variable=vstring libchipcard-server`"
    lcs_vbuild="`$PKG_CONFIG --variable=vbuild libchipcard-server`"
    lcs_versionstring="$lcs_vstring.$lcs_vbuild"
    AC_MSG_RESULT([found $lcs_versionstring])
    if test "$vma" -gt "$lcs_vmajor"; then
      AC_MSG_ERROR([Your Chipcard-Server version is way too old.
      Please update from https://www.aquamaniac.de])
    elif test "$vma" = "$lcs_vmajor"; then
      if test "$vmi" -gt "$lcs_vminor"; then
        AC_MSG_ERROR([Your Chipcard-Server version is too old.
          Please update from https://www.aquamaniac.de])
      elif test "$vmi" = "$lcs_vminor"; then
          if test "$vpl" -gt "$lcs_vpatchlevel"; then
            AC_MSG_ERROR([Your Chipcard-Server version is a little bit too old.
            Please update from https://www.aquamaniac.de])
          elif test "$vpl" = "$lcs_vpatchlevel"; then
            if test "$vbld" -gt "$lcs_vbuild"; then
              AC_MSG_ERROR([Your Chipcard-Server version is a little bit too old.
  Please update to the latest git version. Instructions for accessing
  git can be found on https://www.aquamaniac.de])
             fi
           fi
      fi
    fi
    have_chipcard_server="yes"
    #AC_MSG_RESULT(yes)
  else
    have_chipcard_server="yes"
    AC_MSG_RESULT(assuming yes)
  fi
dnl end of "if enable-chipcard-server"
fi

AC_SUBST(chipcard_server_servicedir)
AC_SUBST(chipcard_server_driverdir)
AC_SUBST(chipcard_server_datadir)
])


