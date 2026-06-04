#include "es.h"
#include "prim.h"
#include "gc.h"

enum {
	Read = 4,
	Write = 2,
	Exec = 1,

	User = 6,
	Group = 3,
	Other = 0,
};

typedef struct AccessContext AccessContext;
typedef struct stat Stat;

struct AccessContext {
	Boolean initialized;
	int ngroups;
	gidset_t gidset[NGROUPS_MAX];
	gidset_t uid;
	gidset_t gid;
};

char *naccess_usage = nil;
char *naccess_opts = nil;

static void
initialize_accesscontext(AccessContext *ac)
{
	assert(ac->initialized == FALSE);

	/* memset(ac, 0, sizeof(AccessContext)); */
	ac->initialized = TRUE;
	ac->ngroups = getgroups(NGROUPS_MAX, &ac->gidset[0]);
	ac->uid = geteuid();
	ac->gid = getegid();
}

static Boolean
ingroupset(AccessContext *ac, gidset_t gid)
{
	int i = 0;

	assert(ac->initialized == TRUE);
	for(i = 0; i < ac->ngroups; i++)
		if(gid == ac->gidset[i])
			return TRUE;
	return FALSE;
}

static int
testperm(AccessContext *ac, Stat *stat, int perm)
{
	int mask = 0;

	assert(ac->initialized == TRUE);

	if(perm == 0)
		return 0;

	if(ac->uid == 0)
		return (stat->st_mode & ((perm << User) | (perm << Group) | (perm << Other))) ? 0 : EACCES;
	if(ac->uid == stat->st_uid)
		return (stat->st_mode & User) ? 0 : EACCES;

	mask = perm << ((ac->gid == stat->st_gid || ingroupset(ac, stat->st_gid)) ? Group : Other);
	return (stat->st_mode & mask) ? 0 : EACCES;
}

static int
testfile(AccessContext *ac, char *path, int perm, unsigned int type)
{
	Stat st;

	assert(ac->initialized == TRUE);

	switch(type) {
	default:
		if(stat(path, &st) == -1)
			return errno;
		break;
#ifdef S_IFLNK
	case S_IFLNK:
		if(lstat(path, &st) == -1)
			return errno;
		break;
#endif
	}

	if(type != 0 && (st.st_mode & S_IFMT) != type)
		return EACCES;
	return testperm(ac, &st, perm);
}

char *
checkexecutable(char *file)
{
	AccessContext ac;
	int error;

	ac.initialized = FALSE;
	initialize_accesscontext(&ac);
	if((error = testfile(&ac, file, Exec, S_IFREG)) == 0)
		return nil;
	return esstrerror(error);
}

static char *
pathcat(Arena *arena, char *prefix, char *suffix)
{
	char *s = nil;
	size_t plen, slen, len;
	char *pathbuf = nil;

	if(*prefix == 0)
		return suffix;
	if(*suffix == 0)
		return prefix;

	plen = strlen(prefix);
	slen = strlen(suffix);
	len = plen + slen + 2;
	pathbuf = arena_allocate(arena, len);
	memset(pathbuf, 0, arena_sizeof(arena, pathbuf));

	memcpy(pathbuf, prefix, plen);
	s = pathbuf + plen;
	if(s[-1] != '/')
		*s++ = '/';
	memcpy(s, suffix, slen + 1);
	return pathbuf;
}

static int
naccess_gen_usage_opts(void)
{
	char usage[256];
	char opts[32];
	size_t usagei = 0;
	size_t optsi = 0;
	const char usage_start[] = "naccess [-n name] [-1e] [-rwx] [-fdcb";
	const char usage_end[] = "] path ...";
	const char opts_start[] = "bcdDefn:rwx1h";

	if(naccess_usage == nil) {
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
		naccess_usage = estrdup(usage);
	}
	if(naccess_opts == nil) {
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
		naccess_opts = estrdup(opts);
	}
	return 0;
}

PRIM(naccess) {
	int c, perm = 0, type = 0;
	Boolean first = FALSE, exception = FALSE, found = FALSE;
	char *suffix = nil;
	List *lp = nil;
	List *result = nil; Root r_result;
	AccessContext ac;
	Arena *naccess_arena = nil;
	int error = 0, lasterror = 0;
	Dict *resdict = nil; Root r_resdict;

	gcdisable();
	naccess_gen_usage_opts();
	esoptbegin(list, "$&naccess", naccess_usage);
	while((c = esopt(naccess_opts)) != EOF)
		switch(c) {
		default:
			esoptend();
			fail("$&naccess", "naccess -%c is not supported on this system", c);
			break;
		case 'h':
			esoptend();
			gcenable();
			eprint("usage: %s\n", naccess_usage);
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
		case 'D':
			resdict = mkdict();
			break;
		case 'r':
			perm |= Read;
			break;
		case 'w':
			perm |= Write;
			break;
		case 'x':
			perm |= Exec;
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

	naccess_arena = newarena(512);
	arena_annotate(naccess_arena, "naccess arena");
	memset(&ac, 0, sizeof(AccessContext));
	initialize_accesscontext(&ac);
	gcref(&r_result, (void **)&result);
	gcref(&r_resdict, (void **)&resdict);

	for(lp = nil, list = esoptend(); list != nil; list = list->next) {
		char *name = nil;

		name = suffix != nil ? pathcat(naccess_arena, getstr(list->term), suffix)
							 : arena_dup(naccess_arena, getstr(list->term));
		error = testfile(&ac, name, perm, type);

		if(first && error == 0) {
			result = mklist(mkstr(gcdup(name)), nil);
			gcenable();
			gcrderef(&r_result);
			arena_destroy(naccess_arena);
			return result;
		}

		if(exception && suffix == nil && error != 0) {
			char *gcname = gcdup(name); // need to get name out of the arena
			arena_destroy(naccess_arena);
			fail("$&naccess", "%s: %s", gcname, esstrerror(error));
		}

		if(error == 0)
			found = TRUE;
		else
			lasterror = error;
		if(resdict)
			resdict = dictput(resdict, gcdup(name),
							  mklist(mkstr(str("%s", error == 0 ? "0" : esstrerror(error))), nil));
		else
			lp = mklist(mkstr(error == 0 ? "0" : esstrerror(error)), lp);
	}

	if(exception && suffix != nil && !found) {
		arena_destroy(naccess_arena);
		fail("$&naccess", "%s: %s", suffix, esstrerror(lasterror));
	}

	if(resdict)
		result = mklist(mkdictterm(resdict), nil);
	else
		result = reverse(lp);
	gcenable();
	gcrderef(&r_resdict);
	gcrderef(&r_result);
	arena_destroy(naccess_arena);
	return result;
}

Primitive prims_naccess[] = {
	DX(naccess),
};

Dict *
initprims_naccess(Dict *primdict)
{
	size_t i = 0;

	for(i = 0; i < nelem(prims_naccess); i++)
		primdict = dictput(primdict, prims_naccess[i].name, (void *)prims_naccess[i].symbol);

	return primdict;
}
