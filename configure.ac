AC_PREREQ([2.66])
AC_INIT([fs-utils], [0.01], [http://github.com/stacktic/fs-utils])

AC_CONFIG_SRCDIR([lib/fattr.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects -Wall -Werror])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

AC_SYS_LARGEFILE

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([util], [forkpty])
AC_CHECK_LIB([socket], [socket])

AC_TRY_LINK_FUNC([nanosleep],,
        AC_CHECK_LIB([rt], [nanosleep])
)

# Checks for header files.
AC_CHECK_HEADERS([err.h sys/cdefs.h sys/mkdev.h])

# librump depends on librumpuser which has platform specific
# dependencies.  trust that things are ok if rump.h is found.
# need to fix this later to autodetect the dependencies.
# AC_CHECK_LIB([rump], [rump__init],, AC_MSG_ERROR([rump kernel required]))

AC_CHECK_HEADERS([rump/rump.h],, AC_MSG_ERROR([rump kernel required]))

AC_C_BIGENDIAN

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
#AC_TYPE_UID_T
#AC_TYPE_INT16_T
#AC_TYPE_INT32_T
#AC_TYPE_INT64_T
#AC_TYPE_MODE_T
#AC_TYPE_OFF_T
#AC_TYPE_PID_T
#AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
#AC_CHECK_MEMBERS([struct stat.st_blksize])
#AC_STRUCT_ST_BLOCKS
#AC_CHECK_MEMBERS([struct stat.st_rdev])
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
#AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([getprogname humanize_number setmode strmode strlcpy strsep strvis])

AC_ARG_ENABLE([nfs],
	      [  --enable-nfs            Enable Network File System],
	      [enable_nfs=true],[enable_nfs=false])
AM_CONDITIONAL([ENABLE_NFS], [test x$enable_nfs = xtrue])

AC_TRY_LINK_FUNC([lchmod],
	[AC_DEFINE([HAVE_LCHMOD], 1, [define if lchmod() is supported])
	  WRAP_LCHMOD="-Wl,--wrap=lchmod"]
)
AC_SUBST([WRAP_LCHMOD])

AC_CHECK_DECL([optreset],
	[AC_DEFINE(HAVE_GETOPT_OPTRESET, 1, [define for getopt/optreset])],,
	[#include <getopt.h>])

AC_CHECK_MEMBERS([struct tm.tm_gmtoff],,,[#include <time.h>])
AC_CHECK_MEMBERS([struct dirent.d_namlen],,,[#include <dirent.h>])
AC_CHECK_MEMBERS([struct dirent.d_ino],,,[#include <dirent.h>])
AC_CHECK_MEMBERS([struct stat.st_atimespec],,,[#include <sys/stat.h>])
AC_CHECK_MEMBERS([struct stat.st_flags],,,[#include <sys/stat.h>])
AC_CHECK_MEMBERS([struct stat.st_gen],,,[#include <sys/stat.h>])

EXTRA_LIBS=""
AS_CASE([$target_os],
	[linux*], [EXTRA_LIBS="-lrumpnet_sockin -lrumpnet_net -lrumpnet -lrumpkern_sys_linux"],
	[solaris*], [EXTRA_LIBS="-lrumpkern_sys_sunos"],
	[AC_MSG_ERROR([Conversions for $target_os data types not available])])
AC_SUBST([EXTRA_LIBS])

AC_CACHE_CHECK([whether linker requires --no-as-needed],
	[my_cv_as_needed],
	[if $CC -Wl,--help 2>&1 | grep -q as-needed; then
		LINKER_NO_AS_NEEDED="-Wl,--no-as-needed"
		my_cv_as_needed=yes;
	else
		LINKER_NO_AS_NEEDED=""
		my_cv_as_needed=no;
	fi])
AC_SUBST([LINKER_NO_AS_NEEDED])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
