/* parse.y -- grammar for es ($Revision: 1.2 $) */

%{
/* Some yaccs insist on including stdlib.h */
#include "es.h"
#include "input.h"
#include "syntax.h"
%}

%union {
	Tree *tree;
	char *str;
	NodeKind kind;
}

%token <str>	WORD
%token <str>	QWORD
%token <tree>	REDIR
%token <tree>	PIPE
%token <tree> 	DUP
%token	LOCAL LET LETS FOR CLOSURE FN
%token	ANDAND BACKBACK STBACK STRLIST FUNPIPE
%token	EXTRACT CALL COUNT FLAT OROR TOSTR PRIM SUB
%token	NL ENDFILE ERROR MATCH MATCHALL PROCESS TRY
%token	DICTASSOC DICT DICTASSIGN APPENDASSIGN

%left	LOCAL LET LETS FOR CLOSURE ')'
%left	ANDAND OROR NL MATCH MATCHALL PROCESS
%left	'!'
%left	PIPE FUNPIPE ONERR
%right	'$' TOSTR
%left	SUB

%type <str> 	keyword
%type <tree>	body cmd cmdsa cmdsan comword first fn line word param assign
				binding bindings params nlwords words simple redir sword
				cases case assocs assoc dictassign appendassign
%type <kind>	binder

%start es

%%

es	: line end		{ parsetree = $1; YYACCEPT; }
	| error end		{ yyerrok; parsetree = NULL; YYABORT; }

end	: NL			{ if (!readheredocs(FALSE)) YYABORT; }
	| ENDFILE		{ if (!readheredocs(TRUE)) YYABORT; }

line	: cmd			{ $$ = $1; }
	| cmdsa line		{ $$ = mkseq("%seq", $1, $2); }

body	: cmd			{ $$ = $1; }
	| cmdsan body		{ $$ = mkseq("%seq", $1, $2); }

cmdsa	: cmd ';'		{ $$ = $1; }
	| cmd '&'		{ $$ = prefix("%bghook", mk(nList, thunkify($1), NULL)); }
	| cmd TRY		{ $$ = prefix("try", mk(nList, thunkify($1), NULL)); }

cmdsan	: cmdsa			{ $$ = $1; }
	| cmd NL		{ $$ = $1; if (!readheredocs(FALSE)) YYABORT; }

cmd	:		%prec LET		{ $$ = NULL; }
	| simple				{ $$ = redirect($1); if ($$ == &errornode) YYABORT; }
	| redir cmd	%prec '!'		{ $$ = redirect(mk(nRedir, $1, $2)); if ($$ == &errornode) YYABORT; }
	| first assign				{ $$ = mk(nAssign, $1, $2); }
	| first dictassign		{ $$ = mkdictassign($1, $2); }
	| first appendassign	{ $$ = mkappendassign($1, $2); }
	| fn					{ $$ = $1; }
	| binder nl '(' bindings ')' nl cmd	{ $$ = mk($1, $4, $7); }
	| cmd ANDAND nl cmd			{ $$ = mkseq("%and", $1, $4); }
	| cmd OROR nl cmd			{ $$ = mkseq("%or", $1, $4); }
 	| cmd PIPE nl cmd			{ $$ = mkpipe($1, $2->u[0].i, $2->u[1].i, $4); }
	| cmd FUNPIPE nl cmd		{ $$ = mkfunpipe($1, $4); }
	| cmd ONERR nl cmd			{ $$ = mkonerror($1, $4); }
	| '!' caret cmd				{ $$ = prefix("%not", mk(nList, thunkify($3), NULL)); }
	| '~' word words			{ $$ = mk(nMatch, $2, $3); }
	| EXTRACT word words			{ $$ = mk(nExtract, $2, $3); }
	| MATCH word nl '(' cases ')' {$$ = mkmatch($2, $5); }
	| MATCHALL word nl '(' cases ')' {$$ = mkmatchall($2, $5); }
	| PROCESS word nl '(' cases ')' {$$ = mkprocess($2, $5); }

cases : case			{ $$ = treecons2($1, NULL); }
	  | cases ';' case        { $$ = treeconsend2($1, $3); }
	  | cases NL case         { $$ = treeconsend2($1, $3); }

case : 					{ $$ = NULL; }
	 | word first		{ $$ = mk(nList, $1, thunkify($2)); }

simple	: first				{ $$ = treecons2($1, NULL); }
	| simple word			{ $$ = treeconsend2($1, $2); }
	| simple redir			{ $$ = redirappend($1, $2); }

redir	: DUP				{ $$ = $1; }
	| REDIR word			{ $$ = mkredir($1, $2); }

bindings: binding			{ $$ = treecons2($1, NULL); }
	| bindings ';' binding		{ $$ = treeconsend2($1, $3); }
	| bindings NL binding		{ $$ = treeconsend2($1, $3); }

binding	:				{ $$ = NULL; }
	| fn				{ $$ = $1; }
	| word assign			{ $$ = mk(nAssign, $1, $2); }

assign	: caret '=' caret words		{ $$ = $4; }

appendassign : caret APPENDASSIGN caret words { $$ = $4; }

dictassign : caret DICTASSIGN assoc { $$ = $3; }

assocs : { $$ = NULL; } 
	   | assoc { $$ = treecons2($1, NULL); }
	   | assocs ';' assoc { $$ = treeconsend2($1, $3); }
	   | assocs NL assoc { $$ = treeconsend2($1, $3); }
	   | assocs ';' { $$ = $1; }
	   | assocs NL { $$ = $1; }

assoc : word DICTASSOC words { $$ = mk(nAssoc, $1, $3); }

fn	: FN word params '{' body '}'	{ $$ = fnassign($2, $3, $5); }
	| FN word			{ $$ = fnassign($2, NULL, NULL); }

first	: comword			{ $$ = $1; }
	| first '^' sword		{ $$ = mk(nConcat, $1, $3); }

sword	: comword			{ $$ = $1; }
	| keyword			{ $$ = mk(nWord, $1); }

word	: sword				{ $$ = $1; }
	| word '^' sword		{ $$ = mk(nConcat, $1, $3); }

comword	: param				{ $$ = $1; }
	| DICT '(' assocs ')'		{ $$ = mk(nDict, $3); }
	| '(' nlwords ')'		{ $$ = $2; }
	| '{' body '}'			{ $$ = thunkify($2); }
	| '@' params '{' body '}'	{ $$ = mklambda($2, $4); }
	| '$' sword			{ $$ = mk(nVar, $2); }
	| '$' sword SUB words ')'	{ $$ = mk(nVarsub, $2, $4); }
	| CALL sword			{ $$ = mk(nCall, $2); }
	| COUNT sword			{ $$ = mk(nCall, prefix("%count", treecons(mk(nVar, $2), NULL))); }
	| FLAT sword			{ $$ = flatten(mk(nVar, $2), " "); }
	| PRIM WORD			{ $$ = mk(nPrim, $2); }
	| TOSTR sword		{ $$ = mk(nCall, prefix("%string", treecons(mk(nVar, $2), NULL))); }
	| TOSTR sword SUB words ')' { $$ = mk(nCall, prefix("%string", treecons(mk(nVarsub, $2, $4), NULL))); }
	| STRLIST sword		{ $$ = mk(nCall, prefix("%strlist", treecons(mk(nVar, $2), NULL))); }
	| '`' sword			{ $$ = backquote(mk(nVar, mk(nWord, "ifs")), $2); }
	| BACKBACK word	sword		{ $$ = backquote($2, $3); }
	| STBACK sword { $$ = stbackquote(mk(nVar, mk(nWord, "ifs")), $2); }

param	: WORD				{ $$ = mk(nWord, $1); }
	| QWORD				{ $$ = mk(nQword, $1); }

params	:				{ $$ = NULL; }
	| params param			{ $$ = treeconsend($1, $2); }

words	:				{ $$ = NULL; }
	| words word			{ $$ = treeconsend($1, $2); }

nlwords :				{ $$ = NULL; }
	| nlwords word			{ $$ = treeconsend($1, $2); }
	| nlwords NL			{ $$ = $1; }

nl	:
	| nl NL

caret 	:
	| '^'

binder	: LOCAL	        { $$ = nLocal; }
	| LET		{ $$ = nLet; }
	| LETS		{ $$ = nLets; }
	| FOR		{ $$ = nFor; }
	| CLOSURE	{ $$ = nClosure; }

keyword	: '!'		{ $$ = "!"; }
	| '~'		{ $$ = "~"; }
	| EXTRACT	{ $$ = "~~"; }
	| LOCAL 	{ $$ = "local"; }
	| LET		{ $$ = "let"; }
	| LETS		{ $$ = "lets"; }
	| FOR		{ $$ = "for"; }
	| FN		{ $$ = "fn"; }
	| CLOSURE	{ $$ = "%closure"; }
	| MATCH		{ $$ = "match"; }
	| MATCHALL	{ $$ = "matchall"; }
	| PROCESS	{ $$ = "process"; }

