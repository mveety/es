/* prim-sys.c -- system call primitives ($Revision: 1.2 $) */

#include <sys/resource.h>
#include <unistd.h>
#define	REQUIRE_IOCTL	1

#include "es.h"
#include "prim.h"

#ifdef HAVE_SETRLIMIT
# define BSD_LIMITS 1
#else
# define BSD_LIMITS 0
#endif

PRIM(newpgrp) {
	int pid;
	if (list != NULL)
		fail("$&newpgrp", "usage: newpgrp");
	pid = getpid();
	setpgrp(pid, pid);
#ifdef TIOCSPGRP
	{
		Sigeffect sigtstp = esignal(SIGTSTP, sig_ignore);
		Sigeffect sigttin = esignal(SIGTTIN, sig_ignore);
		Sigeffect sigttou = esignal(SIGTTOU, sig_ignore);
		ioctl(2, TIOCSPGRP, &pid);
		esignal(SIGTSTP, sigtstp);
		esignal(SIGTTIN, sigttin);
		esignal(SIGTTOU, sigttou);
	}
#endif
	return list_true;
}

PRIM(background) {
	int pid = efork(TRUE, TRUE);
	if (pid == 0) {
#if JOB_PROTECT
		/* job control safe version: put it in a new pgroup. */
		setpgrp(0, getpid());
#endif
		mvfd(eopen("/dev/null", oOpen), 0);
		exit(exitstatus(eval(list, NULL, evalflags | eval_inchild)));
	}
	return mklist(mkstr(str("%d", pid)), NULL);
}

PRIM(fork) {
	int pid, status;
	pid = efork(TRUE, FALSE);
	if (pid == 0)
		exit(exitstatus(eval(list, NULL, evalflags | eval_inchild)));
	status = ewaitfor(pid);
	SIGCHK();
	printstatus(0, status);
	return mklist(mkstr(mkstatus(status)), NULL);
}

PRIM(run) {
	char *file;
	if (list == NULL)
		fail("$&run", "usage: %%run file argv0 argv1 ...");
	Ref(List *, lp, list);
	file = getstr(lp->term);
	lp = forkexec(file, lp->next, (evalflags & eval_inchild) != 0);
	RefReturn(lp);
}

PRIM(umask) {
	if (list == NULL) {
		int mask = umask(0);
		umask(mask);
		print("%04o\n", mask);
		return list_true;
	}
	if (list->next == NULL) {
		int mask;
		char *s, *t;
		s = getstr(list->term);
		mask = strtol(s, &t, 8);
		if ((t != NULL && *t != '\0') || ((unsigned) mask) > 07777)
			fail("$&umask", "bad umask: %s", s);
		umask(mask);
		return list_true;
	}
	fail("$&umask", "usage: umask [mask]");
	NOTREACHED;
	return NULL;
}

PRIM(cd) {
	char *dir;
	if (list == NULL || list->next != NULL)
		fail("$&cd", "usage: $&cd directory");
	dir = getstr(list->term);
	if (chdir(dir) == -1)
		fail("$&cd", "chdir %s: %s", dir, esstrerror(errno));
	return list_true;
}

PRIM(setsignals) {
	int i;
	Sigeffect effects[NSIG];
	for (i = 0; i < NSIG; i++)
		effects[i] = sig_default;
	Ref(List *, lp, list);
	for (; lp != NULL; lp = lp->next) {
		int sig;
		const char *s = getstr(lp->term);
		Sigeffect effect = sig_catch;
		switch (*s) {
		case '-':	effect = sig_ignore;	s++; break;
		case '/':	effect = sig_noop;	s++; break;
		case '.':	effect = sig_special;	s++; break;
		}
		sig = signumber(s);
		if (sig < 0)
			fail("$&setsignals", "unknown signal: %s", s);
		effects[sig] = effect;
	}
	RefEnd(lp);
	blocksignals();
	setsigeffects(effects);
	unblocksignals();
	return mksiglist();
}

/*
 * limit builtin -- this is too much code for what it gives you
 */

#if BSD_LIMITS
typedef struct Suffix Suffix;
struct Suffix {
	const char *name;
	long amount;
	const Suffix *next;
};

static const Suffix sizesuf[] = {
	{ "g",	1024*1024*1024,	sizesuf + 1 },
	{ "m",	1024*1024,	sizesuf + 2 },
	{ "k",	1024,		NULL },
};

static const Suffix timesuf[] = {
	{ "h",	60 * 60,	timesuf + 1 },
	{ "m",	60,		timesuf + 2 },
	{ "s",	1,		NULL },
};

typedef struct {
	char *name;
	int flag;
	const Suffix *suffix;
} Limit;

static const Limit limits[] = {

#ifdef RLIMIT_AS
	{"addrspace", RLIMIT_AS, NULL },
#endif

#ifdef RLIMIT_CPU
	{ "cputime", RLIMIT_CPU, timesuf },
#endif

#ifdef RLIMIT_FSIZE
	{ "filesize", RLIMIT_FSIZE, sizesuf },
#endif

#ifdef RLIMIT_DATA
	{ "datasize", RLIMIT_DATA, sizesuf },
#endif

#ifdef RLIMIT_STACK
	{ "stacksize", RLIMIT_STACK, sizesuf },
#endif

#ifdef RLIMIT_CORE
	{ "coredumpsize", RLIMIT_CORE, sizesuf },
#endif

#ifdef RLIMIT_RSS	/* SysVr4 does not have this */
	{ "memoryuse", RLIMIT_RSS, sizesuf },
#endif
#ifdef RLIMIT_VMEM	/* instead, they have this! */
	{ "memorysize", RLIMIT_VMEM, sizesuf },
#endif

#ifdef RLIMIT_MEMLOCK	/* 4.4bsd adds an unimplemented limit on non-pageable memory */
	{ "lockedmemory", RLIMIT_CORE, sizesuf },
#endif

#ifdef RLIMIT_NOFILE	/* SunOS 4.1 adds a limit on file descriptors */
	{ "descriptors", RLIMIT_NOFILE, NULL },
#endif

#ifdef RLIMIT_NPROC	/* 4.4bsd adds a limit on child processes */
	{ "processes", RLIMIT_NPROC, NULL },
#endif

#ifdef RLIMIT_KQUEUES
	{ "kqueues", RLIMIT_KQUEUES, NULL },
#endif

#ifdef RLIMIT_NPTS
	{ "npts", RLIMIT_NPTS, NULL },
#endif

#ifdef RLIMIT_PIPEBUF
	{ "pipebuf", RLIMIT_PIPEBUF, sizesuf },
#endif

#ifdef RLIMIT_SBSIZE
	{ "sbsize", RLIMIT_SBSIZE, sizesuf },
#endif

#ifdef RLIMIT_SWAP
	{ "swap", RLIMIT_SWAP, sizesuf },
#endif

#ifdef RLIMIT_UMTXP
	{ "umtxp", RLIMIT_UMTXP, NULL },
#endif

#ifdef RLIMIT_VMEM
	{ "vmem", RLIMIT_VMEM, NULL },
#endif

#ifdef RLIMIT_LOCKS
	{ "locks", RLIMIT_LOCKS, NULL },
#endif

#ifdef RLIMIT_MSGQUEUE
	{ "msgqueue", RLIMIT_MSGQUEUE, sizesuf },
#endif

#ifdef RLIMIT_NICE
	{ "nice", RLIMIT_NICE, NULL },
#endif

#ifdef RLIMIT_RTPRIO
	{ "rtprio", RLIMIT_RTPRIO, NULL },
#endif

#ifdef RLIMT_RTTIME
	{ "rttime", RLIMIT_RTTIME, NULL },
#endif

#ifdef RLIMIT_SIGPENDING
	{ "sigpending", RLIMIT_SIGPENDING, NULL },
#endif

#ifdef RLIMIT_POSIXLOCKS
	{ "posixlocks", RLIMIT_POSIXLOCKS, NULL },
#endif

#ifdef RLIMIT_NTHR
	{ "nthr", RLIMIT_NTHR, NULL },
#endif

	{ NULL, 0, NULL }
};

struct Limret {
	Boolean unlimited;
	size_t size;
};
typedef struct Limret Limret;

static Limret
printlimit(const Limit *limit, Boolean hard, int retlimit) {
	struct rlimit rlim;
	LIMIT_T lim;
	Limret limret = {FALSE, 0};

	getrlimit(limit->flag, &rlim);
	if (hard)
		lim = rlim.rlim_max;
	else
		lim = rlim.rlim_cur;
	if(retlimit){
		if(lim == (LIMIT_T) RLIM_INFINITY)
			limret.unlimited = TRUE;
		else {
			limret.unlimited = FALSE;
			limret.size = lim;
		}
		return limret;
	}
	if (lim == (LIMIT_T) RLIM_INFINITY)
		print("%-8s\tunlimited\n", limit->name);
	else {
		const Suffix *suf;

		for (suf = limit->suffix; suf != NULL; suf = suf->next)
			if (lim % suf->amount == 0 && (lim != 0 || suf->amount > 1)) {
				lim /= suf->amount;
				break;
			}
		print("%-8s\t%d%s\n", limit->name, (int)lim, (suf == NULL || lim == 0) ? "" : suf->name);
	}
	return limret;
}

static long
parselimit(const Limit *limit, char *s) {
	long lim;
	char *t;
	const Suffix *suf = limit->suffix;
	if (streq(s, "unlimited"))
		return RLIM_INFINITY;

	if (!isdigit(*s))
		fail("$&limit", "%s: bad limit value", s);

	if (suf == timesuf && (t = strchr(s, ':')) != NULL) {
		char *u;
		lim = strtol(s, &u, 0) * 60;
		if (u != t)
			fail("$&limit", "%s %s: bad limit value", limit->name, s);
		lim += strtol(u + 1, &t, 0);
		if (t != NULL && *t == ':')
			lim = lim * 60 + strtol(t + 1, &t, 0);
		if (t != NULL && *t != '\0')
			fail("$&limit", "%s %s: bad limit value", limit->name, s);
	} else {
		lim = strtol(s, &t, 0);
		if (t != NULL && *t != '\0')
			for (;; suf = suf->next) {
				if (suf == NULL)
					fail("$&limit", "%s %s: bad limit value", limit->name, s);
				if (streq(suf->name, t)) {
					lim *= suf->amount;
					break;
				}
			}
	}
	return lim;
}

PRIM(limit) {
	const Limit *lim = limits;
	Boolean hard = FALSE;
	Boolean returnlimit = FALSE;
	Limret limit;
	Ref(List *, lp, list);

	if (lp != NULL && streq(getstr(lp->term), "-h")) {
		hard = TRUE;
		lp = lp->next;
	}

	if (lp == NULL)
		for (; lim->name != NULL; lim++)
			printlimit(lim, hard, 0);
	else {
		char *name = getstr(lp->term);
		if(streq(name, "-r")){
			if(lp->next == NULL)
				fail("$&limit", "missing limit");
			name = getstr(lp->next->term);
			returnlimit = TRUE;
			lp = lp->next;
		}

		for (;; lim++) {
			if (lim->name == NULL)
				fail("$&limit", "%s: no such limit", name);
			if (streq(name, lim->name))
				break;
		}
		lp = lp->next;
		if (lp == NULL)
			if(returnlimit)
				limit = printlimit(lim, hard, 1);
			else
				printlimit(lim, hard, 0);
		else {
			long n;
			struct rlimit rlim;
			getrlimit(lim->flag, &rlim);
			if ((n = parselimit(lim, getstr(lp->term))) < 0)
				fail("$&limit", "%s: bad limit value", getstr(lp->term));
			if (hard)
				rlim.rlim_max = (LIMIT_T)n;
			else
				rlim.rlim_cur = (LIMIT_T)n;
			if (setrlimit(lim->flag, &rlim) == -1)
				fail("$&limit", "setrlimit: %s", esstrerror(errno));
		}
	}
	RefEnd(lp);
	if(returnlimit == TRUE){
		if(limit.unlimited == TRUE)
			return mklist(mkstr("unlimited"), NULL);
		else
			return mklist(mkstr(str("%uld", limit.size)), NULL);
	}
	return list_true;
}
#endif	/* BSD_LIMITS */

#if BUILTIN_TIME
PRIM(time) {
	int pid, status;
	struct rusage r;
	struct timespec before, after;
	WaitStatus s;

	Ref(List *, lp, list);

	gc();	/* do a garbage collection first to ensure reproducible results */
	clock_gettime(CLOCK_MONOTONIC, &before);
	pid = efork(TRUE, FALSE);
	if (pid == 0)
		exit(exitstatus(eval(lp, NULL, evalflags | eval_inchild)));
	s = ewait(pid, FALSE, &r);
	status = s.status;
	clock_gettime(CLOCK_MONOTONIC, &after);
	SIGCHK();
	printstatus(0, status);
	after.tv_sec -= before.tv_sec;
	after.tv_nsec -= before.tv_nsec;
	if(after.tv_nsec < 0)
		after.tv_sec--, after.tv_nsec += 1000000000;

	eprint(
		"    %5ld.%02ld real %5ld.%02ld user %5ld.%02ld sys\t%L\n",
		after.tv_sec, after.tv_nsec/1000000,
		r.ru_utime.tv_sec, (long) (r.ru_utime.tv_usec / 10000),
		r.ru_stime.tv_sec, (long) (r.ru_stime.tv_usec / 10000),
		lp, " "
	);

	RefEnd(lp);
	return mklist(mkstr(mkstatus(status)), NULL);
}
#endif	/* BUILTIN_TIME */

#if !KERNEL_POUNDBANG
PRIM(execfailure) {
	int fd, len, argc;
	char header[1024], *args[10], *s, *end, *file;

	gcdisable();
	if (list == NULL)
		fail("$&execfailure", "usage: %%exec-failure name argv");

	file = getstr(list->term);
	fd = eopen(file, oOpen);
	if (fd < 0) {
		gcenable();
		return NULL;
	}
	len = read(fd, header, sizeof header);
	close(fd);
	if (len <= 2 || header[0] != '#' || header[1] != '!') {
		gcenable();
		return NULL;
	}

	s = &header[2];
	end = &header[len];
	argc = 0;
	while (argc < arraysize(args) - 1) {
		int c;
		while ((c = *s) == ' ' || c == '\t')
			if (++s >= end) {
				gcenable();
				return NULL;
			}
		if (c == '\n' || c == '\r')
			break;
		args[argc++] = s;
		do
			if (++s >= end) {
				gcenable();
				return NULL;
			}
		while (s < end && (c = *s) != ' ' && c != '\t' && c != '\n' && c != '\r');
		*s++ = '\0';
		if (c == '\n' || c == '\r')
			break;
	}
	if (argc == 0) {
		gcenable();
		return NULL;
	}

	list = list->next;
	if (list != NULL)
		list = list->next;
	list = mklist(mkstr(file), list);
	while (argc != 0)
		list = mklist(mkstr(args[--argc]), list);

	Ref(List *, lp, list);
	gcenable();
	lp = eval(lp, NULL, eval_inchild);
	RefReturn(lp);
}
#endif /* !KERNEL_POUNDBANG */


PRIM(getpid) {
	List *res = NULL; Root r_res;

	gcref(&r_res, (void**)&res);

	res = mklist(mkstr(str("%d", getpid())), NULL);

	gcderef(&r_res, (void**)&res);

	return res;
}

extern Dict *initprims_sys(Dict *primdict) {
	X(newpgrp);
	X(background);
	X(umask);
	X(cd);
	X(fork);
	X(run);
	X(setsignals);
#if BSD_LIMITS
	X(limit);
#endif
#if BUILTIN_TIME
	X(time);
#endif
#if !KERNEL_POUNDBANG
	X(execfailure);
#endif /* !KERNEL_POUNDBANG */

	X(getpid);
	return primdict;
}
