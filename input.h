/* input.h -- definitions for es lexical analyzer ($Revision: 1.1.1.1 $) */

#ifndef __es_input_h
#define __es_input_h

#include "gc.h"

#define MAXUNGET 2	   /* maximum 2 character pushback */
#define MAXTOKBUF 4096 // defined in input.c

typedef struct Input Input;
struct Input {
	int (*get)(Input *self);
	int (*fill)(Input *self), (*rfill)(Input *self);
	void (*cleanup)(Input *self);
	Input *prev;
	const char *name;
	unsigned char *buf, *bufend, *bufbegin, *rbuf;
	size_t buflen;
	int unget[MAXUNGET];
	int ungot;
	int lineno;
	int fd;
	int runflags;
	char tokstatus[MAXTOKBUF];
	char *lasttokstatus;
	size_t tokstatusi;
	Arena *arena;
};

/* input.c */

extern Input *input;
extern void unget(Input *in, int c);
extern void input_ungetc(int c);
extern int input_getc(void);
extern void input_resettokstatus(void);
extern char *input_dumptokstatus(void);
extern Boolean disablehistory;
extern void yyerror(char *s);

/* token.c */

extern const char dnw[];
extern int yylex(void);
extern void inityy(void);
extern void print_prompt2(void);

/* parse.y */

extern Tree *parsetree;
extern int yyparse(void);
extern void initparse(void);

/* heredoc.c */

extern void emptyherequeue(void);

#endif
