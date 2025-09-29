/* status.c -- status manipulations ($Revision: 1.1.1.1 $) */

#include "es.h"

// clang-format off
static const Term
	trueterm	= { tkString, ttNone, {.str = "0"}},
	falseterm	= { tkString, ttNone, {.str = "1"}};
static const List
	truelist	= { (Term *) &trueterm, nil },
	falselist	= { (Term *) &falseterm, nil };
List
	*list_true		= (List *) &truelist,
	*list_false		= (List *) &falselist;
// clang-format on

/* istrue -- is this status list true? */
extern Boolean
istrue(List *status)
{
	char *str = nil;

	for(; status != NULL; status = status->next) {
		switch(status->term->kind) {
		default:
			unreachable();
			break;
		case tkClosure:
			if(status->term->tag == ttError)
				return TRUE;
			else
				return FALSE;
		case tkDict:
		case tkRegex:
		case tkObject:
			return FALSE;
		case tkString:
			if(termeq(status->term, "true"))
				return TRUE;
			else if(termeq(status->term, "false"))
				return FALSE;
			else {
				str = status->term->str;
				assert(str != NULL);
				if(*str != '\0' && (*str != '0' || str[1] != '\0'))
					return FALSE;
			}
			break;
		}
	}
	return TRUE;
}

/* exitstatus -- turn a status list into an exit(2) value */
extern int
exitstatus(List *status)
{
	Term *term;
	char *s;
	unsigned long n;

	if(status == NULL)
		return 0;
	if(status->next != NULL)
		return istrue(status) ? 0 : 1;

	term = status->term;
	switch(term->kind) {
	default:
		unreachable();
	case tkClosure:
	case tkDict:
	case tkRegex:
		return 1;
	case tkString:
		s = term->str;
		if(*s == '\0')
			return 0;
		n = strtol(s, &s, 0);
		if(*s != '\0' || n > 255)
			return 1;
		return n;
	}
	unreachable();
}

/* mkstatus -- turn a unix exit(2) status into a string */
extern char *
mkstatus(int status)
{
	if(SIFSIGNALED(status)) {
		char *name = signame(STERMSIG(status));
		if(SCOREDUMP(status))
			name = str("%s+core", name);
		return name;
	}
	return str("%d", SEXITSTATUS(status));
}

/* printstatus -- print the status if we should */
extern void
printstatus(int pid, int status)
{
	if(SIFSIGNALED(status)) {
		const char *msg = sigmessage(STERMSIG(status)), *tail = "";
		if(SCOREDUMP(status)) {
			tail = "--core dumped";
			if(*msg == '\0')
				tail += (sizeof "--") - 1;
		}
		if(*msg != '\0' || *tail != '\0') {
			if(pid == 0)
				eprint("%s%s\n", msg, tail);
			else
				eprint("%d: %s%s\n", pid, msg, tail);
		}
	}
}
