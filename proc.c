/* proc.c -- process control system calls ($Revision: 1.2 $) */

#include "es.h"

/* TODO: the rusage code for the time builtin really needs to be cleaned up */

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
extern Proc *
mkproc(int pid, Boolean background)
{
	Proc *proc;
	for(proc = proclist; proc != NULL; proc = proc->next)
		if(proc->pid == pid) {	  /* are we recycling pids? */
			assert(!proc->alive); /* if false, violates unix semantics */
			break;
		}
	if(proc == NULL) {
		proc = ealloc(sizeof(Proc));
		proc->next = proclist;
	}
	proc->pid = pid;
	proc->alive = TRUE;
	proc->background = background;
	proc->prev = NULL;
	return proc;
}

/* efork -- fork (if necessary) and clean up as appropriate */
extern int
efork(Boolean parent, Boolean background)
{
	Proc *proc = nil;
	int pid = -1;

	if(parent) {
		switch((pid = fork())) {
		default:
			proc = mkproc(pid, background);
			if(proclist != NULL)
				proclist->prev = proc;
			proclist = proc;
			return pid;
		case 0: /* child */
			proclist = NULL;
			hasforked = TRUE;
			break;
		case -1:
			fail("es:efork", "fork: %s", esstrerror(errno));
		}
	}
	all_objects_onfork_ops();
	closefds();
	setsigdefaults();
	newchildcatcher();
	return 0;
}

static struct rusage wait_rusage;

static void
timevaldiff(struct timeval *after, struct timeval *before)
{
	after->tv_sec -= before->tv_sec;
	after->tv_usec -= before->tv_usec;
	if(after->tv_usec < 0)
		after->tv_sec -= 1, after->tv_usec += 1000000;
}

/* dowait -- a wait wrapper that interfaces with signals */
static int
dowait(int *statusp)
{
	int n;
	interrupted = FALSE;
	struct rusage r_before;

	if(!setjmp(slowlabel)) {
		slow = TRUE;
		if(interrupted)
			n = -2;
		else {
			/* on freebsd this maybe should be WEXITED|WTRAPPED */
			/* get the current rusage to help simulate how things would
			 * be if there was only one child */
			getrusage(RUSAGE_CHILDREN, &r_before);
			n = waitpid(-1, (void *)statusp, 0);
			getrusage(RUSAGE_CHILDREN, &wait_rusage);
			timevaldiff(&wait_rusage.ru_utime, &r_before.ru_utime);
			timevaldiff(&wait_rusage.ru_stime, &r_before.ru_stime);
		}
	} else
		n = -2;
	slow = FALSE;
	if(n == -2) {
		errno = EINTR;
		n = -1;
	}
	return n;
}

/* reap -- mark a process as dead and attach its exit status */
static void
reap(int pid, int status, struct rusage r)
{
	Proc *proc;
	for(proc = proclist; proc != NULL; proc = proc->next)
		if(proc->pid == pid) {
			assert(proc->alive);
			proc->alive = FALSE;
			proc->status = status;
			proc->rusage = wait_rusage;
			return;
		}
}

/* non-blocking wait. returns 0 if you could wait and not block,
 * -1 otherwise
 */
static int
nbwaitpid(int pid)
{
	int kstat, n, status;
	struct rusage r_before;

	kstat = kill(pid, 0);
	if(kstat < 0)
		return -1;

	getrusage(RUSAGE_CHILDREN, &r_before);
	n = waitpid(pid, (void *)&status, WNOHANG);
	if(n == pid) {
		getrusage(RUSAGE_CHILDREN, &wait_rusage);
		timevaldiff(&wait_rusage.ru_utime, &r_before.ru_utime);
		timevaldiff(&wait_rusage.ru_stime, &r_before.ru_stime);
		reap(pid, status, wait_rusage);
		return 0;
	}
	return -1;
}

/* ewait -- wait for a specific process to die, or any process if pid == 0 */
WaitStatus
ewait1(int pid, Boolean interruptible, void *rusage, Boolean print)
{
	Proc *proc;
	WaitStatus s;
top:
	for(proc = proclist; proc != NULL; proc = proc->next)
		if(proc->pid == pid || (pid == 0 && !proc->alive)) {
			int status;
			if(proc->alive) {
				int deadpid;
				int seen_eintr = FALSE;
				while((deadpid = dowait(&proc->status)) != pid)
					if(deadpid != -1)
						reap(deadpid, proc->status, wait_rusage);
					else if(errno == EINTR) {
						if(interruptible)
							sigchk();
						seen_eintr = TRUE;
					} else if(errno == ECHILD && seen_eintr)
						/* TODO: not clear on why this is necessary
						 * (child procs _sometimes_ disappear after SIGINT) */
						break;
					else
						fail("es:ewait", "wait: %s", esstrerror(errno));
				proc->alive = FALSE;
				proc->rusage = wait_rusage;
			}
			if(proc->next != NULL)
				proc->next->prev = proc->prev;
			if(proc->prev != NULL)
				proc->prev->next = proc->next;
			else
				proclist = proc->next;
			status = proc->status;
			if(proc->background && print)
				printstatus(proc->pid, status);
			s.pid = proc->pid;
			s.status = status;
			if(rusage != NULL)
				memcpy(rusage, &proc->rusage, sizeof(struct rusage));
			efree(proc);
			return s;
		}
	if(pid == 0) {
		int status;
		while((pid = dowait(&status)) == -1) {
			if(errno != EINTR)
				fail("es:ewait", "wait: %s", esstrerror(errno));
			if(interruptible)
				sigchk();
		}
		reap(pid, status, wait_rusage);
		goto top;
	}
	fail("es:ewait", "wait: %d is not a child of this shell", pid);
	unreachable();
	return s;
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
	int alive = 1, dead = 1;
	List *lp = NULL; Root r_lp;

	gcref(&r_lp, (void **)&lp);

	if(list != NULL && termeq(list->term, "-a"))
		dead = 0;
	else if(list != NULL && termeq(list->term, "-d"))
		alive = 0;

	for(p = proclist; p != NULL; p = p->next)
		if(p->background) {
			if(p->alive)
				nbwaitpid(p->pid);
			if(p->alive && !alive)
				continue;
			if(!p->alive && !dead)
				continue;
			Term *t = mkstr(str("%d", p->pid));
			lp = mklist(t, lp);
		}
	/* TODO: sort the return value, but by number? */

	gcderef(&r_lp, (void **)&lp);
	return lp;
}

PRIM(wait) {
	int pid = 0;
	Boolean print = TRUE;
	Boolean onlystatus = FALSE;
	WaitStatus s;
	if(list == NULL)
		pid = 0;
	else if(list->next == NULL) {
		pid = atoi(getstr(list->term));
		print = FALSE;
		if(pid < 0) {
			fail("$&wait", "wait: %d: bad pid", pid);
			unreachable();
		}
		if(pid != 0)
			onlystatus = TRUE;
	} else {
		fail("$&wait", "usage: wait [pid]");
		unreachable();
	}
	s = ewait1(pid, TRUE, NULL, print);
	if(onlystatus)
		return mklist(mkstr(mkstatus(s.status)), NULL);
	return mklist(mkstr(str("%d", s.pid)), mklist(mkstr(mkstatus(s.status)), NULL));
}

extern Dict *
initprims_proc(Dict *primdict)
{
	X(apids);
	X(wait);
	return primdict;
}
