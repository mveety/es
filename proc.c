/* proc.c -- process control system calls ($Revision: 1.2 $) */

#include "es.h"

/* TODO: the rusage code for the time builtin really needs to be cleaned up */

#include <sys/time.h>
#include <sys/resource.h>

Boolean hasforked = FALSE;

typedef struct Proc Proc;
struct Proc {
	int pid;
	int status;
	Boolean alive, background;
	Proc *next, *prev;
	struct rusage rusage;
};


static Proc *proclist = NULL;

/* mkproc -- create a Proc structure */
extern Proc *mkproc(int pid, Boolean background) {
	Proc *proc;
	for (proc = proclist; proc != NULL; proc = proc->next)
		if (proc->pid == pid) {		/* are we recycling pids? */
			assert(!proc->alive);	/* if false, violates unix semantics */
			break;
		}
	if (proc == NULL) {
		proc = ealloc(sizeof (Proc));
		proc->next = proclist;
	}
	proc->pid = pid;
	proc->alive = TRUE;
	proc->background = background;
	proc->prev = NULL;
	return proc;
}

/* efork -- fork (if necessary) and clean up as appropriate */
extern int efork(Boolean parent, Boolean background) {
	if (parent) {
		int pid = fork();
		switch (pid) {
		default: {	/* parent */
			Proc *proc = mkproc(pid, background);
			if (proclist != NULL)
				proclist->prev = proc;
			proclist = proc;
			return pid;
		}
		case 0:		/* child */
			proclist = NULL;
			hasforked = TRUE;
			break;
		case -1:
			fail("es:efork", "fork: %s", esstrerror(errno));
		}
	}
	closefds();
	setsigdefaults();
	newchildcatcher();
	return 0;
}

static struct rusage wait_rusage;

/* dowait -- a wait wrapper that interfaces with signals */
static int dowait(int *statusp) {
	int n;
	interrupted = FALSE;

	if (!setjmp(slowlabel)) {
		slow = TRUE;
		if(interrupted)
			n = -2;
		else {
			/* on freebsd this maybe should be WEXITED|WTRAPPED */
			n = waitpid(-1, (void*) statusp, 0);
			getrusage(RUSAGE_CHILDREN, &wait_rusage);
		}
	} else
		n = -2;
	slow = FALSE;
	if (n == -2) {
		errno = EINTR;
		n = -1;
	}
	return n;
}

/* reap -- mark a process as dead and attach its exit status */
static void reap(int pid, int status) {
	Proc *proc;
	for (proc = proclist; proc != NULL; proc = proc->next)
		if (proc->pid == pid) {
			assert(proc->alive);
			proc->alive = FALSE;
			proc->status = status;
			proc->rusage = wait_rusage;
			return;
		}
}

/* ewait -- wait for a specific process to die, or any process if pid == 0 */
WaitStatus ewait1(int pid, Boolean interruptible, void *rusage, Boolean print) {
	Proc *proc;
	WaitStatus s;
top:
	for (proc = proclist; proc != NULL; proc = proc->next)
		if (proc->pid == pid || (pid == 0 && !proc->alive)) {
			int status;
			if (proc->alive) {
				int deadpid;
				int seen_eintr = FALSE;
				while ((deadpid = dowait(&proc->status)) != pid)
					if (deadpid != -1)
						reap(deadpid, proc->status);
					else if (errno == EINTR) {
						if (interruptible)
							SIGCHK();
						seen_eintr = TRUE;
					} else if (errno == ECHILD && seen_eintr)
						/* TODO: not clear on why this is necessary
						 * (child procs _sometimes_ disappear after SIGINT) */
						break;
					else
						fail("es:ewait", "wait: %s", esstrerror(errno));
				proc->alive = FALSE;
				proc->rusage = wait_rusage;
			}
			if (proc->next != NULL)
				proc->next->prev = proc->prev;
			if (proc->prev != NULL)
				proc->prev->next = proc->next;
			else
				proclist = proc->next;
			status = proc->status;
			if (proc->background && print)
				printstatus(proc->pid, status);
			s.pid = proc->pid;
			s.status = status;
			if (rusage != NULL)
				memcpy(rusage, &proc->rusage, sizeof (struct rusage));
			efree(proc);
			return s;
		}
	if (pid == 0) {
		int status;
		while ((pid = dowait(&status)) == -1) {
			if (errno != EINTR)
				fail("es:ewait", "wait: %s", esstrerror(errno));
			if (interruptible)
				SIGCHK();
		}
		reap(pid, status);
		goto top;
	}
	fail("es:ewait", "wait: %d is not a child of this shell", pid);
	NOTREACHED;
}

extern WaitStatus
ewait(int pid, Boolean interruptable, void *rusage)
{
	return ewait1(pid, interruptable, rusage, TRUE);
}

extern int
ewaitfor(int pid)
{
	WaitStatus s;

	s = ewait1(pid, FALSE, NULL, TRUE);
	return s.status;
}

#include "prim.h"

PRIM(apids) {
	Proc *p;
	Ref(List *, lp, NULL);
	for (p = proclist; p != NULL; p = p->next)
		if (p->background && p->alive) {
			Term *t = mkstr(str("%d", p->pid));
			lp = mklist(t, lp);
		}
	/* TODO: sort the return value, but by number? */
	RefReturn(lp);
}

PRIM(wait) {
	int pid;
	Boolean print = TRUE;
	Boolean onlystatus = FALSE;
	WaitStatus s;
	if (list == NULL)
		pid = 0;
	else if (list->next == NULL) {
		pid = atoi(getstr(list->term));
		print = FALSE;
		if (pid < 0) {
			fail("$&wait", "wait: %d: bad pid", pid);
			NOTREACHED;
		}
		if (pid != 0)
			onlystatus = TRUE;
	} else {
		fail("$&wait", "usage: wait [pid]");
		NOTREACHED;
	}
	s = ewait1(pid, TRUE, NULL, print);
	if(onlystatus)
		return mklist(mkstr(mkstatus(s.status)), NULL);
	return mklist(mkstr(str("%d", s.pid)), mklist(mkstr(mkstatus(s.status)), NULL));
}

extern Dict *initprims_proc(Dict *primdict) {
	X(apids);
	X(wait);
	return primdict;
}

