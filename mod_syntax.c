#include "es.h"
#include "pcre2posix.h"
#include "prim.h"
#include "gc.h"
#include "editor.h"
#include "stdenv.h"

#ifndef TokenizerDebug
#define TokenizerDebug 0
#endif

#ifdef dprint
#undef dprint
#endif

#if TokenizerDebug == 1
#define dprint(args...)                    \
	do {                                   \
		if(editor_debugfd > 0) {           \
			dprintf(editor_debugfd, args); \
		}                                  \
	} while(0)
#else
#define dprint(args...) noop()
#endif

#define REMode (REG_EXTENDED | REG_NOSUB)

typedef struct TokenResults TokenResults;
typedef struct Token Token;
typedef struct SOutBuf SOutBuf;
typedef struct String String;

struct TokenResults {
	int status;
	size_t impsz;
	size_t impi;
	Token *imp;
};

struct Token {
	char *str;
	size_t len;
};

struct SOutBuf {
	char *str;
	size_t len;
	size_t size;
};

struct String {
	size_t len;
	char *data;
};

typedef enum {
	AtomNone = 0,
	AtomBasic = 1,
	AtomNumber = 2,
	AtomKeyword = 3,
	AtomPrimitive = 4,
	AtomFunction = 5,
	AtomVariable = 6,
} AtomType;

extern EditorState *editor;

Arena *syntax_arena = nil;
regex_t prim_regex;
regex_t var_regex;
regex_t basic_regex;
regex_t number_regex;
regex_t string_regex;
regex_t comment_regex;
regex_t whitespace_regex;
regex_t keywords_regex;
regex_t path_regex;

int
syntax_onload(void)
{
	if(pcre2_regcomp(&prim_regex, "^\\$&[a-zA-Z0-9\\-_]+$", REMode) != 0)
		return -1;
	if(pcre2_regcomp(&var_regex, "^\\$+[#\\^\":]?[a-zA-Z0-9\\-_%][a-zA-Z0-9\\-_:%]*$", REMode) != 0)
		return -2;
	if(pcre2_regcomp(&basic_regex, "^[a-zA-Z0-9\\-_%][a-zA-Z0-9\\-_.:%]*$", REMode) != 0)
		return -3;
	if(pcre2_regcomp(&number_regex, "^([0-9]+|0x[0-9a-fA-F]+|0b[01]+|0o[0-7]+)$", REMode) != 0)
		return -4;
	if(pcre2_regcomp(&string_regex, "^'(.|'')*'?$", REMode) != 0)
		return -5;
	if(pcre2_regcomp(&comment_regex, "^#.*$", REMode) != 0)
		return -6;
	if(pcre2_regcomp(&whitespace_regex, "^[ \\t\r\\n]+$", REMode) != 0)
		return -7;
	if(pcre2_regcomp(
		   &keywords_regex,
		   "^(~|~~|local|let|lets|for|fn|%closure|match|matchall|process|%dict|%re|onerror)$",
		   REMode) != 0)
		return -8;
	if(pcre2_regcomp(&path_regex, "^.*/.*$", REMode) != 0)
		return -9;

	return 0;
}

int
syntax_onunload(void)
{
	arena_destroy(syntax_arena);
	set_syntax_highlight_hook(editor, nil);
	pcre2_regfree(&prim_regex);
	pcre2_regfree(&var_regex);
	pcre2_regfree(&basic_regex);
	pcre2_regfree(&number_regex);
	pcre2_regfree(&string_regex);
	pcre2_regfree(&comment_regex);
	pcre2_regfree(&whitespace_regex);
	pcre2_regfree(&keywords_regex);
	pcre2_regfree(&path_regex);

	return 0;
}

String
getString(Term *term)
{
	String res = {0, nil};

	res.data = getstr(term);
	res.len = strlen(res.data);
	return res;
}

void
soutbuf_clean(SOutBuf *buf)
{
	memset(buf->str, 0, buf->len + 1);
	buf->len = 0;
}

void
soutbuf_append(Arena *arena, SOutBuf *buf, char *str, size_t len)
{
	if(buf->str == nil) {
		buf->str = arena_allocate(arena, len + 2);
		buf->size = len + 2;
	}
	if(buf->len + len + 1 > buf->size) {
		buf->str = arena_reallocate(arena, buf->str, buf->size + len + 1);
		buf->size += len + 1;
	}
	memcpy(&buf->str[buf->len], str, len);
	buf->len += len;
}

void
soutbuf_append_color(Arena *arena, SOutBuf *buf, char *str, size_t len)
{
	soutbuf_append(arena, buf, "\x01", 1);
	soutbuf_append(arena, buf, str, len);
	soutbuf_append(arena, buf, "\x02", 1);
}

void
soutbuf_initialize(Arena *arena, SOutBuf *buf, int len)
{
	if(buf->str != nil)
		return;
	buf->str = arena_allocate(arena, len + 2);
	buf->size = len + 2;
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
	case ';':
	case '|':
	case '^':
	case '[':
	case ']':
	case '.':
		return 1;
	case ':':
	case '+':
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
basictokenize(char *tokstr, Arena *arena)
{
	char *instr = nil;
	size_t instrsz = 0;
	char *tmp = nil;
	size_t s = 0, i = 0, e = 0;
	enum { Start, Run, End } op = Start;
	enum { Whitespace, Atom, Symbol, String, Comment, Path, Initialize } state = Initialize;
	TokenResults results;

	memset(&results, 0, sizeof(results));
	results.imp = arena_allocate(arena, sizeof(Token) * 100);
	results.impsz = 100;

	instr = arena_dup(arena, tokstr);
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
		case Path:
			dprint("Path");
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
			else if(instr[i] == '/')
				state = Path;
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
					tmp = arena_ndup(arena, &instr[s], e - s);
					if(results.impi >= results.impsz) {
						results.impsz *= 2;
						results.imp =
							arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
					}
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = e - s;
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
				} else if(instr[i] == '/')
					state = Path;
				else
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
					tmp = arena_ndup(arena, &instr[s], e - s);
					if(results.impi >= results.impsz) {
						results.impsz *= 2;
						results.imp =
							arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
					}
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = e - s;
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
				else if(instr[i] == '/')
					state = Path;
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
					tmp = arena_ndup(arena, &instr[s], e - s);
					if(results.impi >= results.impsz) {
						results.impsz *= 2;
						results.imp =
							arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
					}
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = e - s;
					assert(tmp);
				}
				s = i;
				if(_issymbol(instr[i], 0))
					state = Symbol;
				else if(_iswhitespace(instr[i]))
					state = Whitespace;
				else if(instr[i] == '#')
					state = Comment;
				else if(instr[i] == '/')
					state = Path;
				else if(instr[i] == '\'') {
					state = String;
					op = Start;
				} else
					goto fail;
				continue;
			}
			if(instr[i] == '/') {
				state = Path;
				continue;
			}
			i++;
			break;
		case Symbol:
			if(results.impi >= results.impsz) {
				results.impsz *= 2;
				results.imp = arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
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
			case '^':
			case '[':
			case ']':
			case '>':
			case '.':
				tmp = arena_dup(arena, str("%c", instr[i]));
				assert(tmp);
				results.imp[results.impi].str = tmp;
				results.imp[results.impi++].len = 1;
				i++;
				break;
			case '=':
				if(i + 1 < instrsz && instr[i + 1] == '>') {
					tmp = arena_dup(arena, str("=>"));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 2;
					i += 2;
				} else {
					tmp = arena_dup(arena, str("="));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 1;
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
					tmp = arena_dup(arena, str("|>"));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 2;
					i += 2;
				} else {
					tmp = arena_dup(arena, str("|"));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 1;
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
					tmp = arena_dup(arena, str("<<"));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 2;
					i += 2;
				} else if(instr[i + 1] == '=') {
					tmp = arena_dup(arena, str("<="));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 2;
					i += 2;
				} else {
					tmp = arena_dup(arena, str("<"));
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 1;
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
				if(instr[i + 1] != '=') {
					tmp = arena_dup(arena, str("%c", instr[i]));
					assert(tmp);
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 1;
					i++;
				} else {
					tmp = arena_dup(arena, str("%c=", instr[i]));
					assert(tmp);
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = 2;
					i += 2;
				}
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
			else if(instr[i] == '/')
				state = Path;
			else if(instr[i] == '\'') {
				state = String;
				op = Start;
			} else
				goto fail;
			break;
		case Comment:
			tmp = arena_dup(arena, &instr[s]);
			if(results.impi >= results.impsz) {
				results.impsz *= 2;
				results.imp = arena_reallocate(arena, results.imp, sizeof(char *) * results.impsz);
			}
			results.imp[results.impi].str = tmp;
			results.imp[results.impi++].len = strlen(tmp);
			i = instrsz;
			s = i;
			break;
		case Path:
			if(_iswhitespace(instr[i]) || instr[i] == '#') {
				e = i;
				assert(e >= s);
				if(e - s > 0) {
					tmp = arena_ndup(arena, &instr[s], e - s);
					if(results.impi >= results.impsz) {
						results.impsz *= 2;
						results.imp =
							arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
					}
					results.imp[results.impi].str = tmp;
					results.imp[results.impi++].len = e - s;
					assert(tmp);
				}
				s = i;
				if(_iswhitespace(instr[i]))
					state = Whitespace;
				else if(instr[i] == '#')
					state = Comment;
				else
					goto fail;
				continue;
			}
			i++;
			break;
		}
	}
done:
	e = i;
	if(e - s > 0) {
		tmp = arena_ndup(arena, &instr[s], e - s);
		if(results.impi >= results.impsz) {
			results.impsz +=
				5; /* we're done so we don't need to double. just add a safe amount here */
			results.imp = arena_reallocate(arena, results.imp, sizeof(Token) * results.impsz);
		}
		results.imp[results.impi].str = tmp;
		results.imp[results.impi++].len = strlen(tmp);
	}
	results.status = 0;
	return results;
fail:
	results.status = -1;
	return results;
}

AtomType
syn_isatom(char *teststr)
{
	regmatch_t pmatch[1];

	if(pcre2_regexec(&number_regex, teststr, 0, pmatch, 0) == 0)
		return AtomNumber;
	if(pcre2_regexec(&keywords_regex, teststr, 0, pmatch, 0) == 0)
		return AtomKeyword;
	if(pcre2_regexec(&prim_regex, teststr, 0, pmatch, 0) == 0)
		return AtomPrimitive;
	if(pcre2_regexec(&var_regex, teststr, 0, pmatch, 0) == 0)
		return AtomVariable;
	if(pcre2_regexec(&basic_regex, teststr, 0, pmatch, 0) == 0)
		return AtomBasic;
	return AtomNone;
}

int
syn_iscomment(char *teststr)
{
	regmatch_t pmatch[1];

	if(pcre2_regexec(&comment_regex, teststr, 0, pmatch, 0) == 0)
		return 1;
	return 0;
}

int
syn_isstring(char *teststr)
{
	regmatch_t pmatch[1];

	if(pcre2_regexec(&string_regex, teststr, 0, pmatch, 0) == 0)
		return 1;
	return 0;
}

int
syn_iswhitespace(char *teststr)
{
	regmatch_t pmatch[1];

	if(pcre2_regexec(&whitespace_regex, teststr, 0, pmatch, 0) == 0)
		return 1;
	return 0;
}

int
syn_ispath(char *tok, char *nexttok, char *nextnexttok)
{
	regmatch_t pmatch[1];

	if(pcre2_regexec(&path_regex, tok, 0, pmatch, 0) == 0)
		return 1;
	if(nexttok != nil && streq(tok, ".") && pcre2_regexec(&path_regex, nexttok, 0, pmatch, 0) == 0)
		return 1;
	if(nexttok != nil && nextnexttok != nil && streq(tok, ".") && streq(nexttok, ".") &&
	   pcre2_regexec(&path_regex, nextnexttok, 0, pmatch, 0) == 0)
		return 1;
	return 0;
}

int
valid_primitive(char *str)
{
	size_t strsz = strlen(str);
	char *tstr = nil;

	if(strsz > 2 && str[0] == '$' && str[1] == '&')
		tstr = &str[2];
	else
		tstr = str;

	if(dictget(primitives(), tstr) != nil)
		return 1;
	return 0;
}

int
valid_function(char *str)
{
	if(varlookup2("fn-", str, nil) != nil)
		return 1;
	return 0;
}

AtomType
atom_type(char *str, char *lasttok, char *futuretok)
{
	AtomType type = AtomNone;

	switch((type = syn_isatom(str))) {
	default:
	case AtomNone:
		unreachable();
		break;
	case AtomNumber:
	case AtomKeyword:
	case AtomVariable:
	case AtomFunction:
		return type;
	case AtomPrimitive:
		if(valid_primitive(str))
			return AtomPrimitive;
		return AtomBasic;
	case AtomBasic:
		if(valid_function(str))
			return AtomFunction;
		if(lasttok != nil && streq(lasttok, "fn"))
			return AtomFunction;
		if(futuretok != nil &&
		   (futuretok[0] == '=' || streq(futuretok, ":=") || streq(futuretok, "+=")))
			return AtomVariable;
		return AtomBasic;
	}
}

char *
es_fast_highlighting(char *buffer, size_t bufend)
{
	char *line = nil;
	Dict *synhighlights = nil;
	List *synhigh = nil;
	List *tmp = nil;
	TokenResults results;
	size_t i = 0;
	size_t ii = 0;
	char *lasttok = nil;
	char *futuretok = nil;
	SOutBuf *obuf = nil;
	int colored = 0;

	String colorbasic = {0, nil};
	String colornumber = {0, nil};
	String colorvariable = {0, nil};
	String colorkeyword = {0, nil};
	String colorfunction = {0, nil};
	String colorstring = {0, nil};
	String colorcomment = {0, nil};
	String colorprimitive = {0, nil};
	String colorreset = {.len = 4, .data = "\x1b[0m"};
	String colorpath = {0, nil};

	if(syntax_arena == nil) {
		syntax_arena = newarena(4 * 1024);
	} else
		arena_reset(syntax_arena);
	arena_annotate(syntax_arena, "fasthighlighting");

	gcdisable();

	if((synhigh = varlookup("syntax_conf_colors", nil)) == nil) {
		gcenable();
		return estrdup(line);
	}
	obuf = arena_allocate(syntax_arena, sizeof(SOutBuf));
	soutbuf_initialize(syntax_arena, obuf, 1024);
	synhighlights = getdict(synhigh->term);
	if((tmp = dictget(synhighlights, "basic")) != nil)
		colorbasic = getString(tmp->term);
	if((tmp = dictget(synhighlights, "number")) != nil)
		colornumber = getString(tmp->term);
	if((tmp = dictget(synhighlights, "variable")) != nil)
		colorvariable = getString(tmp->term);
	if((tmp = dictget(synhighlights, "keyword")) != nil)
		colorkeyword = getString(tmp->term);
	if((tmp = dictget(synhighlights, "function")) != nil)
		colorfunction = getString(tmp->term);
	if((tmp = dictget(synhighlights, "string")) != nil)
		colorstring = getString(tmp->term);
	if((tmp = dictget(synhighlights, "comment")) != nil)
		colorcomment = getString(tmp->term);
	if((tmp = dictget(synhighlights, "primitive")) != nil)
		colorprimitive = getString(tmp->term);
	if((tmp = dictget(synhighlights, "path")) != nil)
		colorpath = getString(tmp->term);

	line = arena_ndup(syntax_arena, buffer, bufend);
	results = basictokenize(line, syntax_arena);

	for(i = 0; i < results.impi; i++, colored = 0) {
		if(syn_isstring(results.imp[i].str)) {
			if(colorstring.data)
				soutbuf_append_color(syntax_arena, obuf, colorstring.data, colorstring.len);
			soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
			if(colorstring.data)
				soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
		} else if(syn_iscomment(results.imp[i].str)) {
			if(colorcomment.data)
				soutbuf_append_color(syntax_arena, obuf, colorcomment.data, colorcomment.len);
			soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
			if(colorcomment.data)
				soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
		} else if(syn_ispath(results.imp[i].str, nil, nil)) {
			if(colorpath.data)
				soutbuf_append_color(syntax_arena, obuf, colorpath.data, colorpath.len);
			soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
			if(colorpath.data)
				soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
		} else if(syn_isatom(results.imp[i].str)) {
			for(ii = i + 1, futuretok = nil; ii < results.impi; ii++)
				if(!syn_iswhitespace(results.imp[ii].str)) {
					futuretok = results.imp[ii].str;
					break;
				}
			switch(atom_type(results.imp[i].str, lasttok, futuretok)) {
			default:
				unreachable();
				break;
			case AtomNumber:
				if(colornumber.data) {
					soutbuf_append_color(syntax_arena, obuf, colornumber.data, colornumber.len);
					colored = 1;
				}
				break;
			case AtomKeyword:
				if(colorkeyword.data) {
					soutbuf_append_color(syntax_arena, obuf, colorkeyword.data, colorkeyword.len);
					colored = 1;
				}
				break;
			case AtomVariable:
				if(colorvariable.data) {
					soutbuf_append_color(syntax_arena, obuf, colorvariable.data, colorvariable.len);
					colored = 1;
				}
				break;
			case AtomFunction:
				if(colorfunction.data) {
					soutbuf_append_color(syntax_arena, obuf, colorfunction.data, colorfunction.len);
					colored = 1;
				}
				break;
			case AtomPrimitive:
				if(colorprimitive.data) {
					soutbuf_append_color(syntax_arena, obuf, colorprimitive.data,
										 colorprimitive.len);
					colored = 1;
				}
				break;
			case AtomBasic:
				if(colorbasic.data) {
					soutbuf_append_color(syntax_arena, obuf, colorbasic.data, colorbasic.len);
					colored = 1;
				}
				break;
			}
			soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
			if(colored)
				soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
		} else {
			if(i + 2 < results.impi &&
			   syn_ispath(results.imp[i].str, results.imp[i + 1].str, results.imp[i + 2].str)) {
				if(colorpath.data)
					soutbuf_append_color(syntax_arena, obuf, colorpath.data, colorpath.len);
				soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
				if(colorpath.data)
					soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
			} else if(i + 2 < results.impi && syn_ispath(results.imp[i].str, results.imp[i + 1].str,
														 results.imp[i + 2].str)) {
				if(colorpath.data)
					soutbuf_append_color(syntax_arena, obuf, colorpath.data, colorpath.len);
				soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
				if(colorpath.data)
					soutbuf_append_color(syntax_arena, obuf, colorreset.data, colorreset.len);
			} else {
				soutbuf_append(syntax_arena, obuf, results.imp[i].str, results.imp[i].len);
			}
		}
		if(!syn_iswhitespace(results.imp[i].str))
			lasttok = results.imp[i].str;
	}

	dprint("fasthighlighting arena: size = %lu, used = %lu\n", arena_size(syntax_arena),
		   arena_used(syntax_arena));
	dprint("processed %lu tokens. results.impsz = %lu\n", results.impi, results.impsz);

	gcenable();

	return estrndup(obuf->str, obuf->len);
}

PRIM(basictokenize) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	TokenResults results;
	int64_t i = 0;
	Arena *btarena = nil;

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

	btarena = newarena(1024);
	arena_annotate(btarena, "basictokenizer");
	results = basictokenize(getstr(lp->term), btarena);
	if(results.status < 0)
		goto fail;

	dprint("basictokenize arena: size = %lu, used = %lu\n", arena_size(btarena),
		   arena_used(btarena));
	dprint("generated %lu tokens. results.impsz = %lu\n", results.impi, results.impsz);
	gc();
	for(i = results.impi - 1; i >= 0; i--) {
		assert(results.imp[i].str != nil);
		res = mklist(mkstr(str("%s", results.imp[i].str)), res);
	}

	arena_destroy(btarena);
	gcrderef(&r_res);
	gcrderef(&r_lp);
	dprint("done parsing\n");
	return res;

fail:
	arena_destroy(btarena);
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

	if(list == nil)
		fail("$&syn_isatom", "missing argument");

	teststr = estrdup(getstr(list->term));
	if(syn_isatom(teststr))
		result = list_true;

	efree(teststr);
	return result;
}

PRIM(syn_iscomment) {
	if(list == nil)
		fail("$&syn_iscomment", "missing argument");

	if(syn_iscomment(getstr(list->term)))
		return list_true;

	return list_false;
}

PRIM(syn_isstring) {
	if(list == nil)
		fail("$&syn_isstring", "missing argument");

	if(syn_isstring(getstr(list->term)))
		return list_true;
	return list_false;
}

PRIM(syn_iswhitespace) {
	if(list == nil)
		fail("$&syn_isstring", "missing argument");

	if(syn_iswhitespace(getstr(list->term)))
		return list_true;
	return list_false;
}

PRIM(syn_ispath) {
	if(list == nil)
		fail("$&syn_ispath", "missing argument");

	if(list->next != nil && list->next->next != nil)
		if(syn_ispath(getstr(list->term), getstr(list->next->term), getstr(list->next->next->term)))
			return list_true;
	if(list->next != nil)
		if(syn_ispath(getstr(list->term), getstr(list->next->term), nil))
			return list_true;
	if(syn_ispath(getstr(list->term), nil, nil))
		return list_true;
	return list_false;
}

PRIM(syn_atom_type) {
	if(list == nil)
		fail("$&syn_atom_type", "missing $1, $2, and $3");
	if(list->next == nil)
		fail("$&syn_atom_type", "missing $2 and $3");
	if(list->next->next == nil)
		fail("$&syn_atom_type", "missing $3");

	switch(
		atom_type(getstr(list->term), getstr(list->next->term), getstr(list->next->next->term))) {
	default:
	case AtomNone:
		unreachable();
		break;
	case AtomNumber:
		return mklist(mkstr(str("number")), nil);
	case AtomKeyword:
		return mklist(mkstr(str("keyword")), nil);
	case AtomVariable:
		return mklist(mkstr(str("variable")), nil);
	case AtomPrimitive:
		return mklist(mkstr(str("primitive")), nil);
	case AtomFunction:
		return mklist(mkstr(str("function")), nil);
	case AtomBasic:
		return mklist(mkstr(str("basic")), nil);
	}
}

PRIM(fasthighlighting) {
	char *instr = nil;
	char *res = nil;
	List *reslist = nil; Root r_reslist;

	if(list == nil)
		fail("$&fasthighlighting", "missing argument");

	instr = getstr(list->term);
	res = es_fast_highlighting(instr, strlen(instr));
	gcref(&r_reslist, (void **)&reslist);
	reslist = mklist(mkstr(gcdup(res)), nil);
	efree(res);
	gcrderef(&r_reslist);
	return reslist;
}

PRIM(enablefasthighlighting) {
	set_syntax_highlight_hook(editor, &es_fast_highlighting);
	return list_true;
}

//MODULE(mod_syntax) {
//	// core
//	DX(basictokenize),
//	DX(enablehighlighting),
//	DX(disablehighlighting),
//
//	// accel
//	DX(syn_isatom),
//	DX(syn_iscomment),
//	DX(syn_isstring),
//	DX(syn_iswhitespace),
//	DX(syn_ispath),
//	DX(syn_atom_type),
//	DX(fasthighlighting),
//	DX(enablefasthighlighting),
//
//	PRIMSEND,
//} ENDMODULE(mod_syntax, &syntax_onload, &syntax_onunload)

DEFMODULE(mod_syntax, &syntax_onload, &syntax_onunload,
	// core
	DX(basictokenize),
	DX(enablehighlighting),
	DX(disablehighlighting),

	// accel
	DX(syn_isatom),
	DX(syn_iscomment),
	DX(syn_isstring),
	DX(syn_iswhitespace),
	DX(syn_ispath),
	DX(syn_atom_type),
	DX(fasthighlighting),
	DX(enablefasthighlighting),
);

