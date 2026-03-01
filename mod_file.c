#include "es.h"
#include "prim.h"
#include "stdenv.h"
#include "mod_file.h"

static int
modmode2mode(int modmode)
{
	int mode = 0;

	if((modmode & (OREAD | OWRITE)) == (OREAD | OWRITE))
		mode |= O_RDWR;
	else if(modmode & OWRITE)
		mode |= O_WRONLY;
	else if(modmode & OREAD)
		mode |= O_RDONLY;
	else
		unreachable(); // uh oh

	if(modmode & OCREATE)
		mode |= O_CREAT;
	if(modmode & OAPPEND)
		mode |= O_APPEND;

	return mode;
}

static int
fileopen(Object *obj, char *f, int modmode, int flags)
{
	int fd = -1;
	int mode = 0;

	if(obj->sysflags & ObjectInitialized)
		return -1;
	if(!f)
		return -2;
	if((mode = modmode2mode(modmode)) < 0)
		return -3;
	errno = 0;
	if((fd = open(f, mode)) < 0)
		return -4;

	file(obj)->name = estrdup(f);
	file(obj)->fd = fd;
	file(obj)->mode = modmode;
	obj->sysflags |= ObjectInitialized;
	obj->sysflags |= ObjectCallbackOnFork;
	obj->flags = flags;

	return 0;
}

static Object *
file_allocate(void)
{
	Object *o;

	o = allocate_object("file", sizeof(File));
	assert(o);
	memset(file(o), 0, sizeof(File));
	o->sysflags |= ObjectFreeWhenNoRefs;
	o->sysflags |= ObjectGcManaged;
	file(o)->fd = -1;
	file(o)->name = nil;
	file(o)->mode = 0;

	return o;
}

static int
file_deallocate(Object *obj)
{
	close(file(obj)->fd);
	if(file(obj)->name)
		free(file(obj)->name);
	file(obj)->fd = -1;
	file(obj)->name = nil;
	file(obj)->mode = 0;
	return 0;
}

static int
file_onfork(Object *obj)
{
	if(obj->flags & FOFORK && obj->sysflags & ObjectInitialized) {
		close(file(obj)->fd);
		if(file(obj)->name)
			free(file(obj)->name);
		obj->sysflags &= ~ObjectInitialized;
		file(obj)->fd = -1;
		file(obj)->name = nil;
		file(obj)->mode = 0;
	}
	return 0;
}

static char *
file_stringify(Object *obj)
{
	char buf[2048];

	memset(buf, 0, sizeof(buf));
	if(obj->sysflags & ObjectInitialized)
		snprintf(&buf[0], sizeof(buf) - 1, "%s (fd = %d, %c%c%c%c%s)",
				 file(obj)->name ? file(obj)->name : "(nil)", file(obj)->fd,
				 file(obj)->mode & OREAD ? 'r' : '-', file(obj)->mode & OWRITE ? 'w' : '-',
				 file(obj)->mode & OCREATE ? 'c' : '-', file(obj)->mode & OAPPEND ? 'a' : '-',
				 obj->flags & FOFORK ? ", closeonfork" : "");
	else
		snprintf(&buf[0], sizeof(buf) - 1, "(nil) (fd = -1, ----, closeonfork)");

	return estrdup(&buf[0]);
}

Object *
newblob(size_t size)
{
	Object *obj;

	obj = allocate_object("blob", size);
	assert(obj);
	obj->sysflags |= ObjectFreeWhenNoRefs;
	obj->sysflags |= ObjectGcManaged;
	obj->sysflags |= ObjectInitialized;
	memset(obj->data, 0, obj->size);
	return obj;
}

static char *
blob_stringify(Object *obj)
{
	char buf[2048];

	memset(&buf[0], 0, sizeof(buf));
	snprintf(&buf[0], sizeof(buf)-1, "size = %lu", obj->size);
	return estrdup(&buf[0]);
}

static int
file_onload(void)
{
	if(define_type("file", &file_deallocate, nil) < 0) {
		dprintf(2, "unable to define type file\n");
		return -1;
	}
	if(define_type("blob", nil, nil) < 0) {
		dprintf(2, "unable to define type blob\n");
		undefine_type("file");
		return -1;
	}
	define_stringifier("file", &file_stringify);
	define_onfork_callback("file", &file_onfork);
	define_allocator("file", &file_allocate);
	define_stringifier("blob", &blob_stringify);
	return 0;
}

static int
file_onunload(void)
{
	int r = 0;

	if((r = undefine_type("file")) != 0)
		return r;
	if((r = undefine_type("blob")) != 0)
		return r;

	return 0;
}

static int
parsemode(char *modestr)
{
	int mode = 0;
	int i = 0;

	for(i = 0; modestr[i] != 0; i++)
		switch(modestr[i]) {
		default:
			return -1;
		case '-':
			continue;
		case 'r':
			mode |= OREAD;
			break;
		case 'w':
			mode |= OWRITE;
			break;
		case 'c':
			mode |= OCREATE;
			break;
		case 'a':
			mode |= OAPPEND;
			break;
		}
	return mode;
}

PRIM(file_open) {
	Object *obj = nil;
	List *res = nil; Root r_res;
	Root r_list;
	char *fname;
	int mode = 0;

	if(list == nil)
		fail("$&file_open", "missing arguments");
	if(list->next == nil)
		fail("$&file_open", "missing arguments");
	if(list->next->next != nil)
		fail("$&file_open", "too many arguments");

	gcref(&r_res, (void **)&res);
	gcref(&r_list, (void **)&list);

	fname = getstr(list->term);
	if(!fname)
		fail("$&file_open", "invalid file name");
	if((mode = parsemode(getstr(list->next->term))) <= 0)
		fail("$&file_open", "invalid mode: %s", getstr(list->next->term));

	gcdisable();
	obj = file_allocate();
	dprint("mod_file: opening %s, mode = %x\n", fname, mode);
	switch(fileopen(obj, fname, mode, FOFORK)) {
	case -1:
	case -2:
	case -3:
		unreachable(); // should be handled already
	case -4:
		gcenable();
		fail("$&file_open", "unable to open %s: %s", fname, strerror(errno));
		break;
	}

	res = mklist(mkobject(obj), nil);
	gcenable();
	gcrderef(&r_list);
	gcrderef(&r_res);
	return res;
}

PRIM(file_name) {
	Root r_list;
	List *res = nil; Root r_res;
	Object *obj = nil;

	if(list == nil)
		fail("$&file_name", "invalid argument");

	gcref(&r_list, (void**)&list);
	gcref(&r_res, (void**)&res);
	obj = getobject(list->term);
	if(!obj)
		fail("$&file_name", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_name", "$1 must be a file object");

	if(obj->sysflags & ObjectInitialized)
		res = mklist(mkstr(str("%s", file(obj)->name)), nil);

	gcrderef(&r_res);
	gcrderef(&r_list);

	return res;
}

PRIM(file_fd) {
	Root r_list;
	List *res = nil; Root r_res;
	Object *obj = nil;

	if(list == nil)
		fail("$&file_name", "invalid argument");

	gcref(&r_list, (void**)&list);
	gcref(&r_res, (void**)&res);
	obj = getobject(list->term);
	if(!obj)
		fail("$&file_fd", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_fd", "$1 must be a file object");

	if(obj->sysflags & ObjectInitialized)
		res = mklist(mkstr(str("%d", file(obj)->fd)), nil);
	else
		fail("$&file_fd", "file not initialized");

	gcrderef(&r_res);
	gcrderef(&r_list);

	return res;
}

PRIM(blob2string) {
	Root r_list;
	Object *obj = nil;
	char *newstr = nil; Root r_newstr;
	List *res = nil; Root r_res;

	if(list == nil)
		fail("$&blob2string", "missing argument");
	if(list->next != nil)
		fail("$&blob2string", "too many arguments");

	gcref(&r_list, (void**)&list);
	gcref(&r_newstr, (void**)&newstr);
	gcref(&r_res, (void**)&res);

	obj = getobject(list->term);
	if(!obj)
		fail("$&blob2string", "must be an object");
	if(!object_is_type(obj, "blob"))
		fail("$&blob2string", "must be a blob object");

	newstr = gcalloc(obj->size+1, tString);
	memset(newstr, 0, obj->size+1);
	memcpy(newstr, obj->data, obj->size);

	res = mklist(mkstr(newstr), nil);

	gcrderef(&r_res);
	gcrderef(&r_newstr);
	gcrderef(&r_list);

	return res;
}

PRIM(string2blob) {
	Root r_list;
	Object *obj = nil;
	char *str = nil; Root r_str;
	List *res = nil; Root r_res;
	size_t slen = 0;

	if(list == nil)
		fail("$&string2blob", "missing argument");
	if(list->next != nil)
		fail("$&string2blob", "too many arguments");

	gcref(&r_list, (void**)&list);
	gcref(&r_str, (void**)&str);
	gcref(&r_res, (void**)&res);

	str = getstr(list->term);
	if(!str)
		fail("$&string2blob", "not a string");

	slen = strlen(str);
	if(slen == 0)
		fail("$&string2blob", "empty string");

	obj = newblob(slen);
	memcpy(obj->data, str, slen);

	res = mklist(mkobject(obj), nil);

	gcrderef(&r_res);
	gcrderef(&r_str);
	gcrderef(&r_list);

	return res;
}

PRIM(file_read) {
	char *buf = nil; Root r_buf;
	char *rstr = nil; Root r_rstr;
	Object *obj = nil;
	size_t nbytes = 0;
	ssize_t nread = 0;
	List *res = nil; Root r_res;
	Root r_list;

	if(length(list) < 2)
		fail("$&file_read", "missing arguments");
	if(length(list) > 2)
		fail("$&file_read", "too many arguments");

	gcref(&r_list, (void **)&list);
	gcref(&r_res, (void **)&res);
	gcref(&r_buf, (void **)&buf);
	gcref(&r_rstr, (void **)&rstr);

	obj = getobject(list->term);
	if(!obj)
		fail("$&file_read", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_read", "$1 must be a file object");

	errno = 0;
	nbytes = strtoll(getstr(list->next->term), nil, 10);
	if(nbytes == 0)
		switch(errno) {
		case EINVAL:
			fail("$&file_read", "invalid input: $2 = %s", getstr(list->next->term));
			break;
		case ERANGE:
			fail("$&file_read", "conversion overflow: $2 = %s", getstr(list->next->term));
			break;
		}

	buf = gcmalloc(nbytes);

	errno = 0;
	nread = read(file(obj)->fd, buf, nbytes);
	if(nread < 0)
		fail("$&file_read", "unable to read: %s", strerror(errno));
	if(nread == 0) {
		res = mklist(mkstr(str("0")), nil);
		gcrderef(&r_rstr);
		gcrderef(&r_buf);
		gcrderef(&r_res);
		gcrderef(&r_list);
		return res;
	}
	rstr = gcalloc(nread + 1, tString);
	memset(rstr, 0, nread + 1);
	memcpy(rstr, buf, nread);
	res = mklist(mkstr(rstr), nil);
	res = mklist(mkstr(str("%d", nread)), res);

	gcrderef(&r_rstr);
	gcrderef(&r_buf);
	gcrderef(&r_res);
	gcrderef(&r_list);
	return res;
}

PRIM(file_bread) {
	char *buf = nil; Root r_buf;
	Object *obj = nil;
	Object *blob = nil;
	size_t nbytes = 0;
	ssize_t nread = 0;
	List *res = nil; Root r_res;
	Root r_list;

	if(length(list) < 2)
		fail("$&file_bread", "missing arguments");
	if(length(list) > 2)
		fail("$&file_bread", "too many arguments");

	gcref(&r_list, (void **)&list);
	gcref(&r_res, (void **)&res);
	gcref(&r_buf, (void **)&buf);

	obj = getobject(list->term);
	if(!obj)
		fail("$&file_bread", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_bread", "$1 must be a file object");

	errno = 0;
	nbytes = strtoll(getstr(list->next->term), nil, 10);
	if(nbytes == 0)
		switch(errno) {
		case EINVAL:
			fail("$&file_bread", "invalid input: $2 = %s", getstr(list->next->term));
			break;
		case ERANGE:
			fail("$&file_bread", "conversion overflow: $2 = %s", getstr(list->next->term));
			break;
		}

	buf = gcmalloc(nbytes);

	errno = 0;
	nread = read(file(obj)->fd, buf, nbytes);
	if(nread < 0)
		fail("$&file_bread", "unable to read: %s", strerror(errno));
	if(nread == 0) {
		res = mklist(mkstr(str("0")), nil);
		gcrderef(&r_buf);
		gcrderef(&r_res);
		gcrderef(&r_list);
		return res;
	}

	blob = newblob(nread);
	memcpy(blob->data, buf, nread);
	res = mklist(mkobject(blob), nil);
	res = mklist(mkstr(str("%d", nread)), res);

	gcrderef(&r_buf);
	gcrderef(&r_res);
	gcrderef(&r_list);
	return res;
}

PRIM(file_write) {
	Root r_list;
	char *outstr = nil; Root r_outstr;
	size_t outstrlen = 0;
	ssize_t nwritten = 0;
	Object *obj;

	if(length(list) < 2)
		fail("$&file_write", "missing arguments");
	if(length(list) > 2)
		fail("$&file_write", "too many arguments");

	gcref(&r_list, (void**)&list);
	gcref(&r_outstr, (void**)&outstr);

	obj = getobject(list->term);
	if(!obj)
		fail("$&file_write", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_write", "$1 must be a file object");

	outstr = getstr(list->next->term);
	outstrlen = strlen(outstr);

	errno = 0;
	nwritten = write(file(obj)->fd, outstr, outstrlen);
	if(nwritten < 0)
		fail("$&file_write", "unable to write: %s", strerror(errno));

	gcrderef(&r_outstr);
	gcrderef(&r_list);

	return mklist(mkstr(str("%d", nwritten)), nil);
}

PRIM(file_bwrite) {
	Root r_list;
	ssize_t nwritten = 0;
	Object *obj;
	Object *blob;

	if(length(list) < 2)
		fail("$&file_bwrite", "missing arguments");
	if(length(list) > 2)
		fail("$&file_bwrite", "too many arguments");

	gcref(&r_list, (void**)&list);

	obj = getobject(list->term);
	if(!obj)
		fail("$&file_bwrite", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_bwrite", "$1 must be a file object");

	blob = getobject(list->next->term);
	if(!blob)
		fail("$&file_bwrite", "$2 must be an object");
	if(!object_is_type(blob, "blob"))
		fail("$&file_bwrite", "$2 must be a blob object");

	errno = 0;
	nwritten = write(file(obj)->fd, blob->data, blob->size);
	if(nwritten < 0)
		fail("$&file_bwrite", "unable to write: %s", strerror(errno));

	gcrderef(&r_list);

	return mklist(mkstr(str("%d", nwritten)), nil);
}

PRIM(file_seek) {
	Root r_list;
	off_t newoffset = 0;
	off_t offset = 0;
	int whence;
	Object *obj;

	if(length(list) < 3)
		fail("$&file_seek", "missing arguments");
	if(length(list) > 3)
		fail("$&file_seek", "too many arguments");

	gcref(&r_list, (void**)&list);

	obj = getobject(list->term);
	if(!obj)
		fail("$&file_seek", "$1 must be an object");
	if(!object_is_type(obj, "file"))
		fail("$&file_seek", "$1 must be a file object");

	errno = 0;
	offset = strtoll(getstr(list->next->term), nil, 10);
	if(offset == 0)
		switch(errno) {
		case EINVAL:
			fail("$&file_seek", "invalid input: $2 = %s", getstr(list->next->term));
			break;
		case ERANGE:
			fail("$&file_seek", "conversion overflow: $2 = %s", getstr(list->next->term));
			break;
		}

	if(termeq(list->next->next->term, "start"))
		whence = SEEK_SET;
	else if(termeq(list->next->next->term, "cur"))
		whence = SEEK_CUR;
	else if(termeq(list->next->next->term, "end"))
		whence = SEEK_END;
	else
		fail("$&file_seek", "invalid $3. must be start, cur, or end");

	errno = 0;
	newoffset = lseek(file(obj)->fd, offset, whence);
	if(newoffset == -1)
		fail("$&file_seek", "unable to seek: %s", strerror(errno));

	gcrderef(&r_list);
	return mklist(mkstr(str("%d", newoffset)), nil);
}

MODULE(mod_file, &file_onload, &file_onunload,
	DX(file_open),
	DX(file_name),
	DX(file_fd),
	DX(blob2string),
	DX(string2blob),
	DX(file_read),
	DX(file_bread),
	DX(file_write),
	DX(file_bwrite),
	DX(file_seek),
);
