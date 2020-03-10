#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)

AC_INIT(USF scope, 1.10.7, dshuman@usf.edu, scope)

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([WritePop.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_HAVE_LIBRARY(-lm)
AC_HAVE_LIBRARY(-lfftw3f)
AC_PATH_XTRA

# Checks for header files.
AC_CHECK_HEADERS(asm-i486/msr.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_SIZEOF(long)

# Checks for library functions.
AC_CHECK_DECLS([getline],,[AC_LIBOBJ(getline)])

AC_REPLACE_FUNCS(asprintf)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
