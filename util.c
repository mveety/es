/* util.c -- the kitchen sink ($Revision: 1.2 $) */

#include "es.h"
#include <stdint.h>

#if !HAVE_STRERROR
/* strerror -- turn an error code into a string */
static char *
strerror(int n)
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	if(n > sys_nerr)
		return NULL;
	return sys_errlist[n];
}

#endif

/* esstrerror -- a wrapper around sterror(3) */
extern char *
esstrerror(int n)
{
	char *error = strerror(n);

	if(error == NULL)
		return "unknown error";
	return error;
}

/* uerror -- print a unix error, our version of perror */
extern void
uerror(char *s)
{
	if(s != NULL)
		eprint("%s: %s\n", s, esstrerror(errno));
	else
		eprint("%s\n", esstrerror(errno));
}

/* isabsolute -- test to see if pathname begins with "/", "./", or "../" */
extern Boolean
isabsolute(char *path)
{
	return path[0] == '/' ||
		   (path[0] == '.' && (path[1] == '/' || (path[1] == '.' && path[2] == '/')));
}

/* streq2 -- is a string equal to the concatenation of two strings? */
extern Boolean
streq2(const char *s, const char *t1, const char *t2)
{
	int c;
	assert(s != NULL && t1 != NULL && t2 != NULL);
	while((c = *t1++) != '\0')
		if(c != *s++)
			return FALSE;
	while((c = *t2++) != '\0')
		if(c != *s++)
			return FALSE;
	return *s == '\0';
}

/*
 * safe interface to malloc and friends
 */

/* ealloc -- error checked malloc */
extern void *
ealloc(size_t n)
{
	void *p = malloc(n);
	if(p == NULL) {
		uerror("malloc");
		exit(1);
	}
	memset(p, 0, n);
	return p;
}

/* erealloc -- error checked realloc */
extern void *
erealloc(void *p, size_t n)
{
	if(p == NULL)
		return ealloc(n);
	p = realloc(p, n);
	if(p == NULL) {
		uerror("realloc");
		exit(1);
	}
	return p;
}

char*
estrdup(char *str)
{
	char *res = nil;

	res = strdup(str);
	assert(res);
	return res;
}

char *
estrndup(char *str, size_t len)
{
	char *res = nil;

	res = strndup(str, len);
	assert(res);
	return res;
}

/* efree -- error checked free */
extern void
efree(void *p)
{
	assert(p != NULL);
	free(p);
}

/*
 * private interfaces to system calls
 */

extern void
ewrite(int fd, const char *buf, size_t n)
{
	volatile long i = 0, remain;
	const char *volatile bufp = buf;
	for(remain = n; remain > 0; bufp += i, remain -= i) {
		interrupted = FALSE;
		if(!setjmp(slowlabel)) {
			slow = TRUE;
			if(interrupted)
				break;
			else if((i = write(fd, bufp, remain)) <= 0)
				break; /* abort silently on errors in write() */
		} else
			break;
		slow = FALSE;
	}
	slow = FALSE;
	SIGCHK();
}

extern long
eread(int fd, char *buf, size_t n)
{
	long r;
	interrupted = FALSE;
	if(!setjmp(slowlabel)) {
		slow = TRUE;
		if(!interrupted)
			r = read(fd, buf, n);
		else
			r = -2;
	} else
		r = -2;
	slow = FALSE;
	if(r == -2) {
		errno = EINTR;
		r = -1;
	}
	SIGCHK();
	return r;
}

/* exit */

void
esexit(int status)
{
	deallocate_all_objects();
	if(editor_debugfd > 0)
		close(editor_debugfd);
	exit(status);
}

/* results */

int
status(Result r)
{
	return r.status;
}

void*
ok(Result r)
{
	if(r.status == 0)
		return r.ptr;
	uerror("result");
	esexit(r.status);
}

int64_t
ok_int(Result r)
{
	if(r.status == 0)
		return r.i;
	uerror("result");
	esexit(r.status);
}

double
ok_float(Result r)
{
	if(r.status == 0)
		return r.f;
	uerror("result");
	esexit(r.status);
}

char*
ok_str(Result r)
{
	if(r.status == 0)
		return r.str;
	uerror("result");
	esexit(r.status);
}

Result
result(void *ptr, int status)
{
	return (Result){{.ptr = ptr}, status};
}

Result
result_int(int64_t i, int status)
{
	return (Result){{.i = i}, status};
}

Result
result_float(double f, int status)
{
	return (Result){{.f = f}, status};
}

Result
result_str(char *str, int status)
{
	return (Result){{.str = str}, status};
}

