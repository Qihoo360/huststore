#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([hustdb], [1.6], [https://github.com/Qihoo360/huststore])
AC_CONFIG_SRCDIR([server/main.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(hustdb,1.6)


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL


# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([crypto], [-lcrypto])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [-lpthread])
# FIXME: Replace `main' with a function in `-lrt':
AC_CHECK_LIB([rt], [-lrt])
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([ssl], [-lssl])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h mach/mach.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit clock_gettime dup2 getcwd gettimeofday inet_ntoa localtime_r memmove memset mkdir munmap setlocale strcasecmp strchr strerror strrchr strstr strtol strtoul strtoull])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([server/Makefile])
AC_CONFIG_FILES([server/network/Makefile])
AC_CONFIG_FILES([server/module/mdb/Makefile])
AC_CONFIG_FILES([server/module/rdb/Makefile])
AC_CONFIG_FILES([server/module/binlog/Makefile])
AC_OUTPUT
