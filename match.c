/* match.c -- pattern matching routines ($Revision: 1.1.1.1 $) */

#include "es.h"

enum { RANGE_FAIL = -1, RANGE_ERROR = -2 };

/*
   From the ed(1) man pages (on ranges):

	The `-' is treated as an ordinary character if it occurs first
	(or first after an initial ^) or last in the string.

	The right square bracket does not terminate the enclosed string
	if it is the first character (after an initial `^', if any), in
	the bracketed string.

   rangematch() matches a single character against a class, and returns
   an integer offset to the end of the range on success, or -1 on
   failure.
*/

Boolean verbose_rangematch = FALSE;

int
isquoted(const char *q, size_t qi, size_t quotelen)
{
	if(verbose_rangematch)
		dprint("isquoted: q = \"%s\", qi = %lu, quotelen = %lu\n", q, qi, quotelen);
	if(q == QUOTED)
		return 1;
	if(q == UNQUOTED)
		return 0;
	if(qi < quotelen && q[qi] == 'q')
		return 1;
	return 0;
}

char *
tailquote(const char *q, size_t qi, size_t quotelen)
{
	if(q == UNQUOTED)
		return UNQUOTED;
	if(qi < quotelen)
		return (char *)&q[qi];
	return UNQUOTED;
}

/* rangematch -- match a character against a character class */
static int
rangematch(const char *p, const char *q, char c)
{
	const char *orig = p;
	Boolean neg;
	Boolean matched = FALSE;
	size_t quotelen;
	size_t qi = 0;
	int iq = 0;

	quotelen = strlen(q);

	/* it doesn't work without the dprintf's (at least on freebsd with clang -O2) */
	/* i think it's fixed now? */
	if(verbose_rangematch)
		dprint("rangematch: p = \"%s\", q = \"%s\", c = %c\n", p, q, c);
	if(*p == '~' && !isquoted(q, qi, quotelen)) {
		p++;
		if(q != UNQUOTED && q != QUOTED)
			q++;
		neg = TRUE;
	} else
		neg = FALSE;
	if(*p == ']' && !isquoted(q, qi, quotelen)) {
		p++;
		if(q != UNQUOTED && q != QUOTED)
			q++;
		matched = (c == ']');
	}
	for(; *p != ']' || (iq = isquoted(q, qi, quotelen)); p++, qi++) {
		if(verbose_rangematch) {
			dprint("rangematch: p = \"%s\", q = \"%s\", qi = %lu, c = %c", p, q, qi, c);
			dprint(", matched = %s", matched == TRUE ? "true" : "false");
			dprint(", isquoted = %d\n", iq);
		}
		if(*p == '\0')
			return RANGE_ERROR; /* bad syntax */
		if(p[1] == '-' && !isquoted(q, qi + 1, quotelen) &&
		   ((p[2] != ']' && p[2] != '\0') || isquoted(q, qi + 2, quotelen))) {
			/* check for [..-..] but ignore [..-] */
			if(c >= *p && c <= p[2])
				matched = TRUE;
			p += 2;
			if(q != UNQUOTED && q != QUOTED)
				q += 2;
		} else if(*p == c)
			matched = TRUE;
	}
	if(matched ^ neg)
		return p - orig + 1; /* skip the right-bracket */
	else
		return RANGE_FAIL;
}

/* match -- match a single pattern against a single string. */
extern Boolean
match(const char *s, const char *p, const char *q)
{
	int i;
	size_t qlen;

	qlen = strlen(q);
	if(q == QUOTED)
		return streq(s, p);
	for(i = 0;;) {
		int c = p[i++];
		if(c == '\0')
			return *s == '\0';
		else if(q == UNQUOTED || q[i - 1] == 'r') {
			switch(c) {
			case '?':
				if(*s++ == '\0')
					return FALSE;
				break;
			case '*':
				while(p[i] == '*' && (q == UNQUOTED || q[i] == 'r')) /* collapse multiple stars */
					i++;
				if(p[i] == '\0') /* star at end of pattern? */
					return TRUE;
				while(*s != '\0')
					if(match(s++, p + i, tailquote(q, i, qlen)))
						return TRUE;
				return FALSE;
			case '[':
				{
					int j;
					if(*s == '\0')
						return FALSE;
					switch(j = rangematch(p + i, tailquote(q, i, qlen), *s)) {
					default:
						i += j;
						break;
					case RANGE_FAIL:
						return FALSE;
					case RANGE_ERROR:
						if(*s != '[')
							return FALSE;
					}
					s++;
					break;
				}
			default:
				if(c != *s++)
					return FALSE;
			}
		} else if(c != *s++)
			return FALSE;
	}
}

/*
 * listmatch
 *
 * 	Matches a list of words s against a list of patterns p.
 *	Returns true iff a pattern in p matches a word in s.
 *	() matches (), but otherwise null patterns match nothing.
 */

extern Boolean
listmatch(List *subject, List *pattern, StrList *quote)
{
	if(subject == NULL) {
		if(pattern == NULL)
			return TRUE;
		Ref(List *, p, pattern);
		Ref(StrList *, q, quote);
		for(; p != NULL; p = p->next, q = q->next) {
			/* one or more stars match null */
			char *pw = getstr(p->term), *qw = q->str;
			if(*pw != '\0' && qw != QUOTED) {
				int i;
				Boolean matched = TRUE;
				for(i = 0; pw[i] != '\0'; i++)
					if(pw[i] != '*' || (qw != UNQUOTED && qw[i] != 'r')) {
						matched = FALSE;
						break;
					}
				if(matched) {
					RefPop2(q, p);
					return TRUE;
				}
			}
		}
		RefEnd2(q, p);
		return FALSE;
	}

	Ref(List *, s, subject);
	Ref(List *, p, pattern);
	Ref(StrList *, q, quote);

	for(; p != NULL; p = p->next, q = q->next) {
		assert(q != NULL);
		assert(p->term != NULL);
		assert(q->str != NULL);
		Ref(char *, pw, getstr(p->term));
		Ref(char *, qw, q->str);
		Ref(List *, t, s);
		for(; t != NULL; t = t->next) {
			char *tw = getstr(t->term);
			if(match(tw, pw, qw)) {
				RefPop3(t, qw, pw);
				RefPop3(q, p, s);
				return TRUE;
			}
		}
		RefEnd3(t, qw, pw);
	}
	RefEnd3(q, p, s);
	return FALSE;
}

/*
 * extractsinglematch -- extract matching parts of a single subject and a
 *			 single pattern, returning it backwards
 */

List *
extractsinglematch(char *subject, char *pattern, char *quoting, List *result)
{

	size_t i;
	size_t subjectlen;
	size_t si = 0;
	size_t patternlen;
	size_t quotelen;
	size_t begin;
	char *q = nil;
	int c, j;

	subjectlen = strlen(subject);
	patternlen = strlen(pattern);
	quotelen = strlen(quoting);

	if(!haswild(pattern, quoting) || !match(subject, pattern, quoting)) {
		result = nil;
		goto done;
	}

	for(si = 0, i = 0; pattern[i] != '\0' && i < patternlen && si < subjectlen; si++) {
		if(isquoted(quoting, i, quotelen))
			i++;
		else {
			c = pattern[i++];
			switch(c) {
			case '*':
				if(pattern[i] == '\0') {
					result = mklist(mkstr(gcdup(&subject[si])), result);
					goto done;
				}
				for(begin = si;; si++) {
					/* q = TAILQUOTE(quoting, i);*/
					q = tailquote(quoting, i, quotelen);
					assert(subject[si] != '\0');
					if(match(&subject[si], &pattern[i], q)) {
						result = mklist(mkstr(gcndup(&subject[begin], si - begin)), result);
						if(haswild(&pattern[i], q)) {
							result = extractsinglematch(&subject[si], &pattern[i], q, result);
							goto done;
						} else
							goto done;
					}
				}
				break;
			case '[':
				j = rangematch(&pattern[i], tailquote(quoting, i, quotelen), subject[si]);
				assert(j != RANGE_FAIL);
				if(j == RANGE_ERROR) {
					assert(subject[si] == '[');
					break;
				}
				i += j;
				fallthrough;
			case '?':
				result = mklist(mkstr(str("%c", subject[si])), result);
				break;
			default:
				break;
			}
		}
	}

done:
	return result;
}

/*
 * extractmatches
 *
 *	Compare subject and patterns like listmatch().  For all subjects
 *	that match a pattern, return the wildcarded portions of the
 *	subjects as the result.
 */

List * /* new implementation that adds regex and lets the gc run */
extractmatches(List *subjects0, List *patterns0, StrList *quotes0)
{
	List *subjects = nil; Root r_subjects;
	List *subject = nil; Root r_subject;
	List *patterns = nil; Root r_patterns;
	List *pattern = nil; Root r_pattern;
	StrList *quotes = nil; Root r_quotes;
	StrList *quote = nil; Root r_quote;
	List *result = nil; Root r_result;
	List *rlp = nil; Root r_rlp;
	List *match = nil; Root r_match;
	RegexStatus status;
	char errstr[128];
	AppendContext ctx;

	gcref(&r_subjects, (void **)&subjects);
	gcref(&r_subject, (void **)&subject);
	gcref(&r_patterns, (void **)&patterns);
	gcref(&r_pattern, (void **)&pattern);
	gcref(&r_quotes, (void **)&quotes);
	gcref(&r_quote, (void **)&quote);
	gcref(&r_result, (void **)&result);
	gcref(&r_rlp, (void **)&rlp);
	gcref(&r_match, (void **)&match);
	append_start(&ctx);

	subjects = subjects0;
	patterns = patterns0;
	quotes = quotes0;

	for(subject = subjects; subject != nil; subject = subject->next) {
		for(pattern = patterns, quote = quotes; pattern != nil;
			pattern = pattern->next, quote = quote->next) {
			if(pattern->term->kind == tkRegex) {
				memset(errstr, 0, sizeof(errstr));
				status = (RegexStatus){ReNil, FALSE, 0, 0, nil, 0, &errstr[0], sizeof(errstr)};
				regexextract(&status, subject->term, pattern->term);
				assert(status.type == ReExtract);

				if(status.compcode)
					fail("es:reextract", "compilation error: %s", errstr);
				if(status.matchcode != 0 && status.matchcode != REG_NOMATCH)
					fail("es:reextract", "match error: %s", errstr);

				if(status.matched == TRUE && status.substrs->next != nil) {
					partial_append(&ctx, status.substrs->next);
					break;
				}
			} else {
				gcdisable();
				match = nil;
				match = extractsinglematch(getstr(subject->term), getstr(pattern->term), quote->str,
										   nil);
				gcenable();
				if(match != nil) {
					match = reverse(match);
					partial_append(&ctx, match);
					break;
				}
			}
		}
	}

	result = append_end(&ctx);
	gcrderef(&r_match);
	gcrderef(&r_rlp);
	gcrderef(&r_result);
	gcrderef(&r_quote);
	gcrderef(&r_quotes);
	gcrderef(&r_pattern);
	gcrderef(&r_patterns);
	gcrderef(&r_subject);
	gcrderef(&r_subjects);

	return result;
}

Boolean regex_debug = FALSE;

RegexStatus *
regexmatch(RegexStatus *status, Term *subject0, Term *pattern0)
{
	Term *subject = subject0; Root r_subject;
	Term *pattern = pattern0; Root r_pattern;
	char *subjectstr = nil;
	char *patternstr = nil;
	regex_t regex;
	regmatch_t pmatch[1];

	gcref(&r_subject, (void **)&subject);
	gcref(&r_pattern, (void **)&pattern);

	status->type = ReMatch;
	subjectstr = estrdup(getregex(subject));
	patternstr = estrdup(getregex(pattern));
	status->compcode = pcre2_regcomp(&regex, patternstr, REG_EXTENDED | REG_NOSUB);
	if(status->compcode != 0) {
		if(status->errstrsz > 0)
			pcre2_regerror(status->compcode, &regex, status->errstr, status->errstrsz - 1);
		goto done;
	}

	status->matchcode = pcre2_regexec(&regex, subjectstr, 0, pmatch, 0);
	if(status->matchcode == 0)
		status->matched = TRUE;
	else {
		if(status->errstrsz > 0)
			pcre2_regerror(status->compcode, &regex, status->errstr, status->errstrsz - 1);
	}

done:
	pcre2_regfree(&regex);
	efree(subjectstr);
	efree(patternstr);
	gcrderef(&r_pattern);
	gcrderef(&r_subject);
	return status;
}

RegexStatus *
regexextract(RegexStatus *status, Term *subject0, Term *pattern0)
{
	Term *subject = subject0; Root r_subject;
	Term *pattern = pattern0; Root r_pattern;
	List *substrs = nil; Root r_substrs;
	char *subjectstr = nil;
	char *patternstr = nil;
	char *copybuf = nil;
	size_t copybufsz = 0;
	Root r_status_substrs;
	regex_t regex;
	regmatch_t *pmatch;
	size_t nmatch;
	int i;

	gcref(&r_subject, (void **)&subject);
	gcref(&r_pattern, (void **)&pattern);
	gcref(&r_substrs, (void **)&substrs);
	gcref(&r_status_substrs, (void **)&status->substrs);

	status->type = ReExtract;
	subjectstr = estrdup(getregex(subject));
	patternstr = estrdup(getregex(pattern));
	copybufsz = strlen(subjectstr);
	copybuf = ealloc(copybufsz);

	status->compcode = pcre2_regcomp(&regex, patternstr, REG_EXTENDED);
	if(status->compcode > 0) {
		if(status->errstrsz > 0)
			pcre2_regerror(status->compcode, &regex, status->errstr, status->errstrsz - 1);
		goto done;
	}
	nmatch = regex.re_nsub + 1;
	pmatch = ealloc(nmatch * sizeof(regmatch_t));

	status->matchcode = pcre2_regexec(&regex, subjectstr, regex.re_nsub, pmatch, 0);
	if(status->matchcode) {
		if(status->errstrsz > 0)
			pcre2_regerror(status->compcode, &regex, status->errstr, status->errstrsz - 1);
		goto done;
	}
	status->matched = TRUE;
	for(i = (nmatch - 1); i >= 0; i--) {
		if(pmatch[i].rm_so < 0 && pmatch[i].rm_eo < 0)
			continue;
		memset(copybuf, 0, copybufsz);
		memcpy(copybuf, (subjectstr + pmatch[i].rm_so), pmatch[i].rm_eo - pmatch[i].rm_so);
		if(strlen(copybuf) == 0)
			continue;
		if(regex_debug == TRUE) {
			dprint("pmatch[%d].rm_so = %lu, pmatch[%d].rm_eo = %lu, ", i, (uint64_t)pmatch[i].rm_so,
				   i, (uint64_t)pmatch[i].rm_eo);
			dprint("pmatch[%d].rm_eo - pmatch[%d].rm_so = %lu, ", i, i,
				   (uint64_t)(pmatch[i].rm_eo - pmatch[i].rm_so));
			dprint("i = %d, nmatch = %lu, copybuf = %s\n", i, nmatch, copybuf);
		}
		substrs = mklist(mkstr(str("%s", copybuf)), substrs);
	}

	status->substrs = substrs;
	status->nsubstr = nmatch;

done:
	pcre2_regfree(&regex);
	efree(copybuf);
	efree(subjectstr);
	efree(patternstr);
	gcrderef(&r_status_substrs);
	gcrderef(&r_substrs);
	gcrderef(&r_pattern);
	gcrderef(&r_subject);
	return status;
}
