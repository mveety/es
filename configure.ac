dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([access.c])
AC_CONFIG_HEADERS([config.h])

dnl AC_CMDSTDOUT_CPP(variable, command, headers)
AC_DEFUN([AC_CMDSTDOUT_CPP],
[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
[$3]
EOF
$1=`(eval "$ac_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD | $2`
rm -f conftest*
])


use_readline=yes
use_editline=no

AC_ARG_WITH(readline,
--with-readline		Use GNU Readline, use_readline=yes)
AC_ARG_WITH(editline,
--with-editline		Use libedit, [use_editline=yes; use_readline=no])

AC_CANONICAL_HOST

dnl saved_CFLAGS="$CFLAGS"

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_YACC

AC_CACHE_CHECK(whether $CC accepts -Wno-clobbered, es_cv_no_clobbered, 
[
echo "int main(int argc, char *argv[]) { return 0; }" > conftest.c
if $CC -Wno-clobbered -Werror conftest.c -o /dev/null >/dev/null 2>&1; then
	CFLAGS="$CFLAGS -Wno-clobbered"
	echo -n yes
else
	echo -n no
fi])

dnl CFLAGS="$CFLAGS $saved_CFLAGS"

dnl ----------------------------
dnl CHECK FOR /dev/fd FILESYSTEM
dnl ----------------------------
AC_CACHE_CHECK(for /dev/fd filesystem, es_cv_sys_dev_fd,
[test -d /dev/fd && es_cv_sys_dev_fd=yes || es_cv_sys_dev_fd=no])
if test $es_cv_sys_dev_fd = yes; then
  AC_DEFINE(HAVE_DEV_FD)
fi

AC_SYS_INTERPRETER
if test "$ac_cv_sys_interpreter" = yes
then
	AC_DEFINE(KERNEL_POUNDBANG)
fi


dnl Checks for libraries.

if test "$use_readline" = yes || test "$use_editline" = yes
then
	AC_CHECK_LIB(terminfo, main)
	AC_CHECK_LIB(termcap, main)
	if test "$use_readline" = yes
	then
		AC_CHECK_LIB(readline, readline)
	elif test "$use_editline" = yes
	then
		AC_CHECK_LIB(edit, readline)
	fi

fi


dnl Checks for header files.
#AC_HEADER_DIRENT
#m4_warn([obsolete],
#[The preprocessor macro 'STDC_HEADERS' is obsolete.
#  Except in unusual embedded environments, you can safely include all
#  C89 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
#AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/time.h unistd.h memory.h stdarg.h sys/cdefs.h)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_GETGROUPS
#m4_warn([obsolete],
#[m4_warn([obsolete],
#[AC_PROG_GCC_TRADITIONAL is obsolete; use AC_PROG_CC
##])dnl
#AC_REQUIRE(AC_PROG_CC) is obsolete; use AC_PROG_CC
##])dnl
#AC_REQUIRE(AC_PROG_CC)
AC_PROG_CC
AC_FUNC_MMAP
#m4_warn([obsolete],
#[your code may safely assume C89 semantics that RETSIGTYPE is void.
#Remove this warning and the 'AC_CACHE_CHECK' when you adjust the code.])dnl
#AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
#[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
#],
#		 [return *(signal (0, 0)) (0) == 1;])],
#		   [ac_cv_type_signal=int],
#		   [ac_cv_type_signal=void])])
#AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
#		    ('int' or 'void').])
#
#
#if test "$ac_cv_type_signal" = void
#then
#	AC_DEFINE(VOID_SIGNALS)
#fi

AC_CHECK_FUNCS(strerror strtol lstat setrlimit sigrelse sighold sigaction \
sysconf setsid sigsetjmp)

#AC_CACHE_CHECK(for an abused getenv, es_cv_abused_getenv,
#AC_RUN_IFELSE([AC_LANG_SOURCE([[#
#changequote(,)
#char *foo = 0;
#char *foov[] = { "-a", "-bfoo", "bar" };
#int fooc = (sizeof(foov) / sizeof(foov[0]));
#
#int getenv() { return *foo; }
#int main() { while (-1 != getopt(fooc, foov, "ab:")); exit(0); }
#changequote([,])
#]])],[#es_cv_abused_getenv=no],[#es_cv_abused_getenv=yes
#AC_DEFINE(ABUSED_GETENV)],[]))
#
#if test "$es_cv_abused_getenv" = yes
#then
#	rm -f core conftest.core
#fi

dnl Check to see if you can assign to a va_list
AC_CACHE_CHECK(whether assignment to va_list ok?, es_cv_assign_va_list,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifndef HAVE_STDARG_H
choke me
#else
#include <stdarg.h>
#endif]], [[va_list first, second; first = second; return 0;]])],[es_cv_assign_va_list=yes],[es_cv_assign_va_list=no]))
if test "$es_cv_assign_va_list" = no
then
	AC_DEFINE(NO_VA_LIST_ASSIGN)
fi

dnl check for a u_quad_t or something like that
AC_CACHE_CHECK(for rlimit type ..., es_cv_rlimit_t,
[es_cv_rlimit_t=$(echo "#include <sys/resource.h>" | cpp | grep rlim_cur | sed -e 's/rlim_cur.*//' -e 's/^ //g' -e 's/^\t//g' -e q | tr -d "\r\n\t ")])

AC_DEFINE_UNQUOTED(LIMIT_T, $es_cv_rlimit_t)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

