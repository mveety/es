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
ENABLE_LOGINSHELL=false
sys_wait_w_forms=yes
assertions=yes
DEVFD_IMPL=devfd
DYNLIBS=no
CFILEOPTS='$(MAINCFILES)'
OFILEOPTS='$(MAINOFILES)'

AC_ARG_WITH(readline,
--with-readline		Use GNU Readline, use_readline=yes)
AC_ARG_WITH(loginshell,
--with-loginshell	Only load esrc if a login shell, ENABLE_LOGINSHELL=true)
AC_ARG_WITH(internalwaitforms,
--with-internalwaitforms	Do not use W forms of IFSIGNALED and friends from sys/wait.h, sys_wait_w_forms=no)
AC_ARG_WITH(noassert,
--with-noassert	Disable asserts, assertions=no)
AC_ARG_WITH(dynlibs, --with-dynlibs	Allow loading of dynamic libraries,
			DYNLIBS=yes)

AC_SUBST(ENABLE_LOGINSHELL)
AC_SUBST(DEVFD_IMPL)
AC_SUBST(CFILEOPTS)
AC_SUBST(OFILEOPTS)
AC_CANONICAL_HOST

dnl saved_CFLAGS="$CFLAGS"

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P
dnl AC_PROG_YACC
AC_ARG_VAR([YACC], [hopefully bison])
AC_CHECK_PROG([YACC], [bison -y], [bison -y], [no])
AS_IF([test "x$YACC" = "xno"], [AC_MSG_ERROR([bison not found])])

AC_CACHE_CHECK(whether $CC accepts -Wno-clobbered, es_cv_no_clobbered, 
[
echo "int main(int argc, char *argv[]) { return 0; }" > conftest.c
if $CC -Wno-clobbered -Werror conftest.c -o /dev/null >/dev/null 2>&1; then
	CFLAGS="$CFLAGS -Wno-clobbered"
	es_cv_no_clobbered="yes"
else
	es_cv_no_clobbered="no"
fi])

AC_CACHE_CHECK(for extra CFLAGS for $CC on $host_os, es_cv_ccextras,
			   [
				case "$host_os" in
					freebsd*)
						case "`basename ${CC}`" in
							clang* | cc)
								es_cv_ccextras="-I/usr/local/include"
								CFLAGS="$CFLAGS -I/usr/local/include"
								;;
							gcc1*)
								es_cv_ccextras="-Wl,-rpath=/usr/local/lib/${CC}"
								CFLAGS="$CFLAGS -Wl,-rpath=/usr/local/lib/${CC}"
								;;
							*)
								es_cv_ccextras="none needed"
								;;
						esac
						;;
					*)
						es_cv_ccextras="none needed"
						;;
				esac
				])

AC_CACHE_CHECK(for extra LDFLAGS for $CC on $host_os, es_cv_ldextras,
			   [
				case "$host_os" in
					freebsd*)
						case "`basename ${CC}`" in
							clang* | cc)
								es_cv_ldextras="-L/usr/local/lib"
								LDFLAGS="$LDFLAGS $es_cv_ldextras"
								;;
							gcc1*)
								es_cv_ldextras="-Wl,-rpath=/usr/local/lib/${CC}"
								LDFLAGS="-Wl,-rpath=/usr/local/lib/${CC}"
								;;
							*)
								es_cv_ldextras="none needed"
								;;
						esac
						;;
					*)
						es_cv_ldextras="none needed"
						;;
				esac
				])

dnl CFLAGS="$CFLAGS $saved_CFLAGS"

if test "$DYNLIBS" = "yes"
then
	CFILEOPTS='$(MAINCFILES) $(DYNCFILES)'
	OFILEOPTS='$(MAINOFILES) $(DYNOFILES)'
fi

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
AC_CHECK_INCLUDES_DEFAULT

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/time.h unistd.h memory.h stdarg.h sys/cdefs.h)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_GETGROUPS
AC_FUNC_MMAP

AC_CHECK_FUNCS(strerror strtol lstat setrlimit sigrelse sighold sigaction \
sysconf setsid sigsetjmp)

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

if test "$sys_wait_w_forms" = "yes"
then
	AC_DEFINE(USE_WAIT_W_FORMS)
fi

if test "$assertions" = "yes"
then
	AC_DEFINE(ASSERTIONS)
fi

if test "$DYNLIBS" = "yes"
then
	AC_DEFINE(DYNAMIC_LIBRARIES)
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

