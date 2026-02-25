#ifndef _es_mod_file_h
#define _es_mod_file_h

typedef struct File File;

enum {
	OREAD = 1 << 0,
	OWRITE = 1 << 1,
	OCREATE = 1 << 2,
	OAPPEND = 1 << 3,

	FOFORK = 1 << 0,
};

struct File {
	char *name;
	int fd;
	int mode;
};

static File *
file(Object *obj)
{
	return (File *)obj->data;
}

#endif // _es_mod_file_h
