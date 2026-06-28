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
BOUNDSSAFETY=false
debugging='true'
automated_crashing='false'

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
[	if test "x$enableval" = "xyes"; then
		CFLAGS="-g -O0"
		development=true
		EDITORDEBUG=true
	fi
])

AC_ARG_ENABLE(addrsan,
[  --enable-addrsan	Enable llvm address sanitizer],
[	if test "x$enableval" = "xyes"; then
		SANITIZER='asan'
	else
		SANITIZER='none'
	fi
])

AC_ARG_ENABLE(memorysan,
[  --enable-memorysan	Enable llvm memory sanitizer],
[	if test "x$enableval" = "xyes"; then
		SANITIZER='msan'
	else
		SANITIZER='none'
	fi
])

AC_ARG_ENABLE(debugging,
[  --disable-debugging	Disable es built-in debugging],
[	if test "x$enableval" = "xyes"; then
		debugging='true'
	else
		debugging='false'
	fi
])

AC_ARG_ENABLE(automated-crashing,
[  --enable-automated-crashing	Give some primitives the option of crashing],
[	if test "x$enableval" = "xyes"; then
		automated_crashing='true'
	else
		automated_crashing='false'
	fi
])


dnl AC_ARG_ENABLE(ubsan,
dnl [  --enable-ubsan	Enable llvm undefined behavior sanitizer],
dnl [	if test "x$enableval" = "xyes"; then
dnl 		SANITIZER='ubsan'
dnl 	else
dnl 		SANITIZER='none'
dnl 	fi
dnl ])

AC_ARG_ENABLE(syntaxdebug,
[  --enable-syntaxdebug	Enable mod_syntax debugging],
[	if test "x$enableval" = "xyes"; then
		SYNTAXDEBUG=true
	else
		SYNTAXDEBUG=false
	fi
])

AC_ARG_ENABLE(editordebug,
[  --enable-editordebug	Enable line editor debugging output],
[	if test "x$enableval" = "xyes"; then
		EDITORDEBUG=true
	else
		EDITORDEBUG=false
	fi
])

AC_ARG_ENABLE(bounds-safety,
[  --enable-bounds-safety	Enable experimental clang -fbounds-safety],
[	if test "x$enableval" = "xyes"; then
		BOUNDSSAFETY=true
	else
		BOUNDSSAFETY=false
	fi
])

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
AC_CHECK_PROG([YACC], [bison -y], [bison -y], [no])
AS_IF([test "x$YACC" = "xno"], [AC_MSG_ERROR([bison not found])])

AC_CACHE_CHECK(for compiler dist name, es_cv_cctype, [
case "${CC}" in
	clang*)
		es_cv_cctype='clang'
		;;
	gcc*)
		es_cv_cctype='gcc'
		;;
	cc*)
		case "`cc --version | head -n 1`" in
			*clang*)
				es_cv_cctype='clang'
				;;
			*gcc*)
				es_cv_cctype='gcc'
				;;
			*)
				AC_MSG_ERROR([es only supports clang and gcc])
				;;
		esac
		;;
esac
])

AC_CACHE_CHECK($es_cv_cctype version, es_cv_ccversion, [
case "$es_cv_cctype" in
	clang*)
		es_cv_ccversion="`${CC} --version | head -n 1 | grep -oE '[[0-9.]]+' | head -n 1`"
		;;
	gcc*)
		es_cv_ccversion="`${CC} -dumpfullversion`"
		;;
esac
])

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
						case "$es_cv_cctype" in
							clang)
								es_cv_ccextras="-I/usr/local/include"
								CFLAGS="$CFLAGS -I/usr/local/include"
								;;
							gcc)
								es_cv_ccextras="-Wno-stringop-truncation"
								CFLAGS="$CFLAGS -Wno-stringop-truncation"
								case "`basename ${CC}`" in
									gcc1*)	
										es_cv_ccextras="$es_cv_ccextras -Wl,-rpath=/usr/local/lib/${CC}"
										CFLAGS="$CFLAGS -Wl,-rpath=/usr/local/lib/${CC}"
										;;
								esac
								;;
						esac
						;;
					linux*)
						case "$es_cv_cctype" in
							gcc)
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
					netbsd*)
						case "$es_cv_cctype" in
							gcc)
								case `basename "${CC%/*/*}"` in
									gcc13 | gcc14 | gcc15)
										es_cv_ccextras="-I/usr/pkg/include -Wno-stringop-truncation -Wno-char-subscripts"
										CFLAGS="$CFLAGS -I/usr/pkg/include -Wno-stringop-truncation -Wno-char-subscripts"
										;;
									*)
										es_cv_ccextras=`basename "${CC%/*/*}"`
										AC_MSG_ERROR([($es_cv_ccextras) is unsupported])
										;;
								esac
								;;
							clang)
								es_cv_ccextras="-I/usr/pkg/include -Wno-char-subscripts"
								CFLAGS="$CFLAGS -I/usr/pkg/include -Wno-char-subscripts"
								;;
						esac
						;;
					*)
						case "$es_cv_cctype" in
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
					netbsd*)
						case "$es_cv_cctype" in
							gcc)
								case `basename "${CC%/*/*}"` in
									gcc13 | gcc14 | gcc15)
										es_cv_ldextras="-L/usr/pkg/`basename ${CC%/*/*}`/lib"
										CFLAGS="$CFLAGS -L/usr/pkg/`basename ${CC%/*/*}`/lib"
										;;
									*)
										es_cv_ldextras=`basename "${path%/*/*}"`
										AC_MSG_ERROR([($es_cv_ldextras) is unsupported])
										;;
								esac
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

if test "$EDITORDEBUG" = "true"
then
	CFLAGS="$CFLAGS -DEditorDebug=1"
fi

if test "$DYNLIBS" = "true" && test "$SYNTAXDEBUG" = "true"
then
	CFLAGS="$CFLAGS -DTokenizerDebug=1"
fi

if test "$development" = "true"
then
dnl force debugging on if --enable-development
	debugging='true'
	if test "$BOUNDSSAFETY" = "true"
	then
		case "$es_cv_cctype" in
			clang)
				case "$es_cv_ccversion" in
					20.* | 21.* | 22.*)
						CFLAGS="-Xclang -fexperimental-bounds-safety $CFLAGS"
						;;
					*)
						AC_MSG_ERROR([-fbounds-safety requires clang 20+])
						;;
				esac
				;;
			*)
				AC_MSG_ERROR([-fbounds-safety requires clang 20+])
				;;
		esac
	fi

	case "$SANITIZER" in
		asan)
			case "$es_cv_cctype" in
				clang)
					CFLAGS="-fsanitize=address -fno-omit-frame-pointer $CFLAGS"
					LDFLAGS="-fsanitize=address -fno-omit-frame-pointer $LDFLAGS"
					;;
				*)
					AC_MSG_ERROR([address sanitizer requires clang])
					;;
			esac
			;;
		msan)
			case "$es_cv_cctype" in
				clang)
					CFLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer $CFLAGS"
					LDFLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer $LDFLAGS"
					;;
				*)
					AC_MSG_ERROR([memory sanitizer requires clang])
					;;
			esac
			;;
		ubsan)
			case "$es_cv_cctype" in
				clang)
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
sysconf sigsetjmp setpgid getloadavg)

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

if test "$debugging" = "true"
then
	AC_DEFINE(DEBUGGING)
fi

if test "$automated_crashing" = "true"
then
	AC_DEFINE(AUTOMATED_CRASHING)
fi

AC_CONFIG_FILES([config.mk Makefile GNUmakefile])
AC_OUTPUT

