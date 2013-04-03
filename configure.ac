AC_PREREQ([2.66])
AC_INIT([fs-utils], [0.0], [http://github.com/stacktic/fs-utils])

AC_CONFIG_SRCDIR([lib/fattr.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects -Wall -Werror])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([bsd], [strlcpy])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([util], [forkpty])

# Checks for header files.
AC_CHECK_HEADERS([err.h])

# librump depends on librumpuser which has platform specific
# dependencies.  trust that things are ok if rump.h is found.
# need to fix this later to autodetect the dependencies.
# AC_CHECK_LIB([rump], [rump__init],, AC_MSG_ERROR([rump kernel required]))

AC_CHECK_HEADERS([rump/rump.h],, AC_MSG_ERROR([rump kernel required]))

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
#AC_FUNC_CHOWN
#AC_FUNC_FORK
#AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
#AC_HEADER_MAJOR
#AC_FUNC_MALLOC
#AC_FUNC_MBRTOWC
#AC_FUNC_MKTIME
#AC_FUNC_MMAP
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([atexit gettimeofday isascii iswprint lchown localeconv memmove memset mkdir mkfifo munmap realpath rmdir setenv setlocale socket strcasecmp strchr strcspn strdup strerror strrchr strspn strtol strtoul wcwidth])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
