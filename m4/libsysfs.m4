AC_DEFUN([AC_LIBSYSFS], [
dnl searches for libusysfs
dnl IN: 
dnl   nothing
dnl OUT:
dnl   Variables:
dnl     libsysfs_libs     : stuff for LDFLAGS (or LIBADD)
dnl     libsysfs_includes : includes (for CFLAGS)
dnl     have_libsysfs     : "yes" if libsysfs is supported
have_libsysfs="no"
libsysfs_libs=""
libsysfs_includes=""
AC_CHECK_HEADER([sysfs/libsysfs.h],
                [
  AC_CHECK_LIB(sysfs,sysfs_get_mnt_path,[
        have_libsysfs="yes"
	libsysfs_libs="-lsysfs"
	AC_SUBST(libsysfs_libs)
	AC_SUBST(libsysfs_includes)
	
     ],AC_MSG_RESULT([libsysfs.so not found.]))
  ],
                [AC_MSG_RESULT([sysfs/libsysfs.h not found.])])
])
