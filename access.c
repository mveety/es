/* access.c -- access testing and path searching ($Revision: 1.2 $) */

#include <string.h>
#define REQUIRE_STAT 1
#define REQUIRE_PARAM 1

#include "es.h"
#include "prim.h"

#define READ 4
#define WRITE 2
#define EXEC 1

#define USER 6
#define GROUP 3
#define OTHER 0

char *access_usage = nil;
char *access_opts = nil;

/* ingroupset -- determine whether gid lies in the user's set of groups */
static Boolean
ingroupset(gidset_t gid)
{
#ifdef NGROUPS
	int i;
	static int ngroups;
	static gidset_t gidset[NGROUPS];
	static Boolean initialized = FALSE;
	if(!initialized) {
		initialized = TRUE;
		ngroups = getgroups(NGROUPS, gidset);
	}
	for(i = 0; i < ngroups; i++)
		if(gid == gidset[i])
			return TRUE;
#endif
	return FALSE;
}

static int
testperm(struct stat *stat, int perm)
{
	int mask;
	static gidset_t uid, gid;
	static Boolean initialized = FALSE;
	if(perm == 0)
		return 0;
	if(!initialized) {
		initialized = TRUE;
		uid = geteuid();
		gid = getegid();
	}
	mask = (uid == 0) ? (perm << USER) | (perm << GROUP) | (perm << OTHER)
					  : (perm << ((uid == stat->st_uid)
									  ? USER
									  : ((gid == stat->st_gid || ingroupset(stat->st_gid)) ? GROUP : OTHER)));
	return (stat->st_mode & mask) ? 0 : EACCES;
}

static int
testfile(char *path, int perm, unsigned int type)
{
	struct stat st;
#ifdef S_IFLNK
	if(type == S_IFLNK) {
		if(lstat(path, &st) == -1)
			return errno;
	} else
#endif
		if(stat(path, &st) == -1)
		return errno;
	if(type != 0 && (st.st_mode & S_IFMT) != type)
		return EACCES; /* what is an appropriate return value? */
	return testperm(&st, perm);
}

char *
pathcat(char *prefix, char *suffix)
{
	char *s;
	size_t plen, slen, len;
	char *pathbuf = NULL;

	if(*prefix == '\0')
		return suffix;
	if(*suffix == '\0')
		return prefix;

	plen = strlen(prefix);
	slen = strlen(suffix);
	len = plen + slen + 2; /* one for '/', one for '\0' */
	pathbuf = ealloc(len);

	memcpy(pathbuf, prefix, plen);
	s = pathbuf + plen;
	if(s[-1] != '/')
		*s++ = '/';
	memcpy(s, suffix, slen + 1);
	return pathbuf;
}

int
access_gen_usage_opts(void)
{
	char usage[256];
	char opts[32];
	size_t usagei = 0;
	size_t optsi = 0;
	const char usage_start[] = "access [-n name] [-1e] [-rwx] [-fdcb";
	const char usage_end[] = "] path ...";
	const char opts_start[] = "bcdefn:rwx1h";

	if(access_usage == nil) {
		memset(&usage[0], 0, sizeof(usage));
		strcpy(&usage[0], usage_start);
		usagei = sizeof(usage_start) - 1;
#ifdef S_IFLNK
		usage[usagei++] = 'l';
#endif
#ifdef S_IFSOCK
		usage[usagei++] = 's';
#endif
#ifdef S_IFIFO
		usage[usagei++] = 'p';
#endif
		strcpy(&usage[usagei], usage_end);
		access_usage = strdup(usage);
		if(access_usage == nil) {
			uerror("strdup");
			esexit(-1);
		}
	}
	if(access_opts == nil) {
		memset(&opts[0], 0, sizeof(opts));
		strcpy(&opts[0], opts_start);
		optsi = sizeof(opts_start) - 1;
#ifdef S_IFLNK
		opts[optsi++] = 'l';
#endif
#ifdef S_IFSOCK
		opts[optsi++] = 's';
#endif
#ifdef S_IFIFO
		opts[optsi++] = 'p';
#endif
		access_opts = strdup(opts);
		if(access_opts == nil) {
			uerror("strdup");
			esexit(-1);
		}
	}
	return 0;
}

PRIM(access) {
	int c, perm = 0, type = 0, estatus = ENOENT;
	Boolean first = FALSE, exception = FALSE;
	char *suffix = NULL;
	List *lp;
	List *result = NULL; Root r_result;

	access_gen_usage_opts();

	gcdisable();
	esoptbegin(list, "$&access", access_usage);
	while((c = esopt(access_opts)) != EOF)
		switch(c) {
		default:
			esoptend();
			fail("$&access", "access -%c is not supported on this system", c);
		case 'h':
			esoptend();
			gcenable();
			eprint("usage: %s\n", access_usage);
			return nil;
		case 'n':
			suffix = getstr(esoptarg());
			break;
		case '1':
			first = TRUE;
			break;
		case 'e':
			exception = TRUE;
			break;
		case 'r':
			perm |= READ;
			break;
		case 'w':
			perm |= WRITE;
			break;
		case 'x':
			perm |= EXEC;
			break;
		case 'f':
			type = S_IFREG;
			break;
		case 'd':
			type = S_IFDIR;
			break;
		case 'c':
			type = S_IFCHR;
			break;
		case 'b':
			type = S_IFBLK;
			break;
#ifdef S_IFLNK
		case 'l':
			type = S_IFLNK;
			break;
#endif
#ifdef S_IFSOCK
		case 's':
			type = S_IFSOCK;
			break;
#endif
#ifdef S_IFIFO
		case 'p':
			type = S_IFIFO;
			break;
#endif
		}
	list = esoptend();

	for(lp = NULL; list != NULL; list = list->next) {
		int error;
		char *name;
		char *listname;

		listname = getstr(list->term);
		if(suffix != NULL)
			name = pathcat(listname, suffix);
		else
			name = strdup(listname);
		error = testfile(name, perm, type);

		if(first) {
			if(error == 0) {
				result = mklist(mkstr(gcdup(name)), NULL);
				efree(name);
				gcref(&r_result, (void **)&result);
				gcenable();
				gcderef(&r_result, (void **)&result);
				return result;
			} else if(error != ENOENT)
				estatus = error;
		} else
			lp = mklist(mkstr(error == 0 ? "0" : esstrerror(error)), lp);
		efree(name);
	}

	if(first && exception) {
		gcenable();
		if(suffix)
			fail("$&access", "%s: %s", suffix, esstrerror(estatus));
		else
			fail("$&access", "%s", esstrerror(estatus));
	}

	result = reverse(lp);
	gcref(&r_result, (void **)&result);
	gcenable();
	gcderef(&r_result, (void **)&result);
	return result;
}

extern Dict *
initprims_access(Dict *primdict)
{
	X(access);
	return primdict;
}

extern char *
checkexecutable(char *file)
{
	int err = testfile(file, EXEC, S_IFREG);
	return err == 0 ? NULL : esstrerror(err);
}
