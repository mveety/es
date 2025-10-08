#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "editor.h"

#define nil ((void*)0)

extern void *ealloc(size_t);

int dfd = 0;

const char *stuff[] = {
	"hello_world",
	"this_is_a_test",
	"yoyoyoyoy",
	"wow_man_cool",
	0,
};

char**
completions_hook(char *line, int start, int end)
{
	char **comps;
	size_t i = 0;
	size_t compssz = (sizeof(stuff)/sizeof(char*));

	dprintf(dfd, "te completions_hook: line = \"%s\"\n", line);
	dprintf(dfd, "te completions_hook: start = %d, end = %d\n", start, end);

	comps = ealloc(compssz*sizeof(char*));
	for(i = 0; i < compssz-1; i++){
		comps[i] = strdup(stuff[i]);
		dprintf(dfd, "te completions_hook: comps[%lu](%p) = \"%s\"\n", i, comps[i], comps[i]);
	}
	comps[i] = nil; 
	return comps;
}

int
main(int argc, char *argv[])
{
	EditorState state;
	char *line;

	if((dfd = open("/tmp/editor_debug.fifo", O_WRONLY)) < 0){
		dprintf(2, "unable to open /tmp/editor_debug.fifo\n");
		return -1;
	}
	initialize_editor(&state, 0, 1);
	editor_debugging(&state, dfd);
	set_prompt1(&state, "prompt1> ");
	set_prompt2(&state, "prompt2> ");
	set_complete_hook(&state, &completions_hook);
	for(;;){
		line = basic_editor(&state);
		if(line == nil){
			dprintf(2, "\ngot not input\n");
			continue;
		} else {
			dprintf(2, "\ngot: \"%s\" (size %lu)\n", line, strlen(line));
			history_add(&state, line);
		}
		if(strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0)
			break;
		free(line);
	}
	if(dfd > 0)
		close(dfd);
	return 0;
}

