##--##--##--##--##borrowed from EFL autoconf (LGPL)##--thanks rasties!##
m4_define([v_maj], [1])
m4_define([v_min], [0])
m4_define([v_mic], [0])
m4_define([v_rev], m4_esyscmd([(svnversion -n "${SVN_REPO_PATH:-.}" | grep -v export || echo 0) | awk -F : '{printf("%s\n", $1);}' | tr -d ' :MSP\n']))
m4_if(v_rev, [0], [m4_define([v_rev], m4_esyscmd([git svn log --limit 1 --oneline 2>| /dev/null | sed -e 's/r\([0-9]\+\).*/\1/' | tr -d '\n' || echo 0000]))])
m4_if(v_rev, [], [m4_define([v_rev], [0])])
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
##--##--##--##--##
AC_PREREQ([2.57])
AC_INIT([zrpc],[v_ver],[dev@zentific.com])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(config.h)

AC_LIBTOOL_DLOPEN
AC_GNU_SOURCE
AC_PROG_LIBTOOL
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

if test -z "$v_rev" ; then
	v_rev=0
fi
AC_DEFINE_UNQUOTED(ZENTIFIC_REVISION, [v_rev], [Revison])

AM_INIT_AUTOMAKE([foreign 1.7])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
ZRPC_VERSION=1:0:0
AC_SUBST(ZRPC_VERSION)

# Checks for programs.
AC_C___ATTRIBUTE__
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

AC_CHECK_HEADERS([arpa/inet.h fcntl.h memory.h netdb.h netinet/in.h stdint.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h termios.h])
AC_CHECK_HEADERS([security/pam_appl.h security/pam_misc.h],,[AC_MSG_ERROR([missing pam headers!])])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_ALLOCA

PKG_CHECK_MODULES([AZY], [azy]) 
PKG_CHECK_MODULES([OPENSSL], [libssl >= 0.9.8])
PKG_CHECK_MODULES([ZSHARE], [libzshare])
PKG_CHECK_MODULES([EFL], [eina >= 1.0 ecore >= 1.0])

PKG_CHECK_MODULES([ESQL], [esskyuehl])

AC_CHECK_LIB(pam, pam_authenticate,[], [AC_MSG_ERROR([unable to find functioning pam])])
PAM_LIBS="-ldl -lpam -lpam_misc"
AC_SUBST(PAM_LIBS)

AC_ARG_ENABLE([devel], AS_HELP_STRING([--enable-devel],[enable devel mode @<:@default=no@:>@]))
if test "x$enable_devel" = "xyes";then
	AC_DEFINE([ZDEV_ENABLE_DEVEL_SERVLET_DO_NOT_BUILD_FOR_PRODUCTION], [1], [enable extra devel output/functions])
	AC_DEFINE([ZDEV_DEBUG], [1], [enable extra debug output])
fi

AM_CONDITIONAL([ZDEV_ENABLE_DEVEL_SERVLET_DO_NOT_BUILD_FOR_PRODUCTION], [test "x$enable_devel" = "xyes"])

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug],[enable debug cflags @<:@default=yes@:>@]))
if test "x$enable_debug" != "xno";then
	DEBUGCFLAGS="-g -ggdb3 -Wall"
	DEBUGLDFLAGS="-rdynamic"
fi
AC_SUBST(DEBUGFLAGS)

AC_ARG_ENABLE([zrpc], AS_HELP_STRING([--enable-zrpc],[enable zrpc binary build @<:@default=yes@:>@]))
AM_CONDITIONAL([ZRPC], [test "x$enable_zrpc" != "xno"])

AZY_DIR='$(top_srcdir)/src/servlets/azy'
AC_SUBST(AZY_DIR)

FUNC_DIR='$(top_srcdir)/src/servlets/functions'
AC_SUBST(FUNC_DIR)

ZRPC_DIR='$(top_srcdir)/src/bin'
AC_SUBST(ZRPC_DIR)

ZRPC_CHECK_DISTRO

AC_OUTPUT([
Makefile
db_sources/Makefile
db_sources/mysql/Makefile
src/Makefile
src/bin/Makefile
src/servlets/Makefile
src/servlets/azy/Makefile
src/utils/Makefile
include/Makefile
init/Makefile 
init/debian/Makefile
init/ubuntu/Makefile
init/centos/Makefile
init/suse/Makefile
init/gentoo/Makefile
])
