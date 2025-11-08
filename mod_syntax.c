#include "es.h"
#include "prim.h"
#include "gc.h"
#include "editor.h"

#define dprint(args...)                    \
	do {                                   \
		if(editor_debugfd > 0) {           \
			dprintf(editor_debugfd, args); \
		}                                  \
	} while(0)

#define TokenizerDebug 0

extern EditorState *editor;

char *
es_syntax_highlighting_hook(char *buffer, size_t len)
{
	List *hook = nil; Root r_hook;
	List *res = nil; Root r_res;
	List *args = nil; Root r_args;
	char *resstr = nil;

	if(len == 0)
		return nil;

	gcref(&r_hook, (void **)&hook);

	if((hook = varlookup("fn-%syntax_highlight", nil)) == nil) {
		gcrderef(&r_hook);
		return estrndup(buffer, len);
	}

	gcref(&r_args, (void **)&args);
	gcref(&r_res, (void **)&res);

	args = mklist(mkstr(gcndup(buffer, len)), nil);
	hook = append(hook, args);
	res = eval(hook, nil, 0);

	if(res)
		resstr = estrdup(getstr(res->term));

	gcrderef(&r_res);
	gcrderef(&r_args);
	gcrderef(&r_hook);

	return resstr;
}

int
_iswhitespace(char c)
{
	switch(c) {
	case ' ':
	case '\n':
	case '\t':
	case '\r':
		return 1;
	default:
		return 0;
	}
	return 0;
}

int
_issymbol(char c, int inatom)
{
	switch(c) {
	case '{':
	case '}':
	case '(':
	case ')':
	case '<':
	case '>':
	case '=':
	case '+':
	case '/':
	case ';':
	case '|':
	case '^':
	case '[':
	case ']':
	case '.':
		return 1;
	case ':':
		if(inatom)
			return 0;
		return 1;
	}
	return 0;
}

int
_validatom(char c, char last)
{
	if(c & 0b10000000) // utf-8 above basic ascii
		return 1;
	if(last == '$') {
		switch(c) {
		case '^':
		case ':':
		case '"':
		case '#':
			return 1;
		}
	}
	if(_iswhitespace(c))
		return 0;
	if(_issymbol(c, 1))
		return 0;
	if(c == '\'')
		return 0;
	return 1;
}

PRIM(basictokenize) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Arena *tokarena = nil;
	char *instr = nil;
	size_t instrsz = 0;
	char *tmp = nil;
	size_t s = 0, i = 0, e = 0;
	enum { Start, Run, End } op = Start;
	enum { Whitespace, Atom, Symbol, String, Comment, Initialize } state = Initialize;
	char **imp = nil;
	char **newimp = nil;
	size_t impsz = 0;
	size_t impi = 0;
	size_t oldimpsz = 0;
	int64_t pi = 0;

	/* Whitespace = \n, \t, \r, ' '
	 * Atom = ^\$?[#\^":]?[a-zA-Z0-9\-_:]+$
	 * Symbol = { } ( ) | |> = => <= := +=
	 * String = between ' and '
	 * Regex = between %re( and )
	 */

	if(list == nil)
		fail("$&basictokenize", "missing argument");
	if(list->next != nil)
		fail("$&basictokenize", "too many arguments");
	if(list->term->kind != tkString)
		fail("$&basictokenize", "argument must be a string");

	if(strlen(getstr(list->term)) <= 0)
		return mklist(mkstr(gcdup("")), nil);

	gcref(&r_lp, (void **)&r_lp);
	gcref(&r_res, (void **)&r_res);

	tokarena = newarena(1024);
	lp = list;
	imp = arena_allocate(tokarena, sizeof(char*)*100);
	impsz = 100;

	tokarena->note = arena_dup(tokarena, "basictokenizer_arena");
	instr = arena_dup(tokarena, getstr(lp->term));
	instrsz = strlen(instr);

	s = 0;
	dprint("basictokenizer start\n");
	dprint("instr = \"%s\"\n", instr);
	for(i = 0; i < instrsz;) {
#if TokenizerDebug == 1
		dprint("s = %ld, instr[%lu] = %c, state = ", s, i, instr[i]);
		switch(state) {
		default:
			unreachable();
			break;
		case Whitespace:
			dprint("Whitespace");
			break;
		case Atom:
			dprint("Atom");
			break;
		case Symbol:
			dprint("Symbol");
			break;
		case String:
			dprint("String");
			break;
		case Comment:
			dprint("Comment");
			break;
		case Initialize:
			dprint("Initialize");
			break;
		}
		dprint("\n");
#endif

		switch(state) {
		default:
			unreachable();
			break;
		case Initialize:
			if(_iswhitespace(instr[i]))
				state = Whitespace;
			else if(_issymbol(instr[i], 0))
				state = Symbol;
			else if(instr[i] == '#')
				state = Comment;
			else if(_validatom(instr[i], 0))
				state = Atom;
			else if(instr[i] == '\'') {
				state = String;
				op = Start;
			} else
				goto fail;
			break;
		case Whitespace:
			if(!_iswhitespace(instr[i])) {
				e = i;
				assert(e >= s);
				if(e - s > 0) {
					tmp = arena_ndup(tokarena, &instr[s], e - s);
					if(impi >= impsz){
						oldimpsz = impsz;
						impsz *= 2;
						newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
						memcpy(newimp, imp, oldimpsz*sizeof(char*));
						imp = newimp;
					}
					imp[impi++] = tmp;
				}
				s = i;
				if(_issymbol(instr[i], 0))
					state = Symbol;
				else if(instr[i] == '#')
					state = Comment;
				else if(_validatom(instr[i], 0))
					state = Atom;
				else if(instr[i] == '\'') {
					state = String;
					op = Start;
				} else
					goto fail;
				continue;
			}
			i++;
			break;
		case String:
			switch(op) {
			case Start:
				assert(instr[i] == '\'');
				op = Run;
				i++;
				continue;
			case Run:
				if(instr[i] == '\'') {
					if(i + 1 < instrsz && instr[i + 1] == '\'') {
						i += 2;
						continue;
					}
					op = End;
				}
				i++;
				break;
			case End:
				e = i;
				assert(e >= s);
				if(e - s > 0) {
					tmp = arena_ndup(tokarena, &instr[s], e - s);
					if(impi >= impsz){
						oldimpsz = impsz;
						impsz *= 2;
						newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
						memcpy(newimp, imp, oldimpsz*sizeof(char*));
						imp = newimp;
					}
					imp[impi++] = tmp;
					assert(tmp);
				}
				s = i;
				if(_issymbol(instr[i], 0))
					state = Symbol;
				else if(instr[i] == '#')
					state = Comment;
				else if(_validatom(instr[i], 0))
					state = Atom;
				else if(_iswhitespace(instr[i]))
					state = Whitespace;
				else
					goto fail;
				break;
			}
			break;
		case Atom:
			if(!_validatom(instr[i], i >= 1 ? instr[i - 1] : 0)) {
				e = i;
				assert(e >= s);
				if(e - s > 0) {
					tmp = arena_ndup(tokarena, &instr[s], e - s);
					if(impi >= impsz){
						oldimpsz = impsz;
						impsz *= 2;
						newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
						memcpy(newimp, imp, oldimpsz*sizeof(char*));
						imp = newimp;
					}
					imp[impi++] = tmp;
					assert(tmp);
				}
				s = i;
				if(_issymbol(instr[i], 0))
					state = Symbol;
				else if(_iswhitespace(instr[i]))
					state = Whitespace;
				else if(instr[i] == '#')
					state = Comment;
				else if(instr[i] == '\'') {
					state = String;
					op = Start;
				} else
					goto fail;
				continue;
			}
			i++;
			break;
		case Symbol:
			if(impi >= impsz){
				oldimpsz = impsz;
				impsz *= 2;
				newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
				memcpy(newimp, imp, oldimpsz*sizeof(char*));
				imp = newimp;
			}
			switch(instr[i]) {
			default:
				goto fail;
			case '{':
			case '}':
			case '(':
			case ')':
			case '\\':
			case ';':
			case '/':
			case '^':
			case '[':
			case ']':
			case '>':
			case '.':
				tmp = arena_dup(tokarena, str("%c", instr[i]));
				imp[impi++] = tmp;
				assert(tmp);
				i++;
				break;
			case '=':
				if(i + 1 < instrsz && instr[i + 1] == '>') {
					tmp = arena_dup(tokarena, str("=>"));
					imp[impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(tokarena, str("="));
					imp[impi++] = tmp;
					i++;
				}
				assert(tmp);
				break;
			case '|':
				if(i + 1 >= instrsz) {
					i++;
					goto done;
				}
				if(instr[i + 1] == '>') {
					tmp = arena_dup(tokarena, str("|>"));
					imp[impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(tokarena, str("|"));
					imp[impi++] = tmp;
					i++;
				}
				assert(tmp);
				break;
			case '<':
				if(i + 1 >= instrsz) {
					i++;
					goto done;
				}
				if(instr[i + 1] == '<') {
					tmp = arena_dup(tokarena, str("<<"));
					imp[impi++] = tmp;
					i += 2;
				} else if(instr[i + 1] == '=') {
					tmp = arena_dup(tokarena, str("<="));
					imp[impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(tokarena, str("<"));
					imp[impi++] = tmp;
					i++;
				}
				assert(tmp);
				break;
			case ':':
			case '+':
				if(i + 1 >= instrsz) {
					i++;
					goto done;
				}
				if(instr[i + 1] != '=')
					goto fail;
				tmp = arena_dup(tokarena, str("%c="));
				assert(tmp);
				imp[impi++] = tmp;
				i += 2;
				break;
			}
			s = i;
			if(_iswhitespace(instr[i]))
				state = Whitespace;
			else if(_issymbol(instr[i], 0))
				state = Symbol;
			else if(instr[i] == '#')
				state = Comment;
			else if(_validatom(instr[i], 0))
				state = Atom;
			else if(instr[i] == '\'') {
				state = String;
				op = Start;
			} else
				goto fail;
			break;
		case Comment:
			tmp = arena_dup(tokarena, &instr[s]);
			if(impi >= impsz){
				oldimpsz = impsz;
				impsz *= 2;
				newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
				memcpy(newimp, imp, oldimpsz*sizeof(char*));
				imp = newimp;
			}
			imp[impi++] = tmp;
			i = instrsz;
			s = i;
			e = i;
			break;
		}
	}
done:
	e = i;
	if(e - s > 0) {
		tmp = arena_ndup(tokarena, &instr[s], e - s);
		if(impi >= impsz){
			oldimpsz = impsz;
			impsz *= 2;
			newimp = arena_allocate(tokarena, sizeof(char*)*impsz);
			memcpy(newimp, imp, oldimpsz*sizeof(char*));
			imp = newimp;
		}
		imp[impi++] = tmp;
	}

	dprint("basictokenize arena: size = %lu, used = %lu\n", arena_size(tokarena), arena_used(tokarena));
	dprint("generated %lu tokens. impsz = %lu\n", impi, impsz);
	gc();
	for(pi = impi-1; pi >= 0; pi--){
		assert(imp[pi] != nil);
		res = mklist(mkstr(str("%s", imp[pi])), res);
	}

	arena_destroy(tokarena);
	gcrderef(&r_res);
	gcrderef(&r_lp);
	dprint("done parsing\n");
	return res;

fail:
	arena_destroy(tokarena);
	gcrderef(&r_res);
	gcrderef(&r_lp);
	fail("$&basictokenize", "tokenizer failure");
	unreachable();
	return nil;
}

PRIM(enablehighlighting) {
	set_syntax_highlight_hook(editor, &es_syntax_highlighting_hook);
	return list_true;
}

PRIM(disablehighlighting) {
	set_syntax_highlight_hook(editor, nil);
	return list_true;
}

MODULE(mod_syntax) = {
	DX(basictokenize),
	DX(enablehighlighting),
	DX(disablehighlighting),

	PRIMSEND,
};
