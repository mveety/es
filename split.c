/* split.c -- split strings based on separators ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"


void
initsplitctx(SplitCtx *ctx)
{
	memset(ctx, 0, sizeof(SplitCtx));
	ctx->initialized = FALSE;
	ctx->ifsvalid = FALSE;
	ctx->value = nil;
	gcref(&ctx->r_value, (void**)&ctx->value);
}

void
startsplit(SplitCtx *ctx, const char *sep, Boolean coalescef)
{
	ctx->value = NULL;
	ctx->buffer = NULL;
	ctx->coalesce = coalescef;
	ctx->splitchars = !ctx->coalesce && *sep == '\0';

	if(!ctx->ifsvalid || !streq(sep, ctx->ifs)) {
		int c;
		if(strlen(sep) + 1 < sizeof(ctx->ifs)) {
			strcpy(ctx->ifs, sep);
			ctx->ifsvalid = TRUE;
		} else
			ctx->ifsvalid = FALSE;
		memzero(ctx->isifs, sizeof(ctx->isifs));
		for(ctx->isifs['\0'] = TRUE; (c = (*(unsigned const char *)sep)) != '\0'; sep++)
			ctx->isifs[c] = TRUE;
	}
}

extern char *
stepsplit(SplitCtx *ctx, char *in, size_t len, Boolean endword)
{
	Buffer *buf = ctx->buffer;
	unsigned char *s = (unsigned char *)in, *inend = s + len;

	if(ctx->splitchars) {
		Boolean end;
		Term *term;

		if(*s == '\0')
			return NULL;
		assert(buf == NULL);

		end = *(s + 1) == '\0';

		term = mkstr(gcndup((char *)s, 1));
		ctx->value = mklist(term, ctx->value);

		if(end)
			return NULL;
		return (char *)++s;
	}

	if(!ctx->coalesce && buf == NULL)
		buf = openbuffer(0);

	while(s < inend) {
		int c = *s++;
		if(buf != NULL)
			if(ctx->isifs[c]) {
				Term *term = mkstr(sealcountedbuffer(buf));
				ctx->value = mklist(term, ctx->value);
				ctx->buffer = ctx->coalesce ? NULL : openbuffer(0);
				return (char *)s;
			} else
				buf = bufputc(buf, c);
		else if(!ctx->isifs[c])
			buf = bufputc(openbuffer(0), c);
	}

	if(endword && buf != NULL) {
		Term *term = mkstr(sealcountedbuffer(buf));
		ctx->value = mklist(term, ctx->value);
		buf = NULL;
	}
	ctx->buffer = buf;
	return NULL;
}

extern void
splitstring(SplitCtx *ctx, char *in, size_t len, Boolean endword)
{
	size_t remainder;
	char *s = in;
	do {
		remainder = len - (s - in);
		s = stepsplit(ctx, s, remainder, endword);
	} while(s != NULL);
}

extern List *
endsplit(SplitCtx *ctx)
{
	List *result;

	if(ctx->buffer != NULL) {
		Term *term = mkstr(sealcountedbuffer(ctx->buffer));
		ctx->value = mklist(term, ctx->value);
		ctx->buffer = NULL;
	}
	result = reverse(ctx->value);
	ctx->value = NULL;
	gcrderef(&ctx->r_value);
	return result;
}

extern List *
fsplit(const char *sep, List *list, Boolean coalesce)
{
	SplitCtx ctx;

	initsplitctx(&ctx);
	Ref(List *, lp, list);
	startsplit(&ctx, sep, coalesce);
	for(; lp != NULL; lp = lp->next) {
		char *bs = getstr(lp->term), *s = bs;
		do {
			char *ns = getstr(lp->term);
			s = ns + (s - bs);
			bs = ns;
			s = stepsplit(&ctx, s, strlen(s), TRUE);
		} while(s != NULL);
	}
	RefEnd(lp);
	return endsplit(&ctx);
}
