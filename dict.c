/* dict.c -- hash-table based dictionaries ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include <stdint.h>

#define INIT_DICT_SIZE 2
#define REMAIN(n) (((n) * 2) / 3)
#define GROW(n) ((n) * 2)

/*
 * hashing
 */

#define FNV1A_HashStart 0x811c9dc5
#define FNV1A_HashIncr 0x01000193

HashFunction hashfunction = HaahrHash;

uint32_t
fnv1a_strhash2_len(const char *s1, size_t s1len, const char *s2, size_t s2len)
{
	size_t i;
	uint32_t hash = FNV1A_HashStart;

	for(i = 0; i < s1len; i++) {
		hash ^= (uint8_t)s1[i];
		hash *= FNV1A_HashIncr;
	}
	for(i = 0; i < s2len; i++) {
		hash ^= (uint8_t)s2[i];
		hash *= FNV1A_HashIncr;
	}

	return hash;
}

uint32_t
fnv1a_strhash2(const char *s1, const char *s2)
{
	size_t len1 = 0, len2 = 0;

	if(s1 != nil)
		len1 = strlen(s1);
	if(s2 != nil)
		len2 = strlen(s2);

	return fnv1a_strhash2_len(s1, len1, s2, len2);
}

uint32_t
fnv1a_strhash(const char *str)
{
	return fnv1a_strhash2(str, nil);
}

/* strhash2 -- the (probably too slow) haahr hash function */
static unsigned long
haahr_strhash2(const char *str1, const char *str2)
{

#define ADVANCE()                          \
	{                                      \
		if((c = *s++) == '\0') {           \
			if(str2 == NULL)               \
				break;                     \
			else {                         \
				s = (unsigned char *)str2; \
				str2 = NULL;               \
				if((c = *s++) == '\0')     \
					break;                 \
			}                              \
		}                                  \
	}

	int c;
	unsigned long n = 0;
	unsigned char *s = (unsigned char *)str1;
	assert(str1 != NULL);
	while(1) {
		ADVANCE();
		n += (c << 17) ^ (c << 11) ^ (c << 5) ^ (c >> 1);
		ADVANCE();
		n ^= (c << 14) + (c << 7) + (c << 4) + c;
		ADVANCE();
		n ^= (~c << 11) | ((c << 3) ^ (c >> 1));
		ADVANCE();
		n -= (c << 16) | (c << 9) | (c << 2) | (c & 3);
	}
	return n;
}

/* strhash -- hash a single string */
static unsigned long
haahr_strhash(const char *str)
{
	return haahr_strhash2(str, NULL);
}

uint64_t
strhash2(const char *str1, const char *str2)
{
	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		return haahr_strhash2(str1, str2);
	case FNV1AHash:
		return fnv1a_strhash2(str1, str2);
	}
	unreachable();
	return 0;
}

uint64_t
strhash(const char *str)
{
	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		return haahr_strhash(str);
	case FNV1AHash:
		return fnv1a_strhash(str);
	}
	unreachable();
	return 0;
}

/*
 * data structures and garbage collection
 */

DefineTag(Dict, static);

static Dict *
mkdict0(int size)
{
	size_t len = offsetof(Dict, table[size]);
	Dict *dict = gcalloc(len, tDict);
	memzero(dict, len);
	dict->readonly = 0;
	dict->size = size;
	dict->remain = REMAIN(size);
	return dict;
}

static void *
DictCopy(void *op)
{
	Dict *dict = op;
	size_t len = offsetof(Dict, table[dict->size]);
	void *np = gcalloc(len, tDict);
	memcpy(np, op, len);
	return np;
}

static size_t
DictScan(void *p)
{
	Dict *dict = p;
	int i;
	for(i = 0; i < dict->size; i++) {
		Assoc *ap = &dict->table[i];
		ap->name = forward(ap->name);
		ap->value = forward(ap->value);
	}
	return offsetof(Dict, table[dict->size]);
}

void
DictMark(void *p)
{
	Dict *d;
	Assoc *a;
	int i;

	d = (Dict *)p;
	gc_set_mark(header(p));

	for(i = 0; i < d->size; i++) {
		a = &d->table[i];
		gcmark(a->name);
		gcmark(a->value);
	}
}

/*
 * private operations
 */

char *DEAD = "%%DEAD%%";

static Assoc *
get(Dict *dict, const char *name)
{
	Assoc *ap;
	unsigned long n = strhash(name), mask = dict->size - 1;
	for(; (ap = &dict->table[n & mask])->name != NULL; n++)
		if(ap->name != DEAD && streq(name, ap->name))
			return ap;
	return NULL;
}

static void putwrapper(void *, char *, void *); // shut up clang

static Dict *
put(Dict *dict, char *name, void *value)
{
	unsigned long n, mask;
	Assoc *ap;
	Dict *old = NULL; Root r_old;
	char *np = NULL; Root r_np;
	void *vp = NULL; Root r_vp;
	Dict *new;

	assert(get(dict, name) == NULL);
	assert(value != NULL);
	assert(!dict->readonly);

	if(dict->remain <= 1) {
		old = dict;
		gcref(&r_old, (void **)&old);
		np = name;
		gcref(&r_np, (void **)&np);
		vp = value;
		gcref(&r_vp, (void **)&vp);

		new = mkdict0(GROW(old->size));
		dictforall(old, &putwrapper, new);
		dict = new;
		name = np;
		value = vp;

		gcderef(&r_vp, (void **)&vp);
		gcderef(&r_np, (void **)&np);
		gcderef(&r_old, (void **)&old);
	}

	n = strhash(name);
	mask = dict->size - 1;
	for(; (ap = &dict->table[n & mask])->name != DEAD; n++)
		if(ap->name == NULL) {
			--dict->remain;
			break;
		}

	ap->name = name;
	ap->value = value;
	return dict;
}

static void
putwrapper(void *a, char *b, void *c)
{
	put(a, b, c);
}

static void
rm(Dict *dict, Assoc *ap)
{
	unsigned long n, mask;
	assert(dict->table <= ap && ap < &dict->table[dict->size]);

	ap->name = DEAD;
	ap->value = NULL;
	n = ap - dict->table;
	mask = dict->size - 1;
	for(n++; (ap = &dict->table[n & mask])->name == DEAD; n++)
		;
	if(ap->name != NULL)
		return;
	for(n--; (ap = &dict->table[n & mask])->name == DEAD; n--) {
		ap->name = NULL;
		++dict->remain;
	}
}

/*
 * exported functions
 */

extern Dict *
mkdict(void)
{
	return mkdict0(INIT_DICT_SIZE);
}

extern void *
dictget(Dict *dict, const char *name)
{
	Assoc *ap = get(dict, name);
	if(ap == NULL)
		return NULL;
	return ap->value;
}

extern Dict *
dictput(Dict *dict, char *name, void *value)
{
	Assoc *ap = nil;

	assert(!dict->readonly);
	ap = get(dict, name);
	if(value != NULL)
		if(ap == NULL)
			dict = put(dict, name, value);
		else
			ap->value = value;
	else if(ap != NULL)
		rm(dict, ap);
	return dict;
}

extern void
dictforall(Dict *dp, void (*proc)(void *, char *, void *), void *arg)
{
	int i;
	Dict *dict = NULL; Root r_dict;
	void *argp = NULL; Root r_argp;

	dict = dp;
	gcref(&r_dict, (void **)&dict);
	argp = arg;
	gcref(&r_argp, (void **)&argp);
	for(i = 0; i < dict->size; i++) {
		Assoc *ap = &dict->table[i];
		if(ap->name != NULL && ap->name != DEAD)
			(*proc)(argp, ap->name, ap->value);
	}
	gcderef(&r_argp, (void **)&argp);
	gcderef(&r_dict, (void **)&dict);
}

/* dictget2 -- look up the catenation of two names (such a hack!) */
extern void *
dictget2(Dict *dict, const char *name1, const char *name2)
{
	Assoc *ap;
	unsigned long n = strhash2(name1, name2), mask = dict->size - 1;
	for(; (ap = &dict->table[n & mask])->name != NULL; n++)
		if(ap->name != DEAD && streq2(ap->name, name1, name2))
			return ap->value;
	return NULL;
}

Dict *
dictcopy(Dict *oda)
{
	int i;
	Dict *odict = NULL; Root r_odict;
	Dict *dict = NULL; Root r_dict;

	gcref(&r_odict, (void **)&odict);
	gcref(&r_dict, (void **)&dict);

	odict = oda;
	dict = mkdict();

	for(i = 0; i < odict->size; i++) {
		if(odict->table[i].name == NULL || odict->table[i].name == DEAD)
			continue;
		dict = dictput(dict, odict->table[i].name, (void *)odict->table[i].value);
	}

	gcrderef(&r_dict);
	gcrderef(&r_odict);

	return dict;
}

Dict *
dictappend(Dict *desta, Dict *srca, Boolean overwrite)
{
	int i;
	Dict *dest = NULL; Root r_dest;
	Dict *src = NULL; Root r_src;

	gcref(&r_dest, (void **)&dest);
	gcref(&r_src, (void **)&src);

	dest = desta;
	src = srca;

	for(i = 0; i < src->size; i++) {
		if(src->table[i].name == NULL || src->table[i].name == DEAD)
			continue;
		if(dictget(dest, src->table[i].name))
			if(!overwrite)
				continue;
		dest = dictput(dest, src->table[i].name, src->table[i].value);
	}

	gcrderef(&r_src);
	gcrderef(&r_dest);

	return dest;
}

Dict *
parsedict(Tree *tree0, Binding *binding0)
{
	Tree *tree = nil; Root r_tree;
	Binding *binding = nil; Root r_binding;
	Tree *inner = nil; Root r_inner;
	Dict *dict = nil; Root r_dict;
	Tree *assoc = nil; Root r_assoc;
	List *name = nil; Root r_name;
	List *value = nil; Root r_value;
	char *namestr = nil; Root r_namestr;

	gcref(&r_tree, (void **)&tree);
	gcref(&r_binding, (void **)&binding);
	gcref(&r_inner, (void **)&inner);
	gcref(&r_dict, (void **)&dict);
	gcref(&r_assoc, (void **)&assoc);
	gcref(&r_name, (void **)&name);
	gcref(&r_value, (void **)&value);
	gcref(&r_namestr, (void **)&namestr);

	tree = tree0;
	binding = binding0;
	dict = mkdict();
	for(inner = tree->u[0].p; inner != nil; inner = inner->u[1].p) {
		assoc = inner->u[0].p;
		assert(assoc->kind = nAssoc);
		name = glom1(assoc->u[0].p, binding);
		value = glom(assoc->u[1].p, binding, TRUE);
		if(name == nil)
			continue;
		namestr = getstr(name->term);
		dict = dictput(dict, namestr, value);
	}

	gcrderef(&r_namestr);
	gcrderef(&r_value);
	gcrderef(&r_name);
	gcrderef(&r_assoc);
	gcrderef(&r_dict);
	gcrderef(&r_inner);
	gcrderef(&r_binding);
	gcrderef(&r_tree);

	return dict;
}
