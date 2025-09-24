CFLAGS	= $(ADDCFLAGS) -I$(srcdir) -W -Wall -Wno-missing-field-initializers -Wno-unused-parameter $(AMCFLAGS)
LDFLAGS	= $(ADDLDFLAGS) $(AMLDFLAGS)
LIBS	= $(ADDLIBS) $(AMLIBS)

VPATH = $(srcdir)

HFILES	= config.h es.h gc.h input.h prim.h print.h sigmsgs.h \
		  stdenv.h syntax.h esconfig.h eval.h esmodule.h float_util.h

MAINCFILES	= access.c closure.c conv.c dict.c eval.c except.c fd.c gc.c \
		  gc_common.c glob.c glom.c input.c heredoc.c list.c main.c \
		  match.c ms_gc.c open.c opt.c prim-ctl.c prim-dict.c prim-etc.c \
		  prim-io.c prim-math.c prim-mv.c prim-sys.c prim.c print.c \
		  proc.c sigmsgs.c signal.c split.c status.c str.c syntax.c \
		  term.c token.c tree.c util.c var.c vec.c version.c y.tab.c dump.c \
		  objects.c

DYNCFILES = dynlib.c

MAINOFILES	= access.o closure.o conv.o dict.o eval.o except.o fd.o gc.o \
		  gc_common.o glob.o glom.o input.o heredoc.o list.o main.o \
		  match.o ms_gc.o open.o opt.o prim-ctl.o prim-dict.o prim-etc.o \
		  prim-io.o prim-math.o prim-mv.o prim-sys.o prim.o print.o \
		  proc.o sigmsgs.o signal.o split.o status.o str.o syntax.o \
		  term.o token.o tree.o util.o var.o vec.o version.o y.tab.o \
		  objects.o

DYNOFILES = dynlib.o

MODBASE_OFILES = mod_hello.o float_util.o mod_float.o mod_math.o mod_objutil.o
BASE_MODULES = mod_hello.so mod_float.so mod_math.so mod_objutil.so

MODJSON_OFILES = mod_json.o
MODJSON = mod_json.so

OTHER	= Makefile parse.y mksignal getsigfiles

SYSLIB = initial.es config.es conf.es macros.es dict.es applymap.es \
		 libraries.es errmatch.es range.es glob.es complete.es \
		 gc.es parseargs.es doiterator.es newvars.es suffix.es \
		 initialize.es
GEN	= esdump y.tab.c y.tab.h y.output token.h sigmsgs.c system.c config.es

