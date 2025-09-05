/* stdenv.h -- set up an environment we can use ($Revision: 1.3 $) */

#ifndef __es_stdenv
#define __es_stdenv

#include "esconfig.h"
#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h>
#endif

/*
 * type qualifiers
 */

#if !USE_VOLATILE
# ifndef volatile
#  define volatile
# endif
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
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if REQUIRE_PARAM
#include <sys/param.h>
#endif

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

/* #if REQUIRE_STAT || REQUIRE_IOCTL */
/* We need sys/types.h for the prototype of gid_t on Linux */
#include <sys/types.h>
/* #endif */

#if REQUIRE_IOCTL
#include <sys/ioctl.h>
#endif

#include <dirent.h>
typedef struct dirent Dirent;


#if REQUIRE_PWD
#include <pwd.h>
#endif

#if REQUIRE_FCNTL
#include <fcntl.h>
#endif

/* stdlib */
#if __GNUC__
typedef volatile void noreturn;
#else
typedef void noreturn;
#endif


#if READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* for regexes */
#include <regex.h>

/*
 * things that should be defined by header files but might not have been
 */

#ifndef	offsetof
#define	offsetof(t, m)	((size_t) (((char *) &((t *) 0)->m) - (char *)0))
#endif

#ifndef	EOF
#define	EOF	(-1)
#endif

/* setjmp */

#if defined sigsetjmp || HAVE_SIGSETJMP
/* under linux, sigsetjmp and setjmp are both macros 
 * -- need to undef setjmp to avoid problems
 */
# ifdef setjmp
#  undef setjmp
# endif
# define setjmp(buf) sigsetjmp(buf,1)
# define longjmp(x,y)     siglongjmp(x,y)
# define jmp_buf     sigjmp_buf
#endif


/*
 * macros
 */

#define STMT(stmt) do { stmt; } while (0)
#define NOP do {} while (0)
#define CONCAT(a,b) a ## b
#define STRING(s) #s
#define	streq(s, t)		(strcmp(s, t) == 0)
#define	strneq(s, t, n)		(strncmp(s, t, n) == 0)
#define	hasprefix(s, p)		strneq(s, p, (sizeof p) - 1)
#define	arraysize(a)		((int) (sizeof (a) / sizeof (*a)))
#define	memzero(dest, count)	memset(dest, 0, count)
#define	atoi(s)			strtol(s, NULL, 0)

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
 * variable argument lists
 */

/* we only support systems that have stdarg.h */
/*
#if HAVE_STDARG_H

#define	VARARGS				, ...
#define	VARARGS1(t1, v1)		(t1 v1, ...)
#define	VARARGS2(t1, v1, t2, v2)	(t1 v1, t2 v2, ...)
#define	VA_START(ap, v)			va_start(ap, v)

#else	* !HAVE_STDARG_H *

#define	VARARGS
#define	VARARGS1(t1, v1)		(v1, va_alist) t1 v1; va_dcl
#define	VARARGS2(t1, v1, t2, v2)	(v1, v2, va_alist) t1 v1; t2 v2; va_dcl
#define	VA_START(ap, var)		va_start(ap)

#endif
*/

/*
 * assertion checking
 */

#if ASSERTIONS
#define	assert(expr) \
	STMT( \
		if (!(expr)) { \
			dprintf(2, "%s:%d: assertion failed (%s)\n", \
				__FILE__, __LINE__, STRING(expr)); \
			abort(); \
		} \
	)
#else
#define	assert(ignore)	NOP
#endif

enum { UNREACHABLE = 0 };


#define	NOTREACHED	STMT(assert(UNREACHABLE))

/*
 * system calls -- can we get these from some standard header uniformly?
 */

#if !HAVE_UNISTD_H
extern int chdir(const char *dirname);
extern int close(int fd);
extern int dup(int fd);
extern int dup2(int srcfd, int dstfd);
extern int execve(char *name, char **argv, char **envp);
extern int fork(void);
extern int getegid(void);
extern int geteuid(void);
extern int getpagesize(void);
extern int getpid(void);
extern int pipe(int p[2]);
extern int read(int fd, void *buf, size_t n);
extern int setpgrp(int pid, int pgrp);
extern int umask(int mask);
extern int write(int fd, const void *buf, size_t n);

#if REQUIRE_IOCTL
extern int ioctl(int fd, int cmd, void *arg);
#endif

#if REQUIRE_STAT
extern int stat(const char *, struct stat *);
#endif

#ifdef NGROUPS
extern int getgroups(int, int *);
#endif
#endif	/* !HAVE_UNISTD_H */


/*
 * hacks to present a standard system call interface
 */

/* basically all platforms I want to support have the appropriate
 * interface, so this logic isn't really needed.
#ifdef HAVE_SETSID
# define setpgrp(a, b)	setsid()
#else
#if defined(linux) || defined(__GLIBC__)
#include "unistd.h"
#define setpgrp(a, b)	setpgid(a, b)
#endif

#if sgi
#define	setpgrp(a, b)	BSDsetpgrp(a,b)
#endif

#if HPUX
#define	setpgrp(a, b)	setpgrp()
#endif
#endif

#if !HAVE_LSTAT
#define	lstat	stat
#endif
*/

#if defined(linux) || defined(__GLIBC__)
#define setpgrp(a, b) setpgid(a, b)
#endif

/*
 * macros for picking apart statuses
 *	we should be able to use the W* forms from <sys/wait.h> but on
 *	some machines they take a union wait (what a bad idea!) and on
 *	others an integer.  we just renamed the first letter to s and
 *	let things be.  on some systems these could just be defined in
 *	terms of the W* forms.
 */

#ifndef USE_WAIT_W_FORMS
#define	SIFSIGNALED(status)	(((status) & 0xff) != 0)
#define	STERMSIG(status)	((status) & 0x7f)
#define	SCOREDUMP(status)	((status) & 0x80)
#define	SIFEXITED(status)	(!SIFSIGNALED(status))
#define	SEXITSTATUS(status)	(((status) >> 8) & 0xff)
#else
#define	SIFSIGNALED(status)	WIFSIGNALED(status)
#define	STERMSIG(status)	WTERMSIG(status)
#define	SCOREDUMP(status)	WCOREDUMP(status)
#define	SIFEXITED(status)	WIFEXITED(status)
#define	SEXITSTATUS(status)	WEXITSTATUS(status)
#endif

#endif

