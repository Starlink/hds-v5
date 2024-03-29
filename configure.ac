dnl    Process this file with autoconf to produce a configure script
AC_REVISION($Revision$)

dnl    Initialisation: package name and version number
AC_INIT([hds-v5],[1.0-2],[starlink@jiscmail.ac.uk])
AC_CONFIG_AUX_DIR([build-aux])

dnl    Require autoconf-2.50 at least
AC_PREREQ([2.69])
dnl    Require Starlink automake at least
AM_INIT_AUTOMAKE([1.8.2-starlink subdir-objects])

dnl    Sanity-check: name a file in the source directory -- if this
dnl    isn't found then configure will complain
AC_CONFIG_SRCDIR([hdsTest.c])

dnl    Include defaults for Starlink configurations
STAR_DEFAULTS

dnl    Find required versions of the programs we need for configuration
AC_PROG_CC
AC_PROG_FC
LT_INIT

#   If --with-pic=no is set we should honour that.
AM_CONDITIONAL(NOPIC, test x$pic_mode = xno)

STAR_CNF_COMPATIBLE_SYMBOLS

dnl    We need HDF5
AC_CHECK_LIB([hdf5],[H5Fopen])

dnl    Used for dynamic header files
AC_CHECK_HEADERS(time.h)

dnl    Look for standard headers rather than assuming availability
dnl    by operating system
AC_HEADER_STDC
AC_CHECK_HEADERS(stddef.h)
AC_CHECK_HEADERS(unistd.h)

dnl    Check for largefile support (various macros and fseeko).
dnl    Make sure we don't use the cached version for this (can
dnl    be incorrect due to _POSIX_C_SOURCE on Solaris).
$as_unset ac_cv_sys_largefile_source
STAR_LARGEFILE_SUPPORT

dnl    Calculate sizes and types
AC_CHECK_TYPES([long long int,int,long,long long,off_t,int64_t])
AC_CHECK_TYPES([uint64_t, int32_t, uint32_t, long double, signed char])

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(size_t)

dnl    Declare the build and use dependencies for this package
STAR_DECLARE_DEPENDENCIES([build], [sae chr cnf ems one prm starmem hdf5])
STAR_DECLARE_DEPENDENCIES([link],  [chr cnf ems one starmem one hdf5])
STAR_DECLARE_DEPENDENCIES([test],  [chr cnf ems prm starmem hdf5])

dnl    There are two .msg files in this directory, so we need to specify
dnl    the names of them explicitly, rather than them being defaulted from
dnl    the package name.
STAR_MESSGEN([dat_err.msg])

AM_CONFIG_HEADER(config.h)

dnl    If you wish to configure extra files, you can add them to this
dnl    declaration.
AC_CONFIG_FILES(Makefile component.xml)

dnl    This is the bit that does the actual work
AC_OUTPUT
