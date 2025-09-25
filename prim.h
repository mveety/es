/* prim.h -- definitions for es primitives ($Revision: 1.1.1.1 $) */

#define PRIMSMAX 500

#define	PRIM(name)	static List *CONCAT(prim_,name)( List * list, Binding * binding, int evalflags)
#define X(name) (primdict = dictput(primdict, STRING(name), (void *)CONCAT(prim_, name)))

#define LIBNAME(name) char dynlibname[] = STRING(name)
#define LIBAPI(n) int64_t dynlibapi = n
#define LIBRARY(name)                 \
	char dynlibname[] = STRING(name); \
	int64_t dynlibapi = DynLibApi
#define LIBRARY_NAME &dynlibname[0]
#define DYNPRIMS() Primitive dynprims[]
#define MODULE(name)                  \
	char dynlibname[] = STRING(name); \
	int64_t dynlibapi = DynLibApi;    \
	Primitive dynprims[]

#define DX(name) {STRING(name), CONCAT(&prim_, name)}
#define PRIMSEND {0, 0}

enum {
	DynLibApi = 3,

	// loading errors
	ErrorOkay = 0,
	ErrorModuleNotLoaded = 1,
	ErrorModuleMissingSymbol = 2,

	// unloading errors
	ErrorModuleInUse = 3, // if the module is in use

	// other errors
	ErrorOther = -126,
	ErrorModuleUnreachable = -127, // if the module can't be unloaded but es can't continue
};

typedef struct Primitive Primitive;
typedef struct DynamicLibrary DynamicLibrary;

/* we need to mirror a bit of dlfcn.h here */
typedef void (*DynFunction)(void *);

struct Primitive {
	char *name;
	List *(*symbol)(List *, Binding *, int);
};

struct DynamicLibrary {
	char *fname;
	char *name;
	void *handle;
	Primitive *prims;
	size_t primslen;
	int64_t apiversion;
	int (*onload)(void);
	int (*onunload)(void);
	DynamicLibrary *next;
};

extern Dict *initprims_controlflow(Dict *primdict); /* prim-ctl.c */
extern Dict *initprims_io(Dict *primdict);			/* prim-io.c */
extern Dict *initprims_etc(Dict *primdict);			/* prim-etc.c */
extern Dict *initprims_sys(Dict *primdict);			/* prim-sys.c */
extern Dict *initprims_proc(Dict *primdict);		/* proc.c */
extern Dict *initprims_access(Dict *primdict);		/* access.c */
extern Dict *initprims_dict(Dict *primdict);		/* prim-dict.c */
extern Dict *initprims_mv(Dict *primdict);			/* prim-mv.c */
extern Dict *initprims_math(Dict *primdict);		/* prim-math.c */
#ifdef DYNAMIC_LIBRARIES
extern Dict *initprims_dynlib(Dict *primdict); /* dynlib.c */
#endif

/* prim.c */
extern void add_prim(char *name, List *(*primfn)(List *, Binding *, int));
extern void remove_prim(char *name);

/* dynlib.c */
extern DynamicLibrary *open_library(char *fname, char *errstr, size_t errstrlen);
extern int close_library(DynamicLibrary *lib, char *errstr, size_t errstrlen);
extern DynamicLibrary *dynlib(char *name);
extern DynamicLibrary *dynlib_file(char *fname);
extern void *dynsymbol(DynamicLibrary *lib, char *symbol);
