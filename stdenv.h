/* stdenv.h -- set up an environment we can use ($Revision: 1.3 $) */

#ifndef __es_stdenv
#define __es_stdenv

#ifdef __linux__
#define _DEFAULT_SOURCE 1
#define _XOPEN_SOURCE 800
#endif

#include "esconfig.h"
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef HAVE_UNISTD_H
#error es-mveety requires unistd.h
#endif

/*
 * type qualifiers
 */

#if !USE_VOLATILE
#ifndef volatile
#define volatile
#endif
#endif

/*
 * protect the rest of es source from the dance of the includes
 */

#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>

#if BSD_LIMITS || BUILTIN_TIME
#include <sys/time.h>
#include <sys/resource.h>
#define PRIM_GETRUSAGE 1
#endif

/* we still need these includes for $&getrusage */
#if HAVE_SYS_TIME_H && !BSD_LIMITS && !BUILTIN_TIME
#include <sys/time.h>
#include <sys/resource.h>
#define PRIM_GETRUSAGE 1
#endif

#include <unistd.h>

#include <sys/param.h>

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#if HAVE_MEMORY_H
#include <memory.h>
#endif

#include <stdarg.h>

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>

#include <sys/types.h>

#include <sys/ioctl.h>

#include <dirent.h>
typedef struct dirent Dirent;

#include <pwd.h>

#include <fcntl.h>

#if defined(__sun) || defined(__illumos__)
#include <sys/loadavg.h>
#endif

/* stdlib */
#if __GNUC__
typedef volatile void noreturn;
#else
typedef void noreturn;
#endif

#include "result.h"
#include "editor.h"

/* for regexes */
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <pcre2posix.h>

/*
 * things that should be defined by header files but might not have been
 */

#ifndef offsetof
#define offsetof(t, m) ((size_t)(((char *)&((t *)0)->m) - (char *)0))
#endif

#ifndef EOF
#define EOF (-1)
#endif

/* setjmp */

#if defined sigsetjmp || HAVE_SIGSETJMP
/* under linux, sigsetjmp and setjmp are both macros
 * -- need to undef setjmp to avoid problems
 */
#ifdef setjmp
#undef setjmp
#endif
#define setjmp(buf) sigsetjmp(buf, 1)
#define longjmp(x, y) siglongjmp(x, y)
#define jmp_buf sigjmp_buf
#endif

/*
 * macros
 */

#define STMT(stmt) \
	do {           \
		stmt;      \
	} while(0)
#define NOP \
	do {    \
	} while(0)
#define CONCAT(a, b) a##b
#define STRING(s) #s
#define streq(s, t) (strcmp(s, t) == 0)
#define strneq(s, t, n) (strncmp(s, t, n) == 0)
#define hasprefix(s, p) strneq(s, p, (sizeof p) - 1)
#define arraysize(a) ((int)(sizeof(a) / sizeof(*a)))
#define memzero(dest, count) memset(dest, 0, count)
#define atoi(s) strtol(s, NULL, 0)

/*
 * types we use throughout es
 */

#undef FALSE
#undef TRUE
typedef enum { FALSE = 0, TRUE = 1 } Boolean;

#if USE_SIG_ATOMIC_T
typedef volatile sig_atomic_t Atomic;
#else
typedef volatile int Atomic;
#endif

typedef void Sigresult;

typedef GETGROUPS_T gidset_t;

/*
 * assertion checking
 */

#if ASSERTIONS
#define assert(expr)                                                                    \
	STMT(if(!(expr)) {                                                                  \
		dprintf(2, "%s:%d: assertion failed (%s)\n", __FILE__, __LINE__, STRING(expr)); \
		abort();                                                                        \
	})
#else
#define assert(ignore) NOP
#endif

enum { UNREACHABLE = 0 };

#define NOTREACHED STMT(assert(UNREACHABLE))

/*
 * hacks to present a standard system call interface
 */

/*
 * macros for picking apart statuses
 *	we should be able to use the W* forms from <sys/wait.h> but on
 *	some machines they take a union wait (what a bad idea!) and on
 *	others an integer.  we just renamed the first letter to s and
 *	let things be.  on some systems these could just be defined in
 *	terms of the W* forms.
 */

#ifndef USE_WAIT_W_FORMS
#define SIFSIGNALED(status) (((status) & 0xff) != 0)
#define STERMSIG(status) ((status) & 0x7f)
#define SCOREDUMP(status) ((status) & 0x80)
#define SIFEXITED(status) (!SIFSIGNALED(status))
#define SEXITSTATUS(status) (((status) >> 8) & 0xff)
#else
#define SIFSIGNALED(status) WIFSIGNALED(status)
#define STERMSIG(status) WTERMSIG(status)
#define SCOREDUMP(status) WCOREDUMP(status)
#define SIFEXITED(status) WIFEXITED(status)
#define SEXITSTATUS(status) WEXITSTATUS(status)
#endif

#ifndef HAVE_SETPGID
#error es-mveety requires setpgid
#endif

#endif
