#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "editor.h"

#define nil ((void *)0)

extern void *ealloc(size_t);

int dfd = -1;

const char *stuff[] = {
	"hello_world", "this_is_a_test", "aaa", "yoyoyoyoy", "wow_man_cool", "bbb", "aaa", 0,
};

char **
completions_hook(char *line, int start, int end)
{
	char **comps;
	size_t i = 0;
	size_t compssz = (sizeof(stuff) / sizeof(char *));

	dprintf(dfd, "te completions_hook: line = \"%s\"\n", line);
	dprintf(dfd, "te completions_hook: start = %d, end = %d\n", start, end);

	comps = ealloc(compssz * sizeof(char *));
	for(i = 0; i < compssz - 1; i++) {
		comps[i] = strdup(stuff[i]);
		dprintf(dfd, "te completions_hook: comps[%lu](%p) = \"%s\"\n", i, comps[i], comps[i]);
	}
	comps[i] = nil;
	return comps;
}

char *
ctrlx_hook(EditorState *state, int key, void *aux)
{
	char *str = "hello world this is a test!";

	return strdup(str);
}

char *
ctrls_hook(EditorState *state, int key, void *aux)
{
	if(state->dfd > 0)
		dprintf(state->dfd, "Ctrl-S pressed!\n");
	return nil;
}

char *
syntax_highlight_hook(char *buffer, size_t len)
{
	char redcolor[] = "\x01\x1b[31m\x02";
	char resetfmt[] = "\x01\x1b[0m\x02";
	char *resbuf = nil;

	dprintf(dfd, "sizeof(redcolor) = %lu, sizeof(resetfmt) = %lu\n", sizeof(redcolor),
			sizeof(resetfmt));
	if(dfd > 0)
		dprintf(dfd, "syntax hook: got \"%s\" (len = %lu)\n", buffer, len);
	if(len == 0)
		return nil;

	resbuf = ealloc(len + sizeof(redcolor) + sizeof(resetfmt));
	memcpy(resbuf, redcolor, sizeof(redcolor) - 1);
	memcpy(&resbuf[sizeof(redcolor) - 1], buffer, len);
	memcpy(&resbuf[sizeof(redcolor) - 1 + len], resetfmt, sizeof(resetfmt));
	dprintf(dfd, "resbuf = \"%s\"\n", resbuf);

	return resbuf;
}

int
main(int argc, char *argv[])
{
	EditorState state;
	Result editres;
	int st;
	char highlight_formatting[] = "\x1b[46m\x1b[37m";

	if(argc == 2) {
		if((dfd = open(argv[1], O_WRONLY)) < 0) {
			dprintf(2, "unable to open %s\n", argv[1]);
			return -1;
		}
	}
	st = initialize_editor(&state, 0, 1);
	switch(st) {
	case -1:
		dprintf(2, "not a tty\n");
		break;
	case -2:
		dprintf(2, "unsupported terminal: %s\n", getenv("TERM"));
		break;
	}
	editor_debugging(&state, dfd);
	set_prompt1(&state, "prompt1> ");
	set_prompt2(&state, "prompt2> ");
	set_complete_hook(&state, &completions_hook);
	bindmapping(&state, KeyCtrlX, (Mapping){.hook = &ctrlx_hook, .reset_completion = 1});
	bindmapping(&state, KeyCtrlS, (Mapping){.hook = &ctrls_hook, .reset_completion = 0});
	state.wordbreaks = " \t\n\\'`$><=;|&{()}";
	state.prefixes = "$";
	set_syntax_highlight_hook(&state, &syntax_highlight_hook);
	state.match_braces = 1;
	state.word_start = FirstBreak;
	register_braces(&state, '(', ')');
	register_braces(&state, '{', '}');
	set_highlight_formatting(&state, highlight_formatting);

	dprintf(state.ofd, "running in %s\n", state.term);
	dprintf(state.ofd, "type \"exit\" or \"quit\" to quit or exit\n");
	for(;;) {
		editres = line_editor(&state);
		if(editres.str == nil) {
			dprintf(2, "got not input\n");
			continue;
		} else {
			dprintf(2, "got: \"%s\" (size %lu)\n", editres.str, strlen(editres.str));
			history_add(&state, editres.str);
		}
		if(strcmp(editres.str, "quit") == 0 || strcmp(editres.str, "exit") == 0)
			break;
		free(editres.str);
	}
	if(dfd > 0)
		close(dfd);
	return 0;
}
