/* es-mveety line editor
 * this has been broadly cribbed from linenoise (https://github.com/antirez/linenoise)
 * basically this is just a little dumber, less featureful, and more integrated
 * into es.
 */

#ifdef STANDALONE
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>
#else
#include "es.h"
#include "gc.h"
#endif
#include "editor.h"

#define dprint(args...)            \
	if(state->dfd > 0) {           \
		dprintf(state->dfd, args); \
	}

typedef struct {
	char *str;
	size_t len;
} OutBuf;

const Position ErrPos = (Position){-1, -1};

#ifdef STANDALONE

#define nil ((void *)0)

void *
ealloc(size_t n)
{
	void *ptr;

	ptr = malloc(n);
	assert(ptr);
	memset(ptr, 0, n);
	return ptr;
}

void *
erealloc(void *p, size_t n)
{
	void *ptr;

	if(!p)
		return ealloc(n);
	ptr = realloc(p, n);
	assert(ptr);
	return ptr;
}

#endif

HistoryEntry *get_history_next(EditorState *state);
HistoryEntry *get_history_last(EditorState *state);

void
outbuf_append(OutBuf *obuf, char *str, int len)
{
	char *new;

	new = erealloc(obuf->str, obuf->len + len);
	memcpy(&new[obuf->len], str, len);
	obuf->str = new;
	obuf->len += len;
}

int
rawmode_on(EditorState *state)
{
	struct termios raw;

	if(tcgetattr(state->ifd, &state->old_termios) < 0)
		return -1;

	raw = state->old_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~OPOST;
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if(tcsetattr(state->ifd, TCSANOW, &raw) < 0) {
		tcsetattr(state->ifd, TCSANOW, &state->old_termios);
		return -1;
	}
	state->rawmode = 1;
	return 0;
}

int
rawmode_off(EditorState *state)
{
	if(tcsetattr(state->ifd, TCSANOW, &state->old_termios) < 0)
		return -1;
	state->rawmode = 0;
	return 0;
}

Position
getposition(EditorState *state)
{
	char buf[32];
	int cols, rows;
	size_t i = 0;

	if(write(state->ofd, "\x1b[6n", 4) != 4)
		return ErrPos;

	for(i = 0; i < sizeof(buf) - 1; i++) {
		if(read(state->ifd, &buf[i], 1) != 1)
			return ErrPos;
		if(buf[i] == 'R')
			break;
	}

	buf[i] = 0;
	if(buf[0] != KeyEscape || buf[1] != '[')
		return ErrPos;
	if(sscanf(&buf[2], "%d;%d", &rows, &cols) != 2)
		return ErrPos;
	return (Position){cols, rows};
}

int
setposition_lines(EditorState *state, Position pos)
{
	char buf[32];

	snprintf(&buf[0], sizeof(buf), "\x1b[%dH", pos.lines);
	if(write(state->ofd, buf, strnlen(buf, sizeof(buf))) < 0)
		return -1;
	return 0;
}

int
setposition_cols(EditorState *state, Position pos)
{
	char buf[32];

	snprintf(&buf[0], sizeof(buf), "\x1b[;%dH", pos.cols);
	if(write(state->ofd, buf, strnlen(buf, sizeof(buf))) < 0)
		return -1;
	return 0;
}

int
setposition(EditorState *state, Position pos)
{
	char buf[32];

	snprintf(&buf[0], sizeof(buf), "\x1b[%d;%dH", pos.lines, pos.cols);
	if(write(state->ofd, buf, strnlen(buf, sizeof(buf))) < 0)
		return -1;
	return 0;
}

Position
gettermsize(EditorState *state)
{
	struct winsize ws;
	Position saved_pos;
	Position res;

	if(ioctl(state->ofd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		saved_pos = getposition(state);
		if(saved_pos.cols == -1 && saved_pos.lines == -1)
			return ErrPos;

		if(write(state->ofd, "\1b[999C", 6) != 6)
			return ErrPos;
		res = getposition(state);
		setposition(state, saved_pos);
		return res;
	}
	return (Position){ws.ws_col, ws.ws_row};
}

int
initialize_editor(EditorState *state, int ifd, int ofd)
{
	if(!isatty(ifd) || !isatty(ofd))
		return -1;
	memset(state, 0, sizeof(EditorState));
	*state = (EditorState){
		.ifd = ifd,
		.ofd = ofd,
		.dfd = -1,
		.buffer = ealloc(EDITINITIALBUFSZ),
		.bufsz = EDITINITIALBUFSZ,
		.bufpos = 0,
		.bufend = 0,
		.position = (Position){.lines = 1, .cols = 0},
		.last_end = (Position){.lines = 1, .cols = 0},
		.initialized = 1,
		.history = nil,
		.histbuf = nil,
		.inhistory = 0,
		.cur = nil,
		.completions = nil,
		.completionsi = 0,
		.completionssz = 0,
		.completions_hook = nil,
		.completebuf = nil,
		.lastcomplen = 0,
		.wordbreaks = " \t\n",
		.in_completion = 0,
		.pos = (Wordpos){0, 0},
	};
	rawmode_on(state);
	state->size = gettermsize(state);
	rawmode_off(state);
	return 0;
}

void
editor_debugging(EditorState *state, int dfd)
{
	state->dfd = dfd;
}

void
free_editor(EditorState *state)
{
	HistoryEntry *cur;

	if(!state->initialized)
		return;

	free(state->buffer);
	if(state->prompt1)
		free(state->prompt1);
	if(state->prompt2)
		free(state->prompt2);
	state->prompt1sz = 0;
	state->prompt2sz = 0;
	state->initialized = 0;

	cur = state->history;
	if(cur) {
		while(cur->next != nil) {
			if(cur->prev) {
				free(cur->prev->str);
				free(cur->prev);
			}
			cur = cur->next;
		}
		if(cur->prev) {
			free(cur->prev->str);
			free(cur->prev);
		}
		free(cur->str);
		free(cur);
	}
}

int
reset_editor(EditorState *state)
{
	size_t i;

	if(!state->initialized)
		return -1;
	memset(state->buffer, 0, state->bufsz);
	state->bufpos = 0;
	state->bufend = 0;
	state->position = (Position){.lines = 1, .cols = 0};
	state->last_end = (Position){.lines = 1, .cols = 0};
	state->size = gettermsize(state);
	state->inhistory = 0;
	state->cur = nil;
	if(state->histbuf)
		free(state->histbuf);
	state->histbufsz = 0;
	if(state->completions) {
		for(i = 0; state->completions[i] != nil; i++)
			free(state->completions[i]);
		free(state->completions);
		state->completions = nil;
		state->completionssz = 0;
	}
	if(state->completebuf)
		free(state->completebuf);
	state->completebuf = nil;
	state->completionsi = 0;
	state->lastcomplen = 0;
	state->in_completion = 0;
	state->pos = (Wordpos){0,0};
	return 0;
}

void
refresh(EditorState *state)
{
	OutBuf buf = (OutBuf){nil, 0};
	char snbuf[64];
	size_t snsz;
	int promptn = state->which_prompt;
	size_t promptsz = promptn == 0 ? state->prompt1sz : state->prompt2sz;
	char *prompt = promptn == 0 ? state->prompt1 : state->prompt2;
	size_t i;
	Position real_position;
	Position rel_end;
	Position rel_next_end;
	Position rel_cur_pos;
	Position rel_next_pos;

	assert(state->bufpos <= state->bufend);
	/* we need to work out where the line originally started */
	real_position = getposition(state);
	rel_end = state->last_end;
	rel_next_end = (Position){
		.lines = (state->bufend + promptsz + state->size.cols) / state->size.cols,
		.cols = (state->bufend + promptsz) % state->size.cols,
	};
	rel_cur_pos = state->position;
	rel_next_pos = (Position){
		.lines = (state->bufpos + promptsz + state->size.cols) / state->size.cols,
		.cols = (state->bufpos + promptsz) % state->size.cols,
	};
	if(state->dfd > 0) {
		dprintf(state->dfd, "----------\n");
		dprintf(state->dfd, "state->size = {.lines = %d, .cols = %d}\n", state->size.lines,
				state->size.cols);
		dprintf(state->dfd, "state->bufend = %lu, state->bufpos = %lu\n", state->bufend,
				state->bufpos);
		dprintf(state->dfd, "rel_end = {.lines = %d, .cols = %d}\n", rel_end.lines, rel_end.cols);
		dprintf(state->dfd, "rel_next_end = {.lines = %d, .cols = %d}\n", rel_next_end.lines,
				rel_next_end.cols);
		dprintf(state->dfd, "rel_cur_pos = {.lines = %d, .cols = %d}\n", rel_cur_pos.lines,
				rel_cur_pos.cols);
		dprintf(state->dfd, "rel_next_pos = {.lines = %d, .cols = %d}\n", rel_next_pos.lines,
				rel_next_pos.cols);
		dprintf(state->dfd, "real_position = {.lines = %d, .cols = %d}\n", real_position.lines,
				real_position.cols);
	}

	/* go to end */
	if(state->dfd > 0)
		dprintf(state->dfd, "go to end of line...");
	if(rel_end.lines - rel_cur_pos.lines > 0)
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dB", rel_end.lines - rel_cur_pos.lines);
	else
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r");
	outbuf_append(&buf, &snbuf[0], snsz);

	if(state->dfd > 0)
		dprintf(state->dfd, "clear lines...");
	for(i = 0; i < (size_t)(rel_end.lines - 1); i++) {
		if(state->dfd > 0)
			dprintf(state->dfd, "clearing and moving up 1 line...");
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[0K\x1b[1A");
		outbuf_append(&buf, &snbuf[0], snsz);
	}

	snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[0K");
	outbuf_append(&buf, &snbuf[0], snsz);

	if(state->dfd > 0)
		dprintf(state->dfd, "print prompt+buffer...");
	outbuf_append(&buf, prompt, promptsz);
	outbuf_append(&buf, state->buffer, state->bufend);

	if(state->dfd > 0)
		dprintf(state->dfd, "move back to beginning...");
	if(rel_next_end.lines - 1 > 0) {
		if(rel_next_end.cols > 0)
			snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dA", rel_next_end.lines - 1);
		else
			snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\n\x1b[%dA", rel_next_end.lines - 1);
	} else
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r");
	outbuf_append(&buf, &snbuf[0], snsz);

	if(rel_next_pos.lines - 1 > 0) {
		if(state->dfd > 0)
			dprintf(state->dfd, "move cursor down...");
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dB", rel_next_pos.lines - 1);
		outbuf_append(&buf, &snbuf[0], snsz);
	}

	if(rel_next_pos.cols > 0) {
		if(state->dfd > 0)
			dprintf(state->dfd, "moving to cursor position...");
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dC", rel_next_pos.cols);
		outbuf_append(&buf, &snbuf[0], snsz);
	}

	if(state->dfd > 0)
		dprintf(state->dfd, "outputting to screen\n");
	write(state->ofd, buf.str, buf.len);
	state->position = rel_next_pos;
	state->last_end = rel_next_end;
	free(buf.str);
}

int
insert_char(EditorState *state, char c)
{
	assert(state->bufpos <= state->bufend);
	if(state->bufend == (state->bufsz - 2)) {
		state->bufsz = state->bufsz * 2;
		state->buffer = erealloc(state->buffer, state->bufsz);
	}
	if(state->bufpos == state->bufend) {
		state->buffer[state->bufpos] = c;
		state->bufpos++;
		state->bufend++;
		return 0;
	}
	memmove(&state->buffer[state->bufpos + 1], &state->buffer[state->bufpos],
			state->bufend - state->bufpos);
	state->buffer[state->bufpos] = c;
	state->bufpos++;
	state->bufend++;
	return 0;
}

int
backspace_char(EditorState *state)
{
	if(state->bufpos == 0)
		return 0;
	if(state->bufpos == state->bufend) {
		state->bufpos--;
		state->bufend--;
		state->buffer[state->bufpos] = 0;
		return 0;
	}
	state->bufpos--;
	state->bufend--;
	memmove(&state->buffer[state->bufpos], &state->buffer[state->bufpos + 1],
			state->bufend - state->bufpos);
	state->buffer[state->bufend] = 0;
	return 0;
}

int
delete_char(EditorState *state)
{
	if(state->bufpos == state->bufend)
		return 0;
	state->bufend--;
	memmove(&state->buffer[state->bufpos], &state->buffer[state->bufpos + 1],
			state->bufend - state->bufpos);
	state->buffer[state->bufend] = 0;
	return 0;
}

void
cursor_move_left(EditorState *state)
{
	if(state->bufpos > 0)
		state->bufpos--;
}

void
cursor_move_right(EditorState *state)
{
	if(state->bufpos < state->bufend)
		state->bufpos++;
}

void
cursor_home(EditorState *state)
{
	if(state->bufpos != 0)
		state->bufpos = 0;
}

void
cursor_end(EditorState *state)
{
	if(state->bufpos < state->bufend)
		state->bufpos = state->bufend;
}

void
set_prompt1(EditorState *state, char *str)
{
	if(state->prompt1) {
		free(state->prompt1);
		state->prompt1sz = 0;
	}
	if(str == nil)
		return;
	state->prompt1 = strdup(str);
	state->prompt1sz = strlen(str);
}

void
set_prompt2(EditorState *state, char *str)
{
	if(state->prompt2) {
		free(state->prompt2);
		state->prompt2sz = 0;
	}
	if(str == nil)
		return;
	state->prompt2 = strdup(str);
	state->prompt2sz = strlen(str);
}

void
set_complete_hook(EditorState *state, char **(*hook)(char *, int, int))
{
	completion_reset(state);
	state->completions_hook = hook;
}

/* history */

void
history_add(EditorState *state, char *str)
{
	HistoryEntry *new_entry = nil;

	new_entry = ealloc(sizeof(HistoryEntry));

	new_entry->str = strdup(str);
	new_entry->len = strlen(str);
	new_entry->next = state->history;
	if(new_entry->next)
		new_entry->next->prev = new_entry;
	new_entry->prev = nil;
	state->history = new_entry;
}

void
history_clear(EditorState *state)
{
	HistoryEntry *cur;

	cur = state->history;
	if(cur) {
		while(cur->next != nil) {
			if(cur->prev) {
				free(cur->prev->str);
				free(cur->prev);
			}
			cur = cur->next;
		}
		if(cur->prev) {
			free(cur->prev->str);
			free(cur->prev);
		}
		free(cur->str);
		free(cur);
	}
	state->cur = nil;
	state->history = nil;
}

HistoryEntry *
get_history_next(EditorState *state)
{
	if(state->history == nil)
		return nil;
	if(state->cur == nil) {
		state->cur = state->history;
		return state->cur;
	}
	state->cur = state->cur->next;
	return state->cur;
}

HistoryEntry *
get_history_prev(EditorState *state)
{
	if(state->history == nil)
		return nil;
	if(state->cur == nil)
		return nil;
	state->cur = state->cur->prev;
	return state->cur;
}

void
do_history_replace(EditorState *state, HistoryEntry *ent)
{
	if(state->inhistory) {
		assert(state->histbuf);
		memset(state->buffer, 0, state->bufend);
		if(ent == nil) {
			memcpy(state->buffer, state->histbuf, state->histbufsz);
			state->bufend = state->histbufsz;
			free(state->histbuf);
			state->inhistory = 0;
		} else {
			memcpy(state->buffer, ent->str, ent->len);
			state->bufend = ent->len;
		}
		if(state->bufpos > state->bufend)
			state->bufpos = state->bufend;
	} else {
		if(ent == nil)
			return;
		state->inhistory = 1;
		state->histbuf = strdup(state->buffer);
		state->histbufsz = strlen(state->buffer);
		memset(state->buffer, 0, state->bufend);
		memcpy(state->buffer, ent->str, ent->len);
		state->bufend = ent->len;
		if(state->bufpos > state->bufend)
			state->bufpos = state->bufend;
	}
}

void
history_next(EditorState *state)
{
	HistoryEntry *ent = nil;

	ent = get_history_next(state);
	do_history_replace(state, ent);
}

void
history_prev(EditorState *state)
{
	HistoryEntry *ent = nil;

	ent = get_history_prev(state);
	do_history_replace(state, ent);
}

void
history_cancel(EditorState *state)
{
	if(state->inhistory) {
		assert(state->histbuf);
		memset(state->buffer, 0, state->bufend);
		memcpy(state->buffer, state->histbuf, state->histbufsz);
		state->bufend = state->histbufsz;
		free(state->histbuf);
		if(state->bufpos > state->bufend)
			state->bufpos = state->bufend;
		state->cur = nil;
	}
}

int
isawordbreak(EditorState *state, size_t wordbreakssz, char c)
{
	size_t i = 0;

	assert(state->wordbreaks);
	for(i = 0; i < wordbreakssz; i++)
		if(c == state->wordbreaks[i])
			return 1;
	return 0;
}

Wordpos
get_completion_position(EditorState *state)
{
	Wordpos res = {0, 0};
	size_t i;
	size_t wordbreakssz;

	assert(state->wordbreaks);
	wordbreakssz = strlen(state->wordbreaks);
	if(state->bufend == 0)
		return res;
	res.end = state->bufpos;
	if(state->bufpos < state->bufend){
		for(i = state->bufpos; i < state->bufend; i++)
			if(isawordbreak(state, wordbreakssz, state->buffer[i])){
				res.end = i;
				break;
			}
	}
	for(i = res.end; i >= 1; i--)
		if(isawordbreak(state, wordbreakssz, state->buffer[i-1])){
			res.start = i;
			assert(res.start <= res.end);
			return res;
		}
	res.start = 0;
	assert(res.start <= res.end);
	return res;
}

void
call_completions_hook(EditorState *state, Wordpos pos)
{
	size_t i;
	char *curline;

	if(!state->completions_hook)
		return;
	curline = strdup(state->buffer);
	state->completions = state->completions_hook(curline, pos.start, pos.end);
	if(state->completions == nil)
		return;
	for(i = 0; state->completions[i] != nil; i++){
		if(state->dfd > 0){
			if(state->completions[i] != nil)
				dprintf(state->dfd, "state->completions[%lu] = %p\n", i, state->completions[i]);
			else
				dprintf(state->dfd, "state->completions[%lu] = nil\n", i);
		}
	}

	state->completionssz = i;
}

char *
get_next_completion(EditorState *state, Wordpos pos)
{
	if(state->completions == nil)
		call_completions_hook(state, pos);
	if(state->completions == nil)
		return nil;

	if(state->completionsi >= state->completionssz){
		state->completionsi = 0;
		return nil;
	}

	return state->completions[state->completionsi++];
}

char *
get_prev_completion(EditorState *state, Wordpos pos)
{
	if(state->completions == nil)
		call_completions_hook(state, pos);
	if(state->completions == nil)
		return nil;

	if(state->completionsi > state->completionssz){
		state->completionsi = 0;
		return nil;
	}
	if(state->completionsi == 0 || state->completionsi > state->completionssz)
		state->completionsi = state->completionssz - 1;

	return state->completions[state->completionsi--];
}

void /* maybe I should use a rope? */
do_completion(EditorState *state, char *comp, Wordpos pos)
{
	size_t complen = 0;
	size_t i = 0;
	size_t prefixlen = 0;
	size_t suffixlen = 0;

	assert(pos.end <= state->bufend);

	if(!state->in_completion){
		if(state->dfd > 0)
			dprintf(state->dfd, "state->in_completion = %d -> 1\n", state->in_completion);
		if(comp == nil)
			return;
		state->pos = pos;
		state->in_completion = 1;
		state->completebuf = strdup(state->buffer);
		dprint("state->completebuf = \"%s\"\n", state->completebuf);
		if(pos.start > 0){
			state->comp_prefix = ealloc(pos.start+1);
			memcpy(state->comp_prefix, state->buffer, pos.start);
			dprint("state->comp_prefix = \"%s\"\n", state->comp_prefix);
		} else {
			state->comp_prefix = nil;
			dprint("state->comp_prefix = nil\n");
		}
		if(state->bufend - pos.end > 0){
			state->comp_suffix = ealloc((state->bufend - pos.end)+1);
			memcpy(state->comp_suffix, &state->buffer[pos.end], (state->bufend - pos.end));
			dprint("state->comp_suffix = \"%s\"\n", state->comp_suffix);
		} else {
			state->comp_suffix = nil;
			dprint("state->comp_suffix = nil\n");
		}
	}

	if(comp == nil){
		dprint("state->in_completion = %d -> 0\n", state->in_completion);
		memset(state->buffer, 0, state->bufend+1);
		memcpy(state->buffer, state->completebuf, strlen(state->completebuf));
		state->bufend = strlen(state->completebuf);
		state->bufpos = pos.end;
		if(state->bufpos > state->bufend)
			state->bufpos = state->bufend;
		free(state->completebuf);
		if(state->comp_prefix)
			free(state->comp_prefix);
		if(state->comp_suffix)
			free(state->comp_suffix);
		state->in_completion = 0;
		return;
	}

	complen = strlen(comp);
	if(state->comp_prefix){
		prefixlen = strlen(state->comp_prefix);
		memcpy(state->buffer, state->comp_prefix, prefixlen);
		i += prefixlen;
	}
	memcpy(&state->buffer[i], comp, complen);
	i += complen;
	if(state->comp_suffix){
		suffixlen = strlen(state->comp_suffix);
		memcpy(&state->buffer[i], state->comp_suffix, suffixlen);
		i += suffixlen;
	}

	state->bufpos = prefixlen + complen;
	state->bufend = i;
	if(state->bufpos > state->bufend)
		state->bufpos = state->bufend;
	state->lastcomplen = complen;
}

void
completion_reset(EditorState *state)
{
	size_t i;

	if(state->completions){
		for(i = 0; state->completions[i] != nil; i++)
			free(state->completions[i]);
		free(state->completions);
		state->completions = nil;
		state->completionssz = 0;
	}
	if(state->completebuf){
		free(state->completebuf);
		state->completebuf = nil;
	}
	state->completionsi = 0;
	state->lastcomplen = 0;
	if(state->comp_prefix)
		free(state->comp_prefix);
	state->comp_prefix = nil;
	if(state->comp_suffix)
		free(state->comp_suffix);
	state->comp_suffix = nil;
	state->in_completion = 0;
}

void
completion_maybe_reset(EditorState *state)
{
	if(state->bufpos < state->pos.start
		|| state->bufpos > (state->pos.start + state->lastcomplen))
		completion_reset(state);
}

void
completion_next(EditorState *state)
{
	char *comp = nil;

	if(!state->in_completion)
		state->pos = get_completion_position(state);
	comp = get_next_completion(state, state->pos);
	if(state->dfd > 0){
		dprintf(state->dfd, "complete start = %lu, end = %lu\n", state->pos.start, state->pos.end);
		if(comp != nil)
			dprintf(state->dfd, "completion string = \"%s\"\n", comp);
		else
			dprintf(state->dfd, "completion string = nil\n");
	}
	do_completion(state, comp, state->pos);
}

void
completion_prev(EditorState *state)
{
	char *comp = nil;

	if(!state->in_completion)
		state->pos = get_completion_position(state);
	comp = get_prev_completion(state, state->pos);
	if(state->dfd > 0){
		dprintf(state->dfd, "complete start = %lu, end = %lu\n", state->pos.start, state->pos.end);
		if(comp != nil)
			dprintf(state->dfd, "completion string = \"%s\"\n", comp);
		else
			dprintf(state->dfd, "completion string = nil\n");
	}
	do_completion(state, comp, state->pos);
}

char *
basic_editor(EditorState *state)
{
	char c;
	char *res;
	char seq[3];
	enum { StateRead, StateDone } readstate = StateRead;

	rawmode_on(state);
	if(reset_editor(state) < 0)
		goto fail;

	while(readstate == StateRead) {
		refresh(state);
		if(read(state->ifd, &c, 1) < 0)
			goto fail;
top:
		switch(c) {
		default:
			if(c >= ' ' && c <= '~')
				insert_char(state, c);
			completion_reset(state);
			break;
		case KeyEnter:
			readstate = StateDone;
			break;
		case KeyCtrlC:
			goto fail;
			break;
		case KeyBackspace:
			backspace_char(state);
			completion_reset(state);
			break;
		case KeyTab:
			completion_next(state);
			break;
		case KeyCtrlA:
			cursor_home(state);
			completion_maybe_reset(state);
			break;
		case KeyCtrlE:
			cursor_end(state);
			completion_maybe_reset(state);
			break;
		case KeyEscape:
			if(read(state->ifd, &seq[0], 1) < 0)
				break;
			if(seq[0] == '[') {
				completion_maybe_reset(state);
				if(read(state->ifd, &seq[1], 1) < 0)
					break;
				switch(seq[1]) {
				default:
					break;
				case 'A':
					history_next(state);
					break;
				case 'B':
					history_prev(state);
					break;
				case 'C':
					cursor_move_right(state);
					break;
				case 'D':
					cursor_move_left(state);
					break;
				}
			} else {
				c = seq[0];
				goto top;
			}
			break;
		}
	}
	refresh(state);

	if(state->bufend == 0)
		goto fail;

	rawmode_off(state);
	res = strndup(state->buffer, state->bufend + 1);
	return res;

fail:
	rawmode_off(state);
	return nil;
}

char *
line_editor(EditorState *state)
{
	char c;
	char *res;
	char seq[3];
	enum { StateRead, StateDone } readstate = StateRead;

	rawmode_on(state);
	if(reset_editor(state) < 0)
		goto fail;

	while(readstate == StateRead) {
		refresh(state);
		if(read(state->ifd, &c, 1) < 0)
			goto fail;
top:
		switch(c) {
		default:
			if(c >= ' ' && c <= '~')
				insert_char(state, c);
			break;
		case KeyEnter:
			readstate = StateDone;
			break;
		case KeyCtrlC:
			goto fail;
			break;
		case KeyBackspace:
			backspace_char(state);
			break;
		case KeyEscape:
			if(read(state->ifd, &seq[0], 1) < 0)
				break;
			if(seq[0] == '[') {
				if(read(state->ifd, &seq[1], 1) < 0)
					break;
				switch(seq[1]) {
				default:
					break;
				case 'A':
					history_next(state);
					break;
				case 'B':
					history_prev(state);
					break;
				case 'C':
					cursor_move_right(state);
					break;
				case 'D':
					cursor_move_left(state);
					break;
				}
			} else {
				c = seq[0];
				goto top;
			}
			break;
		}
	}
	refresh(state);

	if(state->bufend == 0)
		goto fail;

	rawmode_off(state);
	res = strndup(state->buffer, state->bufend + 1);
	return res;

fail:
	rawmode_off(state);
	return nil;
}
