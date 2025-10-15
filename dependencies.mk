# --- dependencies ---

ESHFILES = config.h esconfig.h stdenv.h es.h editor.h

stdenv.h : esconfig.h
es.h : config.h stdenv.h
access.o : access.c $(ESHFILES) prim.h
closure.o : closure.c $(ESHFILES) gc.h
conv.o : conv.c $(ESHFILES) print.h
dict.o : dict.c $(ESHFILES) gc.h
eval.o : eval.c $(ESHFILES)
except.o : except.c $(ESHFILES) print.h
fd.o : fd.c $(ESHFILES)
gc.o : gc.c $(ESHFILES) gc.h
glob.o : glob.c $(ESHFILES) gc.h
glom.o : glom.c $(ESHFILES) gc.h
input.o : input.c $(ESHFILES) input.h
heredoc.o : heredoc.c $(ESHFILES) gc.h input.h syntax.h
list.o : list.c $(ESHFILES) gc.h
main.o : main.c $(ESHFILES) token.h
match.o : match.c $(ESHFILES)
open.o : open.c $(ESHFILES)
opt.o : opt.c $(ESHFILES)
prim.o : prim.c $(ESHFILES) prim.h
prim-ctl.o : prim-ctl.c $(ESHFILES) prim.h
prim-dict.o : prim-dict.c $(ESHFILES) prim.h
prim-etc.o : prim-etc.c $(ESHFILES) prim.h
prim-io.o : prim-io.c $(ESHFILES) gc.h prim.h
prim-math.o : prim-math.c $(ESHFILES) prim.h
prim-mv.o : prim-mv.c $(ESHFILES) prim.h
prim-sys.o : prim-sys.c $(ESHFILES) prim.h
print.o : print.c $(ESHFILES) print.h
proc.o : proc.c $(ESHFILES) prim.h
signal.o : signal.c $(ESHFILES) sigmsgs.h
split.o : split.c $(ESHFILES) gc.h
status.o : status.c $(ESHFILES)
str.o : str.c $(ESHFILES) gc.h print.h
syntax.o : syntax.c $(ESHFILES) input.h syntax.h token.h
term.o : term.c $(ESHFILES) gc.h
token.o : token.c $(ESHFILES) input.h syntax.h token.h
tree.o : tree.c $(ESHFILES) gc.h
util.o : util.c $(ESHFILES)
var.o : var.c $(ESHFILES) gc.h
vec.o : vec.c $(ESHFILES) gc.h
version.o : version.c $(ESHFILES)
dynlib.o : dynlib.c $(ESHFILES) prim.h gc.h
objects.o : objects.c $(ESHFILES) prim.h gc.h
editor.o : editor.c $(ESHFILES) gc.h
test_editor.o : MODCFLAGS=-DSTANDALONE
test_editor.o : test_editor.c editor.h
# modules
mod_hello.o : mod_hello.c $(ESHFILES) prim.h gc.h
mod_hello.so : mod_hello.o
mod_objutil.o : mod_objutil.c $(ESHFILES) prim.h gc.h
mod_objutil.so : mod_objutil.o
mod_float.o : mod_float.c $(ESHFILES) prim.h gc.h float_util.h
mod_float.so : mod_float.o float_util.o
mod_math.o : mod_math.c $(ESHFILES) prim.h gc.h
mod_math.so : MODLIBS=-lm
mod_math.so : mod_math.o
mod_json.o : MODCFLAGS=$(MODJSON_CFLAGS)
mod_json.o : mod_json.c $(ESHFILES) prim.h gc.h float_util.h
mod_json.so : MODLIBS=$(MODJSON_LIBS)
mod_json.so : mod_json.o float_util.o

