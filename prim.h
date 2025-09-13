/* prim.h -- definitions for es primitives ($Revision: 1.1.1.1 $) */

#define	PRIM(name)	static List *CONCAT(prim_,name)( \
				List *list, Binding *binding, int evalflags \
			)
#define	X(name)		(primdict = dictput( \
				primdict, \
				STRING(name), \
				(void *) CONCAT(prim_,name) \
			))

#define LIBNAME(name) char dynlibname[] = STRING(name)
#define DYNPRIMS() Primitive dynprims[]
#define DX(name) {STRING(name), CONCAT(&prim_,name)},
#define DYNPRIMSLEN() size_t dynprimslen = (sizeof(dynprims)/sizeof(Primitive))

typedef struct Primitive Primitive;

struct Primitive {
	char *name;
	List* (*symbol)(List*, Binding*, int);
};

extern Dict *initprims_controlflow(Dict *primdict);	/* prim-ctl.c */
extern Dict *initprims_io(Dict *primdict);		/* prim-io.c */
extern Dict *initprims_etc(Dict *primdict);		/* prim-etc.c */
extern Dict *initprims_sys(Dict *primdict);		/* prim-sys.c */
extern Dict *initprims_proc(Dict *primdict);		/* proc.c */
extern Dict *initprims_access(Dict *primdict);		/* access.c */
extern Dict *initprims_dict(Dict *primdict); /* prim-dict.c */
extern Dict *initprims_mv(Dict *primdict); /* prim-mv.c */
extern Dict *initprims_math(Dict *primdict); /* prim-math.c */
#ifdef DYNAMIC_LIBRARIES
extern Dict *initprims_dynlib(Dict *primdict); /* dynlib.c */
#endif


extern void add_prim(char *name, List* (*primfn)(List*, Binding*, int));
extern void remove_prim(char *name);

