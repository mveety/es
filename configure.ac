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


ENABLE_LOGINSHELL=false
sys_wait_w_forms=yes
assertions=yes
DEVFD_IMPL=devfd
DYNLIBS=false
CFILEOPTS='$(MAINCFILES)'
OFILEOPTS='$(MAINOFILES)'
MODULES=''
MODOFILES=''
development='false'
SANITIZER='none'
EDITORDEBUG=false
SYNTAXDEBUG=false

AC_ARG_ENABLE(loginshell,
[  --enable-loginshell	Only load esrc if a login shell],
  [if test "x$enableval" = "xyes"; then
		ENABLE_LOGINSHELL=true
	else
		ENABLE_LOGINSHELL=false
	fi], [ENABLE_LOGINSHELL=false])

AC_ARG_ENABLE(waitforms,
[  --enable-waitforms	Use W forms of IFSIGNALED and friends from sys/wait.h],
	[sys_wait_w_forms="$enableval"],[sys_wait_w_forms=yes])

AC_ARG_ENABLE(assert,
[  --enable-assert	Enable asserts (enabled by default)],
  [assertions="$enableval"], [assertions=yes])

AC_ARG_ENABLE(modules,
[  --enable-modules	Allow loading of modules],
  [ if test "x$enableval" = "xyes"; then
		DYNLIBS=true
	else
		DYNLIBS=false
	fi], [DYNLIBS=false])

AC_ARG_ENABLE(development,
[  --enable-development	Extras for development],
[CFLAGS="-g -O0 -DEditorDebug=1"; development=true])

AC_ARG_ENABLE(addrsan,
[  --enable-addrsan	Enable llvm address sanitizer],
[SANITIZER='asan'])

AC_ARG_ENABLE(memorysan,
[  --enable-memorysan	Enable llvm memory sanitizer],
[SANITIZER='msan'])

AC_ARG_ENABLE(ubsan,
[  --enable-ubsan	Enable llvm undefined behavior sanitizer],
[SANITIZER='ubsan'])

AC_ARG_ENABLE(editordebug,
[  --enable-editordebug	Enable editor debugging],
[EDITORDEBUG=true])

AC_ARG_ENABLE(syntaxdebug,
[  --enable-syntaxdebug	Enable mod_syntax debugging],
[SYNTAXDEBUG=true])

AC_SUBST(ENABLE_LOGINSHELL)
AC_SUBST(DEVFD_IMPL)
AC_SUBST(CFILEOPTS)
AC_SUBST(OFILEOPTS)
AC_SUBST(DYNLIBS)
AC_SUBST(MODULES)
AC_SUBST(MODOFILES)
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
								es_cv_ccextras="-Wno-stringop-truncation -Wl,-rpath=/usr/local/lib/${CC}"
								CFLAGS="$CFLAGS -Wno-stringop-truncation -Wl,-rpath=/usr/local/lib/${CC}"
								;;
							gcc)
								es_cv_ccextras="-Wno-stringop-truncation"
								CFLAGS="$CFLAGS -Wno-stringop-truncation"
								;;
							*)
								es_cv_ccextras="none needed"
								;;
						esac
						;;
					linux*)
						case "`basename ${CC}`" in
							gcc | gcc*)
dnl								es_cv_ccextras="-Wno-stringop-truncation -D_DEFAULT_SOURCE=1 -D_XOPEN_SOURCE=800"
dnl								CFLAGS="$CFLAGS -Wno-stringop-truncation -D_DEFAULT_SOURCE=1 -D_XOPEN_SOURCE=800"
								es_cv_ccextras="-Wno-stringop-truncation"
								CFLAGS="$CFLAGS -Wno-stringop-truncation"
								;;
							*)
								es_cv_ccextras="none needed"
								;;
						esac
						;;
					*)
						case "`basename ${CC}`" in
							gcc | gcc*)
								es_cv_ccextras="-Wno-stringop-truncation"
								CFLAGS="$CFLAGS -Wno-stringop-truncation"
								;;
							*)
								es_cv_ccextras="none needed"
								;;
						esac
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

AC_CACHE_CHECK(for extra LIBS on $host_os, es_cv_libs,
			   [
				case "$host_os" in
					haiku)
						LIBS="$LIBS -lbsd"
						es_cv_libs="-lbsd"
						;;
					*)
						es_cv_libs="none needed"
						;;
				esac
				])

dnl CFLAGS="$CFLAGS $saved_CFLAGS"

if test "$DYNLIBS" = "true"
then
	MODOFILES='$(MODBASE_OFILES)'
	MODULES='$(BASE_MODULES)'
	CFILEOPTS='$(MAINCFILES) $(DYNCFILES)'
	OFILEOPTS='$(MAINOFILES) $(DYNOFILES)'
	CFLAGS="$CFLAGS -fPIC"
	LDFLAGS="$LDFLAGS -rdynamic"
fi

if test "$EDITORDEBUG" = "true" && test "$development" = "false"
then
	CFLAGS="$CFLAGS -DEditorDebug=1"
fi

if test "$DYNLIBS" = "true" && test "$SYNTAXDEBUG" = "true"
then
	CFLAGS="$CFLAGS -DTokenizerDebug=1"
fi

if test "$development" = "true"
then
	case "$SANITIZER" in
		asan)
			case "`basename ${CC}`" in
				clang*)
					CFLAGS="-fsanitize=address -fno-omit-frame-pointer $CFLAGS"
					LDFLAGS="-fsanitize=address -fno-omit-frame-pointer $LDFLAGS"
					;;
				*)
					AC_MSG_ERROR([address sanitizer requires clang])
					;;
			esac
			;;
		msan)
			case "`basename ${CC}`" in
				clang*)
					CFLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer $CFLAGS"
					LDFLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer $LDFLAGS"
					;;
				*)
					AC_MSG_ERROR([memory sanitizer requires clang])
					;;
			esac
			;;
		ubsan)
			case "`basename ${CC}`" in
				clang*)
					CFLAGS="-fsanitize=undefined -fno-omit-frame-pointer $CFLAGS"
					LDFLAGS="-fsanitize=undefine -fno-omit-frame-pointer $LDFLAGS"
					;;
				*)
					AC_MSG_ERROR([undefined behavior sanitizer requires clang])
					;;
			esac
			;;
		none)
			;;
		*)
			AC_MSG_ERROR([unknown sanitizer $SANITIZER])
			;;
	esac
fi

dnl ----------------------------
dnl CHECK FOR /dev/fd FILESYSTEM
dnl ----------------------------
AC_CACHE_CHECK(for /dev/fd filesystem, es_cv_sys_dev_fd,
[test -d /dev/fd && es_cv_sys_dev_fd=yes || es_cv_sys_dev_fd=no])
if test $es_cv_sys_dev_fd = yes; then
	AC_DEFINE(HAVE_DEV_FD)
	DEVFD_IMPL="devfd"
else
	DEVFD_IMPL="mkfifo"
fi

AC_SYS_INTERPRETER
if test "$ac_cv_sys_interpreter" = yes
then
	AC_DEFINE(KERNEL_POUNDBANG)
fi


dnl Checks for libraries.

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
sysconf sigsetjmp setpgid)

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

m4_include([m4/pkg.m4])
PKG_CHECK_MODULES([PCRE2], [libpcre2-posix libpcre2-8],
	[AC_DEFINE([HAVE_PCRE2], [1], [Is pcre2 available])],
	[AC_MSG_ERROR([PCRE2 is required])])

if test "$DYNLIBS" = "true"
then
	PKG_CHECK_MODULES([CJSON], [libcjson libcjson_utils],
		[
		 MODOFILES="$MODOFILES \$(MODJSON_OFILES)"
		 MODULES="$MODULES \$(MODJSON)"
		],[es_no_cjson="true"])
fi

if test "$sys_wait_w_forms" = "yes"
then
	AC_DEFINE(USE_WAIT_W_FORMS)
fi

if test "$assertions" = "yes"
then
	AC_DEFINE(ASSERTIONS)
fi

if test "$DYNLIBS" = "true"
then
	AC_DEFINE(DYNAMIC_LIBRARIES)
fi

AC_CONFIG_FILES([config.mk Makefile GNUmakefile])
AC_OUTPUT

