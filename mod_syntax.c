#include "es.h"
#include "pcre2posix.h"
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
#define REMode (REG_EXTENDED | REG_NOSUB)

typedef struct TokenResults TokenResults;

struct TokenResults {
	Arena *arena;
	int status;
	size_t impsz;
	size_t impi;
	char **imp;
};

extern EditorState *editor;

regex_t prim_regex;
regex_t var_regex;
regex_t basic_regex;
regex_t number_regex;
regex_t string_regex;
regex_t comment_regex;
regex_t whitespace_regex;
regex_t keywords_regex;

int
dyn_onload(void)
{

	if(pcre2_regcomp(&prim_regex, "^\\$&[a-zA-Z0-9\\-_]+$", REMode) != 0)
		return -1;
	if(pcre2_regcomp(&var_regex, "^\\$+[#\\^\":]?[a-zA-Z0-9\\-_:%]+$", REMode) != 0)
		return -2;
	if(pcre2_regcomp(&basic_regex, "^[a-zA-Z0-9\\-_:%]+$", REMode) != 0)
		return -3;
	if(pcre2_regcomp(&number_regex, "^([0-9]+|0x[0-9a-fA-F]+|0b[01]+|0o[0-7]+)$", REMode) != 0)
		return -4;
	if(pcre2_regcomp(&string_regex, "^'(.|'')*'?$", REMode) != 0)
		return -5;
	if(pcre2_regcomp(&comment_regex, "^#.*$", REMode) != 0)
		return -6;
	if(pcre2_regcomp(&whitespace_regex, "^[ \\t\r\\n]+$", REMode) != 0)
		return -7;
	if(pcre2_regcomp(&keywords_regex, "^(~|~~|local|let|lets|for|fn|%closure|match|matchall|process|%dict|%re|onerror)$", REMode) != 0)
		return -8;

	return 0;
}

int
dyn_onunload(void)
{
	set_syntax_highlight_hook(editor, nil);
	pcre2_regfree(&prim_regex);
	pcre2_regfree(&var_regex);
	pcre2_regfree(&basic_regex);
	pcre2_regfree(&number_regex);
	pcre2_regfree(&string_regex);
	pcre2_regfree(&comment_regex);
	pcre2_regfree(&whitespace_regex);
	pcre2_regfree(&keywords_regex);

	return 0;
}

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

TokenResults
basictokenize(char *tokstr)
{
	char *instr = nil;
	size_t instrsz = 0;
	char *tmp = nil;
	size_t s = 0, i = 0, e = 0;
	enum { Start, Run, End } op = Start;
	enum { Whitespace, Atom, Symbol, String, Comment, Initialize } state = Initialize;
	TokenResults results;

	memset(&results, 0, sizeof(results));
	results.arena = newarena(1024);
	results.imp = arena_allocate(results.arena, sizeof(char*)*100);
	results.impsz = 100;

	results.arena->note = arena_dup(results.arena, "basictokenizer_arena");
	instr = arena_dup(results.arena, tokstr);
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
					tmp = arena_ndup(results.arena, &instr[s], e - s);
					if(results.impi >= results.impsz){
						results.impsz *= 2;
						results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
					}
					results.imp[results.impi++] = tmp;
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
					tmp = arena_ndup(results.arena, &instr[s], e - s);
					if(results.impi >= results.impsz){
						results.impsz *= 2;
						results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
					}
					results.imp[results.impi++] = tmp;
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
					tmp = arena_ndup(results.arena, &instr[s], e - s);
					if(results.impi >= results.impsz){
						results.impsz *= 2;
						results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
					}
					results.imp[results.impi++] = tmp;
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
			if(results.impi >= results.impsz){
				results.impsz *= 2;
				results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
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
				tmp = arena_dup(results.arena, str("%c", instr[i]));
				results.imp[results.impi++] = tmp;
				assert(tmp);
				i++;
				break;
			case '=':
				if(i + 1 < instrsz && instr[i + 1] == '>') {
					tmp = arena_dup(results.arena, str("=>"));
					results.imp[results.impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(results.arena, str("="));
					results.imp[results.impi++] = tmp;
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
					tmp = arena_dup(results.arena, str("|>"));
					results.imp[results.impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(results.arena, str("|"));
					results.imp[results.impi++] = tmp;
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
					tmp = arena_dup(results.arena, str("<<"));
					results.imp[results.impi++] = tmp;
					i += 2;
				} else if(instr[i + 1] == '=') {
					tmp = arena_dup(results.arena, str("<="));
					results.imp[results.impi++] = tmp;
					i += 2;
				} else {
					tmp = arena_dup(results.arena, str("<"));
					results.imp[results.impi++] = tmp;
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
				tmp = arena_dup(results.arena, str("%c="));
				assert(tmp);
				results.imp[results.impi++] = tmp;
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
			tmp = arena_dup(results.arena, &instr[s]);
			if(results.impi >= results.impsz){
				results.impsz *= 2;
				results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
			}
			results.imp[results.impi++] = tmp;
			i = instrsz;
			s = i;
			e = i;
			break;
		}
	}
done:
	e = i;
	if(e - s > 0) {
		tmp = arena_ndup(results.arena, &instr[s], e - s);
		if(results.impi >= results.impsz){
			results.impsz += 5; /* we're done so we don't need to double. just add a safe amount here */
			results.imp = arena_reallocate(results.arena, results.imp, sizeof(char*)*results.impsz);
		}
		results.imp[results.impi++] = tmp;
	}
	results.status = 0;
	return results;
fail:
	results.status = -1;
	return results;
}

PRIM(basictokenize) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	TokenResults results;
	int64_t i = 0;

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

	lp = list;

	results = basictokenize(getstr(lp->term));
	if(status < 0)
		goto fail;

	dprint("basictokenize arena: size = %lu, used = %lu\n", arena_size(results.arena), arena_used(results.arena));
	dprint("generated %lu tokens. results.impsz = %lu\n", results.impi, results.impsz);
	gc();
	for(i = results.impi-1; i >= 0; i--){
		assert(results.imp[i] != nil);
		res = mklist(mkstr(str("%s", results.imp[i])), res);
	}

	arena_destroy(results.arena);
	gcrderef(&r_res);
	gcrderef(&r_lp);
	dprint("done parsing\n");
	return res;

fail:
	arena_destroy(results.arena);
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

PRIM(syn_isatom) {
	List *result = list_false;
	char *teststr = nil;
	regmatch_t pmatch[1];

	if(list == nil)
		fail("$&syn_isatom", "missing argument");

	teststr = estrdup(getstr(list->term));
	if(pcre2_regexec(&prim_regex, teststr, 0, pmatch, 0) == 0)
		result = list_true;
	else if(pcre2_regexec(&var_regex, teststr, 0, pmatch, 0) == 0)
		result = list_true;
	else if(pcre2_regexec(&basic_regex, teststr, 0, pmatch, 0) == 0)
		result = list_true;
	else if(pcre2_regexec(&number_regex, teststr, 0, pmatch, 0) == 0)
		result = list_true;
	else if(pcre2_regexec(&keywords_regex, teststr, 0, pmatch, 0) == 0)
		result = list_true;
	else
		result = list_false;

	free(teststr);
	return result;
}

PRIM(syn_iscomment) {
	regmatch_t pmatch[1];

	if(list == nil)
		fail("$&syn_iscomment", "missing argument");

	if(pcre2_regexec(&comment_regex, getstr(list->term), 0, pmatch, 0) == 0)
		return list_true;
	return list_false;
}


PRIM(syn_isstring) {
	regmatch_t pmatch[1];

	if(list == nil)
		fail("$&syn_isstring", "missing argument");

	if(pcre2_regexec(&string_regex, getstr(list->term), 0, pmatch, 0) == 0)
		return list_true;
	return list_false;
}

PRIM(syn_iswhitespace) {
	regmatch_t pmatch[1];

	if(list == nil)
		fail("$&syn_isstring", "missing argument");

	if(pcre2_regexec(&whitespace_regex, getstr(list->term), 0, pmatch, 0) == 0)
		return list_true;
	return list_false;
}


MODULE(mod_syntax) = {
	DX(basictokenize),
	DX(enablehighlighting),
	DX(disablehighlighting),

	// accel
	DX(syn_isatom),
	DX(syn_iscomment),
	DX(syn_isstring),
	DX(syn_iswhitespace),

	PRIMSEND,
};
