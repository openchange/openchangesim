# Configuration Script for OpenChangeSim
# Written by Julien Kerihuel <j.kerihuel@openchange.org>

AC_PREREQ(2.57)
AC_INIT(openchangesim, 0.1, [j.kerihuel@openchange.org])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AC_DEFINE(_GNU_SOURCE, 1, [Use GNU extensions])

PKG_PROG_PKG_CONFIG([0.20])

CFLAGS="-I. $CFLAGS"

dnl ##################################################################
dnl Some general portability stuff
dnl ##################################################################
AC_CHECK_HEADERS( sys/cdefs.h string.h sys/sockio.h)
AC_CHECK_FUNCS( strcasestr )

dnl ##################################################################
dnl Check for CC
dnl ##################################################################
AC_PROG_CC

dnl ###########################################################################
dnl _AC_LANG_COMPILER_ICC
dnl Check whether the compiler for the current language is really ICC.
dnl ###########################################################################
m4_define([AC_LANG_COMPILER_ICC],
[AC_CACHE_CHECK([whether we are really using the Intel _AC_LANG compiler],
		[ac_cv_[]_AC_LANG_ABBREV[]_compiler_icc],
[_AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#ifndef __INTEL_COMPILER
       choke me
#endif
]])],
		   [ac_compiler_icc=yes],
		   [ac_compiler_icc=no])
ac_cv_[]_AC_LANG_ABBREV[]_compiler_icc=$ac_compiler_icc
])])

dnl ###########################################################################
dnl _AC_LANG_COMPILER_SUNCC
dnl Check whether the compiler for the current language is really Sun compiler.
dnl ###########################################################################
m4_define([AC_LANG_COMPILER_SUNCC],
[AC_CACHE_CHECK([whether we are really using the Sun _AC_LANG compiler],
		[ac_cv_[]_AC_LANG_ABBREV[]_compiler_suncc],
[_AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#ifndef __SUNPRO_C
       choke me
#endif
]])],
		   [ac_compiler_suncc=yes],
		   [ac_compiler_suncc=no])
ac_cv_[]_AC_LANG_ABBREV[]_compiler_suncc=$ac_compiler_suncc
])])

dnl ##################################################################
dnl Set up the right compiler options
dnl ##################################################################
AC_LANG_COMPILER_SUNCC 
AC_LANG_COMPILER_ICC

if test x"$ac_cv_c_compiler_suncc" = x"yes"; then
dnl Sun Studio Compiler
    COMPILER_OPTIONS_SHARED="-D__FUNCTION__=__func__"
    COMPILER_OPTIONS_C="$COMPILER_OPTIONS_SHARED"
    COMPILER_OPTIONS_CXX="$COMPILER_OPTIONS_SHARED"
    LDFLAGS="$LDFLAGS -z ignore -R '\$\$ORIGIN/../lib'"
elif test x"$ac_cv_c_compiler_icc" = x"yes"; then
dnl Intel Compiler
    COMPILER_OPTIONS_SHARED="-O3 -Wall -g3 -fstrict-aliasing -Wmissing-prototypes -Wstrict-prototypes -wd2259,188,593,869,981,181,1419,2218"
    COMPILER_OPTIONS_C="$COMPILER_OPTIONS_SHARED"
    COMPILER_OPTIONS_CXX="$COMPILER_OPTIONS_SHARED"
else
dnl GNU Compiler
    COMPILER_OPTIONS_SHARED="-Wall -g3 -fstrict-aliasing -Wp,-D_FORTIFY_SOURCE=2"
    if test "x$use_cov" = "xyes"; then
        COMPILER_OPTIONS_SHARED="-O0 $COMPILER_OPTIONS_SHARED"
    else
        COMPILER_OPTIONS_SHARED="-O3 $COMPILER_OPTIONS_SHARED"
    fi
    COMPILER_OPTIONS_C="$COMPILER_OPTIONS_SHARED -Wmissing-prototypes -Wstrict-prototypes"
    if test "x$use_cov" = "xyes"; then
        COMPILER_OPTIONS_C="$COMPILER_OPTIONS_C -fprofile-arcs -ftest-coverage"
        LDFLAGS="$LDFLAGS -lgcov --coverage"
    fi
    COMPILER_OPTIONS_CXX="$COMPILER_OPTIONS_SHARED"
fi
AC_SUBST(COMPILER_OPTIONS_C)
AC_SUBST(COMPILER_OPTIONS_CXX)

dnl ##################################################################
dnl Check for install
dnl ##################################################################
AC_PROG_INSTALL

dnl ##################################################################
dnl Check for dependencies
dnl ##################################################################

dnl --------------------------------------------------------------------------
dnl Check for popt
dnl --------------------------------------------------------------------------

AC_CHECK_LIB([popt], [poptFreeContext], 
             [
	       AC_DEFINE(HAVE_LIBPOPT, 1, [Define if you want to use libpopt])
	       enable_libpopt="yes"
             ], 
             [ 
               AC_MSG_ERROR([libpopt is missing - can't build openchangesim]) 
	       enable_libpopt="no"
             ])


PKG_CHECK_MODULES(LIBMAPI, libmapi)

dnl ##################################################################
dnl Makefile
dnl ##################################################################
AC_CONFIG_FILES([config.mk])
AC_OUTPUT

dnl ##################################################################
dnl Print configuration info
dnl ##################################################################
AC_MSG_RESULT([
===============================================================
OpenChange Configuration (Please review)

	   * Install:
	     - prefix:			$prefix

run make
===============================================================

])