/* es-mveety line editor
 * this has been broadly cribbed from linenoise (https://github.com/antirez/linenoise)
 * basically this is just a little dumber, less featureful, and more integrated
 * into es.
 */

#include <stdint.h>
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
#include <unistd.h>
#else
#include "es.h"
#include "gc.h"
#endif
#include <locale.h>
#include <wchar.h>
#include "editor.h"

#define dprint(args...)                \
	do {                               \
		if(state->dfd > 0) {           \
			dprintf(state->dfd, args); \
		}                              \
	} while(0)

#define bufferassert() assert(state->bufpos <= state->bufend)

typedef struct {
	char *str;
	size_t len;
	size_t size;
} OutBuf;

const Position ErrPos = (Position){-1, -1};

#ifdef STANDALONE

#define nil ((void *)0)

#define strneq(s, t, n) (strncmp(s, t, n) == 0)
#define hasprefix(s, p) strneq(s, p, (sizeof(p) - 1))

#ifndef unreachable
#define unreachable() \
	do {              \
		assert(0);    \
	} while(0)
#endif

#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough \
	do {            \
	} while(0) /* fallthrough */
#endif

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

char *
estrdup(char *str)
{
	char *res = nil;

	res = strdup(str);
	assert(res);
	return res;
}

char *
estrndup(char *str, size_t len)
{
	char *res = nil;

	res = strndup(str, len);
	assert(res);
	return res;
}

typedef struct {
	void *ptr;
	int status;
} Result;

Result
result(void *ptr, int status)
{
	return (Result){
		.ptr = ptr,
		.status = status,
	};
}

void *
ok(Result r)
{
	assert(r.status == 0);
	return r.ptr;
}

int
status(Result r)
{
	return r.status;
}

void *
used(void *ptr)
{
	return ptr;
}

int
editor_istracked(void *ptr)
{
	used(ptr);
	return 0;
}

int
eisatty(int fd)
{
	return isatty(fd);
}

#else

int
editor_istracked(void *ptr)
{
	if(istracked(ptr) == TRUE)
		return 1;
	return 0;
}

#endif

HistoryEntry *get_history_next(EditorState *state);
HistoryEntry *get_history_last(EditorState *state);
void create_default_mapping(Keymap *map);

char *
getterm(void)
{
	char *env_term;

	env_term = getenv("TERM");
	return estrdup(env_term);
}

void
outbuf_append(OutBuf *obuf, char *str, int len)
{
	if(obuf->str == nil)
		obuf->str = ealloc(len + 2);
	if(obuf->len + len + 1 > obuf->size) {
		obuf->str = erealloc(obuf->str, obuf->size + len + 1);
		obuf->size += len + 1;
	}
	memcpy(&obuf->str[obuf->len], str, len);
	obuf->len += len;
}

typedef struct {
	Brace *brace;
	enum { Open, Close } type;
} BraceState;

BraceState
findbrace(char c, EditorState *state)
{
	size_t i = 0;

	for(i = 0; i < state->nbraces; i++) {
		if(state->braces[i].open == c)
			return (BraceState){.brace = &state->braces[i], .type = Open};
		if(state->braces[i].close == c)
			return (BraceState){.brace = &state->braces[i], .type = Close};
	}

	return (BraceState){.brace = nil};
}

int64_t
find_matching_paren(EditorState *state)
{
	int64_t i = 0;
	int reg = 0;
	char c = 0;
	BraceState bs;

	if(!state->match_braces)
		return -1;
	if(state->done_reading)
		return -1;
	if(state->braces == nil || state->nbraces == 0)
		return -1;

	if(state->bufpos == state->bufend && state->bufpos > 0)
		c = state->buffer[state->bufpos - 1];
	else
		c = state->buffer[state->bufpos];

	bs = findbrace(c, state);
	if(bs.brace == nil)
		return -1;

	switch(bs.type) {
	default:
		unreachable();
		break;
	case Open:
		if(state->bufpos == state->bufend)
			return -1;
		for(i = state->bufpos; i <= (int64_t)state->bufend; i++) {
			if(reg < 0)
				return -1;
			if(state->buffer[i] == bs.brace->open) {
				reg++;
				continue;
			}
			if(state->buffer[i] == bs.brace->close) {
				reg--;
				if(reg == 0)
					return i;
			}
		}
		return -1;
	case Close:
		for(i = state->bufpos; i >= 0; i--) {
			if(reg < 0)
				return -1;
			if(state->buffer[i] == bs.brace->close) {
				reg++;
				continue;
			}
			if(state->buffer[i] == bs.brace->open) {
				reg--;
				if(reg == 0)
					return i;
			}
		}
		return -1;
	}
	return -1;
}

void
outbuf_append_printable(EditorState *state, OutBuf *obuf, char *str, int len)
{
	int64_t i = 0;
	int64_t highlight = -1;
	int64_t highlightsz = 0;

	highlight = find_matching_paren(state);
	if(highlight >= 0)
		highlightsz = 8;

	if(obuf->str == nil)
		obuf->str = ealloc(len + highlightsz + 2);
	if(obuf->len + highlightsz + len + 1 > obuf->size) {
		obuf->str = erealloc(obuf->str, obuf->size + highlightsz + len + 1);
		obuf->size += len + 1;
	}

	for(i = 0; i < len; i++) {
		if(i == highlight) {
			dprint("adding highlight at %ld\n", i);
			obuf->str[obuf->len++] = '\x1b';
			obuf->str[obuf->len++] = '[';
			obuf->str[obuf->len++] = '7';
			obuf->str[obuf->len++] = 'm';
		}
		if(str[i] >= ' ' && str[i] <= '~') {
			obuf->str[obuf->len++] = str[i];
		} else if(str[i] & 0b10000000) {
			obuf->str[obuf->len++] = str[i];
		} else {
			dprint("got unprintable char in buffer: %x\n", str[i]);
		}
		if(i == highlight) {
			obuf->str[obuf->len++] = '\x1b';
			obuf->str[obuf->len++] = '[';
			obuf->str[obuf->len++] = '0';
			obuf->str[obuf->len++] = 'm';
		}
	}
}

void
outbuf_clean(OutBuf *obuf)
{
	memset(obuf->str, 0, obuf->len + 1);
	obuf->len = 0;
}

int
reconfigure_terminal(EditorState *state)
{
	struct termios raw;

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
rawmode_on(EditorState *state)
{
	struct termios raw;

	if(state->rawmode)
		return 1;
	if(tcgetattr(state->ifd, &state->old_termios) < 0)
		return -1;

	raw = state->old_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~OPOST;
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if(tcsetattr(state->ifd, TCSAFLUSH, &raw) < 0) {
		tcsetattr(state->ifd, TCSAFLUSH, &state->old_termios);
		return -1;
	}
	state->rawmode = 1;
	return 0;
}

int
rawmode_off(EditorState *state)
{
	if(!state->rawmode)
		return -2;
	if(tcsetattr(state->ifd, TCSAFLUSH, &state->old_termios) < 0)
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

	/* this stuff only works if this is really a for real tty */
	if(!isatty(state->ofd) || !isatty(state->ifd))
		return (Position){80, 24};
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
supported_term(char *term)
{
	if(hasprefix(term, "xterm"))
		return 1;
	if(hasprefix(term, "screen"))
		return 1;
	if(hasprefix(term, "rxvt"))
		return 1;
	if(hasprefix(term, "linux"))
		return 1;
	if(hasprefix(term, "tmux"))
		return 1;
	if(hasprefix(term, "putty"))
		return 1;
	if(hasprefix(term, "dtterm"))
		return 1;
	if(hasprefix(term, "st"))
		return 1;
	if(hasprefix(term, "vt1"))
		return 1;
	if(hasprefix(term, "vt2"))
		return 1;
	if(hasprefix(term, "vt3"))
		return 1;
	if(hasprefix(term, "vt4"))
		return 1;
	if(hasprefix(term, "vt5"))
		return 1;
	return 0;
}

int
initialize_editor(EditorState *state, int ifd, int ofd)
{
	char *term = nil;

	memset(state, 0, sizeof(EditorState));
	state->ifd = ifd;
	state->ofd = ofd;
	state->prompt1 = nil;
	state->initialized = 0;

	if(!eisatty(ifd) || !eisatty(ofd))
		return -1;
	term = getterm();
	if(!supported_term(term)) {
		free(term);
		return -2;
	}
	*state = (EditorState){
		.ifd = ifd,
		.ofd = ofd,
		.dfd = -1,
		.term = term,
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
		.prefixes = nil,
		.in_completion = 0,
		.pos = (Wordpos){.start = 0, .end = 0 },
		.outbuf = ealloc(sizeof(OutBuf)),
		.sort_completions = 0,
		.remove_duplicates = 0,
		.clear_screen = 0,
		.done_reading = 0,
		.match_braces = 1,
		.braces = nil,
		.nbraces = 0,
		.force_fallback = 0,
		.word_start = FirstLetter,
	};
	memset(state->outbuf, 0, sizeof(OutBuf));
	rawmode_on(state);
	state->size = gettermsize(state);
	rawmode_off(state);
	state->keymap = ealloc(sizeof(Keymap));
	create_default_mapping(state->keymap);
	setlocale(LC_ALL, "");
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
	OutBuf *buf;

	if(!state->initialized)
		return;

	free(state->term);
	free(state->buffer);
	if(state->prompt1)
		free(state->prompt1);
	if(state->prompt2)
		free(state->prompt2);
	state->prompt1sz = 0;
	state->prompt2sz = 0;
	state->initialized = 0;
	buf = state->outbuf;
	if(buf->str)
		free(buf->str);
	free(state->outbuf);

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
	free(state->keymap);
	if(state->braces)
		free(state->braces);
}

int
reset_editor(EditorState *state)
{
	if(!state->initialized)
		return -1;
	memset(state->buffer, 0, state->bufsz);
	state->bufpos = 0;
	state->bufend = 0;
	state->position = (Position){.lines = 1, .cols = 0};
	state->last_end = (Position){.lines = 1, .cols = 0};
	state->size = gettermsize(state);
	if(state->inhistory) {
		assert(state->histbuf);
		free(state->histbuf);
		state->histbufsz = 0;
		state->cur = nil;
		state->inhistory = 0;
	}
	completion_reset(state);
	state->pos = (Wordpos){0, 0};
	state->done_reading = 0;
	return 0;
}

int
register_braces(EditorState *state, char open, char close)
{
	if(!state->initialized)
		return -1;

	state->braces = erealloc(state->braces, (state->nbraces + 1) * sizeof(Brace));
	state->braces[state->nbraces] = (Brace){.open = open, .close = close};
	state->nbraces++;

	return 0;
}

size_t
marked_strlen(char *str)
{
	size_t i = 0;
	size_t len = 0;
	enum { Count, DontCount } state = Count;

	for(i = 0, len = 0; str[i] != 0; i++) {
		switch(state) {
		default:
			unreachable();
			break;
		case Count:
			if(str[i] == '\x01')
				state = DontCount;
			else
				len++;
			break;
		case DontCount:
			if(str[i] == '\x02')
				state = Count;
			break;
		}
	}
	return len;
}

size_t
utf8_strnlen(EditorState *state, char *str, size_t lim)
{
	size_t i = 0;
	size_t sz = 0;
	wchar_t buf;
	int st;
	int width;

	for(i = 0; i < lim;) {
		width = mbtowc(&buf, &str[i], 4);
		if(width < 0) {
			i++;
			continue;
		}
		i += width;
		st = wcwidth(buf);
		if(st < 0)
			continue;
		sz += st;
	}
	return sz;
}

size_t
utf8_marked_strlen(char *str)
{
	size_t i = 0;
	size_t len = 0;
	enum { Count, DontCount } state = Count;
	wchar_t buf;
	int st;
	int width;

	for(i = 0, len = 0; str[i] != 0;) {
		switch(state) {
		default:
			unreachable();
			break;
		case Count:
			if(str[i] == '\x01') {
				state = DontCount;
				continue;
			}
			width = mbtowc(&buf, &str[i], 4);
			if(width < 0) {
				i++;
				continue;
			}
			i += width;
			st = wcwidth(buf);
			if(st < 0)
				continue;
			len += st;
			break;
		case DontCount:
			if(str[i] == '\x02')
				state = Count;
			i++;
			break;
		}
	}

	return len;
}

void
set_prompt1(EditorState *state, char *str)
{
	dprint("setting prompt1 to \"%s\"\n", str);
	if(state->prompt1) {
		free(state->prompt1);
		state->prompt1sz = 0;
	}
	if(str == nil)
		return;
	state->prompt1 = estrdup(str);
	state->prompt1sz = marked_strlen(str);
}

void
set_prompt2(EditorState *state, char *str)
{
	if(!state->initialized)
		return;
	dprint("setting prompt2 to \"%s\"\n", str);
	if(state->prompt2) {
		free(state->prompt2);
		state->prompt2sz = 0;
	}
	if(str == nil)
		return;
	state->prompt2 = estrdup(str);
	state->prompt2sz = marked_strlen(str);
}

void
set_complete_hook(EditorState *state, char **(*hook)(char *, int, int))
{
	if(!state->initialized)
		return;
	completion_reset(state);
	state->completions_hook = hook;
}

void /* I really think this could use work */
refresh(EditorState *state)
{
	OutBuf *buf = nil;
	char snbuf[64];
	size_t snsz;
	int promptn = state->which_prompt;
	size_t promptsz = promptn == 0 ? state->prompt1sz : state->prompt2sz;
	char *prompt = promptn == 0 ? state->prompt1 : state->prompt2;
	size_t i;
	/*Position real_position;*/
	Position rel_end;
	Position rel_next_end;
	Position rel_cur_pos;
	Position rel_next_pos;
	size_t utf8_promptsz = 0;
	size_t utf8_bufpos = 0;
	size_t utf8_bufend = 0;

	bufferassert();
	buf = state->outbuf;
	/* we need to work out where the line originally started */
	utf8_promptsz = utf8_marked_strlen(prompt);
	utf8_bufpos = utf8_strnlen(state, state->buffer, state->bufpos);
	utf8_bufend = utf8_strnlen(state, state->buffer, state->bufend);
	/* real_position = getposition(state); */
	rel_end = state->last_end;
	rel_next_end = (Position){
		.lines = (utf8_bufend + utf8_promptsz + state->size.cols) / state->size.cols,
		.cols = (utf8_bufend + utf8_promptsz) % state->size.cols,
	};
	rel_cur_pos = state->position;
	rel_next_pos = (Position){
		.lines = (utf8_bufpos + utf8_promptsz + state->size.cols) / state->size.cols,
		.cols = (utf8_bufpos + utf8_promptsz) % state->size.cols,
	};
	if(state->dfd > 0) {
		dprintf(state->dfd, "----------\n");
		dprintf(state->dfd, "buf->size = %lu, buf->len = %lu\n", buf->size, buf->len);
		dprintf(state->dfd, "state->size = {.lines = %d, .cols = %d}\n", state->size.lines,
				state->size.cols);
		dprintf(state->dfd, "state->bufend = %lu, state->bufpos = %lu\n", state->bufend,
				state->bufpos);
		dprintf(state->dfd, "utf8_bufpos = %lu, utf8_bufend = %lu\n", utf8_bufpos, utf8_bufend);
		dprintf(state->dfd, "promptsz = %lu, utf8_promptsz = %lu\n", promptsz, utf8_promptsz);
		dprintf(state->dfd, "strlen(prompt) = %lu\n", strlen(prompt));
		dprintf(state->dfd, "rel_end = {.lines = %d, .cols = %d}\n", rel_end.lines, rel_end.cols);
		dprintf(state->dfd, "rel_next_end = {.lines = %d, .cols = %d}\n", rel_next_end.lines,
				rel_next_end.cols);
		dprintf(state->dfd, "rel_cur_pos = {.lines = %d, .cols = %d}\n", rel_cur_pos.lines,
				rel_cur_pos.cols);
		dprintf(state->dfd, "rel_next_pos = {.lines = %d, .cols = %d}\n", rel_next_pos.lines,
				rel_next_pos.cols);
		/*dprintf(state->dfd, "real_position = {.lines = %d, .cols = %d}\n", real_position.lines,
				real_position.cols);*/
		dprintf(state->dfd, "state->buffer = \"%s\"\n", state->buffer);
	}

	/* go to end */
	if(rel_end.lines - rel_cur_pos.lines > 0)
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dB", rel_end.lines - rel_cur_pos.lines);
	else
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r");
	outbuf_append(buf, &snbuf[0], snsz);

	for(i = 0; i < (size_t)(rel_end.lines - 1); i++) {
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[0K\x1b[1A");
		outbuf_append(buf, &snbuf[0], snsz);
	}

	snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[0K");
	outbuf_append(buf, &snbuf[0], snsz);
	if(state->clear_screen) {
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\x1b[H\x1b[2J");
		outbuf_append(buf, &snbuf[0], snsz);
		state->clear_screen = 0;
	}

	outbuf_append(buf, prompt, strlen(prompt));
	outbuf_append_printable(state, buf, state->buffer, state->bufend);

	if(rel_next_end.lines - 1 > 0) {
		if(rel_next_end.cols > 0)
			snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dA", rel_next_end.lines - 1);
		else
			snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\n\x1b[%dA", rel_next_end.lines - 1);
	} else
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r");
	outbuf_append(buf, &snbuf[0], snsz);

	if(rel_next_pos.lines - 1 > 0) {
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dB", rel_next_pos.lines - 1);
		outbuf_append(buf, &snbuf[0], snsz);
	}

	if(rel_next_pos.cols > 0) {
		snsz = snprintf(&snbuf[0], sizeof(snbuf), "\r\x1b[%dC", rel_next_pos.cols);
		outbuf_append(buf, &snbuf[0], snsz);
	}

	write(state->ofd, buf->str, buf->len);
	state->position = rel_next_pos;
	state->last_end = rel_next_end;

	if(state->dfd > 0)
		dprintf(state->dfd, "buf->size = %lu, buf->len = %lu\n", buf->size, buf->len);
	outbuf_clean(buf);
}

void
grow_buffer(EditorState *state, size_t sz)
{
	bufferassert();
	state->bufsz += sz;
	state->buffer = erealloc(state->buffer, state->bufsz);
}

/* motions and editing */
int
isawordbreak(EditorState *state, size_t wordbreakssz, char c)
{
	size_t i = 0;

	assert(state->wordbreaks);
	for(i = 0; i < wordbreakssz; i++) {
		if(c == '\0')
			return 1; /* we'll call nil a wordbreak */
		if(c == state->wordbreaks[i])
			return 1;
	}
	return 0;
}

int
isaprefix(EditorState *state, char c)
{
	size_t i = 0;
	size_t prefixssz = 0;

	if(!state->prefixes)
		return 0;
	prefixssz = strlen(state->prefixes);
	for(i = 0; i < prefixssz; i++)
		if(state->prefixes[i] == c)
			return 1;
	return 0;
}

Wordpos
get_word_position(EditorState *state)
{
	Wordpos res = {0, 0};
	size_t i;
	size_t wordbreakssz;

	assert(state->wordbreaks);
	wordbreakssz = strlen(state->wordbreaks);
	if(state->bufend == 0)
		return res;
	res.end = state->bufpos;
	for(i = state->bufpos; i < state->bufend; i++)
		if(isawordbreak(state, wordbreakssz, state->buffer[i]))
			break;
	res.end = i;
	for(i = res.end; i >= 1; i--)
		if(isawordbreak(state, wordbreakssz, state->buffer[i - 1])) {
			res.start = i;
			if(i > 0 && isaprefix(state, state->buffer[i - 1]))
				res.start -= 1;
			assert(res.start <= res.end);
			return res;
		}
	res.start = 0;
	assert(res.start <= res.end);
	return res;
}

void
insert_char(EditorState *state, char c)
{
	assert(state->bufpos <= state->bufend);
	if(state->bufend == (state->bufsz - 5))
		grow_buffer(state, state->bufsz);

	if(state->bufpos == state->bufend) {
		state->buffer[state->bufpos] = c;
		state->bufpos++;
		state->bufend++;
		return;
	}
	memmove(&state->buffer[state->bufpos + 1], &state->buffer[state->bufpos],
			state->bufend - state->bufpos);
	state->buffer[state->bufpos] = c;
	state->bufpos++;
	state->bufend++;
	return;
}

void
insert_n_char(EditorState *state, char *str, size_t len)
{
	size_t i = 0;

	for(i = 0; i < len; i++)
		insert_char(state, str[i]);
}

size_t
charsize(EditorState *state)
{
	size_t sz = 0;

	do {
		sz++;
	} while((state->buffer[state->bufpos - sz] & 0b11000000) == 0b10000000 &&
			(state->bufpos - sz) > 0);
	return sz;
}

void
backspace_char(EditorState *state)
{
	size_t charsz = 0;
	size_t i = 0;

	if(state->bufpos == 0)
		return;
	bufferassert();
	charsz = charsize(state);
	if(state->bufpos == state->bufend) {
		for(i = 0; i < charsz; i++) {
			state->bufpos--;
			state->bufend--;
			state->buffer[state->bufpos] = 0;
		}
		return;
	}
	state->bufpos -= charsz;
	state->bufend -= charsz;
	memmove(&state->buffer[state->bufpos], &state->buffer[state->bufpos + charsz],
			state->bufend - state->bufpos);
	memset(&state->buffer[state->bufend], 0, charsz);
	return;
}

void
delete_char(EditorState *state)
{
	size_t charsz = 0;

	bufferassert();
	if(state->bufpos == state->bufend)
		return;
	charsz = charsize(state);
	state->bufend -= charsz;
	memmove(&state->buffer[state->bufpos], &state->buffer[state->bufpos + charsz],
			state->bufend - state->bufpos);
	memset(&state->buffer[state->bufend], 0, charsz);
	return;
}

void
delete_word(EditorState *state)
{
	Wordpos wordpos;
	size_t first_end = 0;

	bufferassert();
	if(state->bufpos == 0)
		return;

	wordpos = get_word_position(state);
	dprint("start wordpos = {.start = %lu, .end = %lu}\n", wordpos.start, wordpos.end);
	if(wordpos.end - wordpos.start == 0) {
		first_end = wordpos.end;
		while(wordpos.end - wordpos.start == 0 && state->bufpos > 0) {
			state->bufpos--;
			wordpos = get_word_position(state);
		}
		wordpos.end = first_end;
	}
	dprint("end wordpos = {.start = %lu, .end = %lu}\n", wordpos.start, wordpos.end);

	memset(&state->buffer[wordpos.start], 0, wordpos.end - wordpos.start);
	memmove(&state->buffer[wordpos.start], &state->buffer[wordpos.end],
			state->bufend - wordpos.end);
	state->bufend -= wordpos.end - wordpos.start;
	state->bufpos = wordpos.start;
}

void
delete_to_start(EditorState *state)
{
	size_t old_bufpos = 0;
	size_t old_bufend = 0;

	bufferassert();
	if(state->bufend == 0)
		return;
	if(state->bufpos == 0)
		return;

	old_bufpos = state->bufpos;
	old_bufend = state->bufend;

	memmove(&state->buffer[0], &state->buffer[state->bufpos], state->bufend - state->bufpos);
	state->bufpos = 0;
	state->bufend = state->bufend - old_bufpos;
	memset(&state->buffer[state->bufend], 0, old_bufend - state->bufend);
	bufferassert();
}

void
delete_to_end(EditorState *state)
{
	bufferassert();
	if(state->bufend == 0)
		return;
	if(state->bufpos == 0) {
		memset(state->buffer, 0, state->bufend);
		state->bufend = 0;
		return;
	}

	memset(&state->buffer[state->bufpos], 0, state->bufend - state->bufpos);
	state->bufend = state->bufpos;
}

void
cursor_move_left(EditorState *state)
{
	bufferassert();
	if(state->bufpos == 0)
		return;
	do {
		state->bufpos--;
	} while((state->buffer[state->bufpos] & 0b11000000) == 0b10000000 && state->bufpos > 0);
}

void
cursor_move_right(EditorState *state)
{
	bufferassert();
	if(state->bufpos == state->bufend)
		return;
	do {
		state->bufpos++;
	} while((state->buffer[state->bufpos] & 0b11000000) == 0b10000000 &&
			state->bufpos < state->bufend);
}

void
cursor_home(EditorState *state)
{
	bufferassert();
	if(state->bufpos != 0)
		state->bufpos = 0;
}

void
cursor_end(EditorState *state)
{
	bufferassert();
	if(state->bufpos < state->bufend)
		state->bufpos = state->bufend;
}

void
clear_screen(EditorState *state)
{
	bufferassert();
	state->clear_screen = 1;
}

void
cursor_move_word_left(EditorState *state)
{
	Wordpos nextword;
	size_t first_end;

	bufferassert();
	if(state->bufpos == 0)
		return;

	nextword = get_word_position(state);
	dprint("start nextword = {.start = %lu, .end = %lu}\n", nextword.start, nextword.end);
	if(nextword.start == state->bufpos) {
		state->bufpos--;
		nextword = get_word_position(state);
		dprint("start nextword = {.start = %lu, .end = %lu}\n", nextword.start, nextword.end);
	}
	if(nextword.end - nextword.start == 0) {
		first_end = nextword.end;
		while(nextword.end - nextword.start == 0 && state->bufpos > 0) {
			state->bufpos--;
			nextword = get_word_position(state);
		}
		nextword.end = first_end;
	}
	dprint("end nextword = {.start = %lu, .end = %lu}\n", nextword.start, nextword.end);

	switch(state->word_start) {
	default:
		unreachable();
	case FirstLetter:
		state->bufpos = nextword.start;
		break;
	case FirstBreak:
		if(nextword.start == 0) {
			state->bufpos = 0;
			return;
		}
		if(isawordbreak(state, strlen(state->wordbreaks), state->buffer[nextword.start - 1]))
			state->bufpos = nextword.start - 1;
		else
			state->bufpos = nextword.start;
		break;
	case LastBreak:
		if(nextword.start == 0) {
			state->bufpos = 0;
			return;
		}
		nextword.start--;
		while(nextword.start > 0 &&
			  isawordbreak(state, strlen(state->wordbreaks), state->buffer[nextword.start]))
			nextword.start--;
		if(isawordbreak(state, strlen(state->wordbreaks), state->buffer[nextword.start + 1]))
			nextword.start++;
		state->bufpos = nextword.start;
		break;
	}
}

void
cursor_move_word_right(EditorState *state)
{
	size_t i;

	bufferassert();
	if(state->bufpos == state->bufend)
		return;

	dprint("finding start of first word...");
	for(i = state->bufpos;
		i < state->bufend && isawordbreak(state, strlen(state->wordbreaks), state->buffer[i]); i++)
		;
	;
	dprint("found at %lu\nfinding end of word...", i);
	for(; i < state->bufend && !isawordbreak(state, strlen(state->wordbreaks), state->buffer[i]);
		i++)
		;
	;
	dprint("found at %lu\n", i);

	if(i > state->bufend)
		i = state->bufend;
	state->bufpos = i;
}

/* history */

void
history_add(EditorState *state, char *str)
{
	HistoryEntry *new_entry = nil;

	new_entry = ealloc(sizeof(HistoryEntry));

	new_entry->str = estrdup(str);
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
	bufferassert();
	if(state->inhistory) {
		assert(state->histbuf);
		memset(state->buffer, 0, state->bufend);
		if(ent == nil) {
			memcpy(state->buffer, state->histbuf, state->histbufsz);
			state->bufend = state->histbufsz;
			free(state->histbuf);
			state->inhistory = 0;
		} else {
			if(ent->len >= state->bufsz)
				grow_buffer(state, state->bufsz);
			memcpy(state->buffer, ent->str, ent->len);
			state->bufend = ent->len;
		}
		state->bufpos = state->bufend;
	} else {
		if(ent == nil)
			return;
		state->inhistory = 1;
		state->histbuf = estrdup(state->buffer);
		state->histbufsz = strlen(state->buffer);
		memset(state->buffer, 0, state->bufend);
		if(ent->len >= state->bufsz)
			grow_buffer(state, state->bufsz);
		memcpy(state->buffer, ent->str, ent->len);
		state->bufend = ent->len;
		state->bufpos = state->bufend;
	}
	bufferassert();
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
		state->inhistory = 0;
	}
	bufferassert();
}

/* completion */

int
complete_compstr(const void *a, const void *b)
{
	const char *astr = *(const char **)a;
	const char *bstr = *(const char **)b;

	return strcmp(astr, bstr);
}

void
call_completions_hook(EditorState *state, Wordpos pos)
{
	size_t i = 0;
	char *partial = nil;
	Wordpos partial_pos;
	char **completions = nil;
	size_t completionssz = 0;
	char **completions2 = nil;
	size_t completions2sz = 0;

	if(!state->completions_hook)
		return;

	completion_reset(state);
	partial_pos = get_word_position(state);
	partial = ealloc(partial_pos.end - partial_pos.start + 2);
	memcpy(partial, &state->buffer[partial_pos.start], partial_pos.end - partial_pos.start);
	dprint("partial = %s\n", partial);
	completions = state->completions_hook(partial, pos.start, pos.end);
	free(partial);
	if(completions == nil)
		return;
	for(i = 0; completions[i] != nil; i++) {
		assert(!editor_istracked(completions[i]));
		if(state->dfd > 0)
			dprintf(state->dfd, "completions[%lu] = %p\n", i, completions[i]);
	}
	completionssz = i;
	dprint("completionssz = %lu\n", completionssz);

	if(state->sort_completions) {
		qsort(completions, completionssz, sizeof(char *), &complete_compstr);
		if(state->remove_duplicates) {
			dprint("removing duplicate completions\n");
			completions2 = ealloc(sizeof(char *) * completionssz);
			completions2[0] = completions[0];
			completions2sz = 1;
			for(i = 1; i < completionssz; i++) {
				if(completions[i] == nil)
					break;
				if(strcmp(completions[i], completions2[completions2sz - 1]) == 0) {
					free(completions[i]);
					completions[i] = nil;
					continue;
				}
				completions2[completions2sz] = completions[i];
				completions2sz++;
			}
			dprint("completions2sz = %lu\n", completionssz);
			free(completions);
			completions = completions2;
			completionssz = completions2sz;
		}
		for(i = 0; i < completionssz; i++) {
			if(state->dfd > 0)
				dprintf(state->dfd, "sorted completions[%lu](%lu) = \"%s\"\n", i,
						strlen(completions[i]), completions[i]);
		}
		dprint("completionssz = %lu\n", completionssz);
	}
	state->completions = completions;
	state->completionssz = completionssz;
}

char *
get_next_completion(EditorState *state, Wordpos pos)
{
	if(state->completions == nil)
		call_completions_hook(state, pos);
	if(state->completions == nil)
		return nil;

	if(state->completionsi < state->completionssz)
		return estrdup(state->completions[state->completionsi++]);

	state->completionsi = 0;
	return nil;
}

char *
get_prev_completion(EditorState *state, Wordpos pos)
{
	if(state->completions == nil)
		call_completions_hook(state, pos);
	if(state->completions == nil)
		return nil;

	if(state->completionsi == 0) {
		state->completionsi = state->completionssz;
		return nil;
	}
	return estrdup(state->completions[--state->completionsi]);
}

void /* maybe I should use a rope? */
do_completion(EditorState *state, char *comp, Wordpos pos)
{
	size_t complen = 0;
	size_t i = 0;
	size_t prefixlen = 0;
	size_t suffixlen = 0;

	assert(pos.end <= state->bufend);
	bufferassert();

	if(!state->in_completion) {
		if(comp == nil)
			return;
		dprint("state->in_completion = %d -> 1\n", state->in_completion);
		state->pos = pos;
		state->in_completion = 1;
		state->completebuf = estrdup(state->buffer);
		if(pos.start > 0) {
			state->comp_prefix = ealloc(pos.start + 1);
			memcpy(state->comp_prefix, state->buffer, pos.start);
		} else {
			state->comp_prefix = nil;
		}
		if(state->bufend - pos.end > 0) {
			state->comp_suffix = ealloc((state->bufend - pos.end) + 1);
			memcpy(state->comp_suffix, &state->buffer[pos.end], (state->bufend - pos.end));
		} else {
			state->comp_suffix = nil;
		}
	}

	if(comp == nil) {
		dprint("state->in_completion = %d -> 0\n", state->in_completion);
		memset(state->buffer, 0, state->bufend + 1);
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
		state->completebuf = nil;
		state->comp_prefix = nil;
		state->comp_suffix = nil;
		state->in_completion = 0;
		return;
	}

	dprint("state->completebuf = \"%s\"\n", state->completebuf);
	if(state->comp_prefix)
		dprint("state->comp_prefix = \"%s\"\n", state->comp_prefix);
	else
		dprint("state->comp_prefix = nil\n");

	if(state->comp_suffix)
		dprint("state->comp_suffix = \"%s\"\n", state->comp_suffix);
	else
		dprint("state->comp_suffix = nil\n");
	dprint("state->completionsi = %lu\n", state->completionsi);
	dprint("state->completionssz = %lu\n", state->completionssz);

	complen = strlen(comp);
	memset(state->buffer, 0, state->bufend);
	if(state->comp_prefix) {
		prefixlen = strlen(state->comp_prefix);
		if((complen + prefixlen) - 2 > state->bufsz)
			grow_buffer(state, state->bufsz);
		memcpy(state->buffer, state->comp_prefix, prefixlen);
		i += prefixlen;
	}
	memcpy(&state->buffer[i], comp, complen);
	i += complen;
	if(state->comp_suffix) {
		suffixlen = strlen(state->comp_suffix);
		if((complen + prefixlen + suffixlen) - 2 > state->bufsz)
			grow_buffer(state, state->bufsz);
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

	if(state->completionssz > 0) {
		for(i = 0; i < state->completionssz; i++) {
			assert(!editor_istracked(state->completions[i]));
			if(state->completions[i] == nil)
				continue;
			free(state->completions[i]);
			state->completions[i] = nil;
		}
		free(state->completions);
		state->completions = nil;
		state->completionssz = 0;
	}
	if(state->completebuf) {
		free(state->completebuf);
		state->completebuf = nil;
	}
	state->completionsi = 0;
	state->lastcomplen = 0;
	if(state->comp_prefix) {
		free(state->comp_prefix);
		state->comp_prefix = nil;
	}
	if(state->comp_suffix) {
		free(state->comp_suffix);
		state->comp_suffix = nil;
	}
	dprint("state->in_completion = %d -> 0\n", state->in_completion);
	state->in_completion = 0;
}

void
completion_maybe_reset(EditorState *state)
{
	if(state->bufpos < state->pos.start || state->bufpos > (state->pos.start + state->lastcomplen))
		completion_reset(state);
}

void
completion_next(EditorState *state)
{
	char *comp = nil;

	if(!state->in_completion)
		state->pos = get_word_position(state);
	comp = get_next_completion(state, state->pos);
	if(state->dfd > 0) {
		dprintf(state->dfd, "complete start = %lu, end = %lu\n", state->pos.start, state->pos.end);
		if(comp != nil)
			dprintf(state->dfd, "completion string = \"%s\"\n", comp);
		else
			dprintf(state->dfd, "completion string = nil\n");
	}
	do_completion(state, comp, state->pos);
	if(comp)
		free(comp);
}

void
completion_prev(EditorState *state)
{
	char *comp = nil;

	if(!state->in_completion)
		state->pos = get_word_position(state);
	comp = get_prev_completion(state, state->pos);
	if(state->dfd > 0) {
		dprintf(state->dfd, "complete start = %lu, end = %lu\n", state->pos.start, state->pos.end);
		if(comp != nil)
			dprintf(state->dfd, "completion string = \"%s\"\n", comp);
		else
			dprintf(state->dfd, "completion string = nil\n");
	}
	do_completion(state, comp, state->pos);
	if(comp)
		free(comp);
}

/* keymapping */

char *
pass_key(EditorState *state, int key, void *aux)
{
	used(aux);

	if(key <= KeyDelete)
		insert_char(state, (char)key);
	return nil;
}

int
bindmapping(EditorState *state, int key, Mapping mapping)
{
	if(!state->initialized)
		return -1;

	if(key < ExtKeyOffset && key > KeyMax)
		return -1;
	if(key >= ExtKeyOffset && key > ExtKeyMax)
		return -1;

	if(key >= ExtKeyOffset) {
		state->keymap->ext_keys[key - ExtKeyOffset] = mapping;
		return 0;
	}

	if(key < KeyMax) {
		state->keymap->base_keys[key] = mapping;
		return 0;
	}
	return -1;
}

Result
runmapping(EditorState *state, int key)
{
	Mapping *map;
	char *res;

	if(key > KeyNull && key < KeyMax)
		map = &state->keymap->base_keys[key];
	else if(key >= ExtKeyOffset && key < ExtKeyMax)
		map = &state->keymap->ext_keys[key - ExtKeyOffset];
	else
		return result(nil, -1);

	switch(map->reset_completion) {
	case 1:
		completion_reset(state);
		break;
	case 2:
		completion_maybe_reset(state);
		break;
	}
	if(map->breakkey)
		return result(nil, -3);
	if(map->end_of_file)
		return result(nil, -4);
	if(map->eof_if_empty && state->bufend == 0)
		return result(nil, -4);
	if(map->input_finished)
		return result(nil, -5);

	if(map->base_hook == nil) {
		if(map->hook == nil)
			return result(nil, -2);

		res = map->hook(state, key, map->aux);
		return result(res, 0);
	} else {
		map->base_hook(state);
		return result(nil, 0);
	}
}

void
create_default_mapping(Keymap *map)
{
	memset(&map->base_keys[0], 0, sizeof(Mapping) * BaseKeys);
	memset(&map->ext_keys[0], 0, sizeof(Mapping) * ExtendedKeys);

	map->base_keys[KeyEnter] = (Mapping){
		.hook = nil,
		.base_hook = nil,
		.input_finished = 1,
	};
	map->base_keys[KeySpace] = (Mapping){
		.hook = &pass_key,
		.base_hook = nil,
		.reset_completion = 1,
	};
	map->base_keys[KeyCtrlC] = (Mapping){
		.hook = nil,
		.base_hook = nil,
		.breakkey = 1,
	};
	map->base_keys[KeyCtrlD] = (Mapping){
		.hook = nil,
		.base_hook = &delete_char,
		.reset_completion = 1,
		.eof_if_empty = 1,
	};
	map->base_keys[KeyBackspace] = (Mapping){
		.hook = nil,
		.base_hook = &backspace_char,
		.reset_completion = 1,
	};
	map->base_keys[KeyCtrlH] = (Mapping){
		.hook = nil,
		.base_hook = &backspace_char,
		.reset_completion = 1,
	};
	map->base_keys[KeyTab] = (Mapping){
		.hook = nil,
		.base_hook = &completion_next,
	};
	map->base_keys[KeyCtrlA] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_home,
		.reset_completion = 2,
	};
	map->base_keys[KeyCtrlE] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_end,
		.reset_completion = 2,
	};
	map->base_keys[KeyCtrlW] = (Mapping){
		.hook = nil,
		.base_hook = &delete_word,
		.reset_completion = 1,
	};
	map->base_keys[KeyCtrlU] = (Mapping){
		.hook = nil,
		.base_hook = &delete_to_start,
		.reset_completion = 1,
	};
	map->base_keys[KeyCtrlK] = (Mapping){
		.hook = nil,
		.base_hook = &delete_to_end,
		.reset_completion = 1,
	};
	map->base_keys[KeyCtrlL] = (Mapping){
		.hook = nil,
		.base_hook = &clear_screen,
	};

	map->ext_keys[KeyArrowUp - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &history_next,
		.reset_completion = 1,
	};
	map->ext_keys[KeyArrowDown - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &history_prev,
		.reset_completion = 1,
	};
	map->ext_keys[KeyArrowLeft - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_move_left,
		.reset_completion = 2,
	};
	map->ext_keys[KeyArrowRight - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_move_right,
		.reset_completion = 2,
	};
	map->ext_keys[KeyHome - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_home,
		.reset_completion = 2,
	};
	map->ext_keys[KeyEnd - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_end,
		.reset_completion = 2,
	};
	map->ext_keys[KeyExtDelete - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &delete_char,
		.reset_completion = 1,
	};
	map->ext_keys[KeyShiftTab - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &completion_prev,
		.reset_completion = 0,
	};
	map->ext_keys[KeyAltb - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_move_word_left,
		.reset_completion = 2,
	};
	map->ext_keys[KeyAltf - ExtKeyOffset] = (Mapping){
		.hook = nil,
		.base_hook = &cursor_move_word_right,
		.reset_completion = 2,
	};
}

// clang-format off
char *keynames[] = {
	[KeyNull]	= "(null)",
	[KeyCtrlA]	= "CtrlA",
	[KeyCtrlB]	= "CtrlB",
	[KeyCtrlC]	= "CtrlC",
	[KeyCtrlD]	= "CtrlD",
	[KeyCtrlE]	= "CtrlE",
	[KeyCtrlF]	= "CtrlF",
	[KeyCtrlG]	= "CtrlG",
	[KeyCtrlH]	= "CtrlH",
	[KeyTab]	= "Tab",
	[KeyCtrlJ]	= "CtrlJ",
	[KeyCtrlK]	= "CtrlK",
	[KeyCtrlL]	= "CtrlL",
	[KeyEnter]	= "Enter",
	[KeyCtrlN]	= "CtrlN",
	[KeyCtrlO]	= "CtrlO",
	[KeyCtrlP]	= "CtrlP",
	[KeyCtrlQ]	= "CtrlQ",
	[KeyCtrlR]	= "CtrlR",
	[KeyCtrlS]	= "CtrlS",
	[KeyCtrlT]	= "CtrlT",
	[KeyCtrlU]	= "CtrlU",
	[KeyCtrlV]	= "CtrlV",
	[KeyCtrlW]	= "CtrlW",
	[KeyCtrlX]	= "CtrlX",
	[KeyCtrlY]	= "CtrlY",
	[KeyCtrlZ]	= "CtrlZ",
	[KeySpace]	= "Space",
	[KeyEscape]	= "Escape",
};

char *extkeynames[] = {
	[0] = "ArrowLeft",
	[1] = "ArrowRight",
	[2] = "ArrowUp",
	[3] = "ArrowDown",
	[4] = "PageUp",
	[5] = "PageDown",
	[6] = "Home",
	[7] = "End",
	[8] = "Insert",
	[9] = "ExtDelete",
	[10] = "PF1",
	[11] = "PF2",
	[12] = "PF3",
	[13] = "PF4",
	[14] = "PF5",
	[15] = "PF6",
	[16] = "PF7",
	[17] = "PF8",
	[18] = "PF9",
	[19] = "PF10",
	[20] = "PF11",
	[21] = "PF12",
	[22] = "ShiftTab",
	[23] = "Alta",
	[24] = "Altb",
	[25] = "Altc",
	[26] = "Altd",
	[27] = "Alte",
	[28] = "Altf",
	[29] = "Altg",
	[30] = "Alth",
	[31] = "Alti",
	[32] = "Altj",
	[33] = "Altk",
	[34] = "Altl",
	[35] = "Altm",
	[36] = "Altn",
	[37] = "Alto",
	[38] = "Altp",
	[39] = "Altq",
	[40] = "Altr",
	[41] = "Alts",
	[42] = "Altt",
	[43] = "Altu",
	[44] = "Altv",
	[45] = "Altw",
	[46] = "Altx",
	[47] = "Alty",
	[48] = "Altz",
	[49] = "AltE",
	[50] = "AltF",
	[51] = "AltG",
	[52] = "AltH",
	[53] = "AltI",
	[54] = "AltJ",
	[55] = "AltK",
	[56] = "AltL",
	[57] = "AltM",
	[58] = "AltN",
	[59] = "AltO",
	[60] = "AltP",
	[61] = "AltQ",
	[62] = "AltR",
	[63] = "AltS",
	[64] = "AltT",
	[65] = "AltU",
	[66] = "AltV",
	[67] = "AltW",
	[68] = "AltX",
	[69] = "AltY",
	[70] = "AltZ",
	[71] = "CtrlLeft",
	[72] = "CtrlRight",
	[73] = "CtrlUp",
	[74] = "CtrlDown",
};
// clang-format on

char *
key2name(int key)
{
	if(key == KeyBackspace)
		return "Backspace";

	if(key == KeySpace)
		return keynames[KeySpace];

	if(key >= KeyNull && key <= KeyEscape)
		return keynames[key];

	if(key >= ExtKeyOffset && key < ExtKeyMax)
		return extkeynames[key - ExtKeyOffset];

	return "Unknown";
}

int
name2key(char *name)
{
	int i;

	if(strcmp(name, "Backspace") == 0)
		return KeyBackspace;

	if(strcmp(name, keynames[KeySpace]) == 0)
		return KeySpace;

	for(i = 0; i <= KeyEscape; i++)
		if(strcmp(name, keynames[i]) == 0)
			return i;

	for(i = ExtKeyOffset; i < ExtKeyMax; i++)
		if(strcmp(name, extkeynames[i - ExtKeyOffset]) == 0)
			return i;

	return -1;
}

char *
getcurrentline(EditorState *state)
{
	return estrndup(state->buffer, state->bufend + 1);
}

/* the big kahuna */

char *
fallback_editor(EditorState *state)
{
	/* this at a minimum needs initialize_editor to have been called
	 * basically you should never call this yourself. let line_editor
	 * do that for you.
	 */

	char *buffer = nil;
	char *res = nil;
	size_t i = 0;

	buffer = ealloc(EDITINITIALBUFSZ);

	if(state->prompt1)
		write(state->ofd, state->prompt1, strlen(state->prompt1));
	read(state->ifd, buffer, EDITINITIALBUFSZ - 2);
	for(i = 0; i < EDITINITIALBUFSZ; i++)
		if(buffer[i] == '\n') {
			buffer[i] = '\0';
			break;
		}
	res = estrdup(buffer);
	free(buffer);
	return res;
}

char * /* budget readline */
basic_editor(EditorState *state)
{
	char c;
	char *res;
	char seq[6];
	enum { StateRead, StateDone } readstate = StateRead;
	size_t readn = 0;

	if(!state->initialized)
		return fallback_editor(state);
	if(state->force_fallback)
		return fallback_editor(state);

	rawmode_on(state);
	if(reset_editor(state) < 0)
		goto fail;

	while(readstate == StateRead) {
		refresh(state);
		dprint("read1...");
		if(read(state->ifd, &c, 1) < 0)
			goto fail;
top:
		switch(c) {
		default:
			if(c >= ' ' && c <= '~') {
				insert_char(state, c);
			} else if((c & 0b11000000) == 0b11000000) {
				/* utf-8 stuff */
				seq[0] = c;
				if((c & 0b11100000) == 0b11000000)
					readn = 1;
				else if((c & 0b11110000) == 0b11100000)
					readn = 2;
				else if((c & 0b11111000) == 0b11110000)
					readn = 3;
				else
					unreachable();
				dprint("utf8 read %lu...", readn);
				read(state->ifd, &seq[1], readn);
				insert_n_char(state, &seq[0], readn + 1);
			} else if((c & 0b11000000) == 0b10000000) {
				/* something got screwed up with utf-8
				 * just skip until we get something sensible
				 */
				continue;
			}
			completion_reset(state);
			break;
		case KeyEnter:
			readstate = StateDone;
			break;
		case KeyCtrlC:
			goto fail;
			break;
		case KeyCtrlH:
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
		case KeyCtrlW:
			delete_word(state);
			completion_reset(state);
			break;
		case KeyEscape:
			dprint("escape read...");
			if(read(state->ifd, &seq[0], 1) < 0)
				break;
			if(seq[0] == '[') {
				completion_maybe_reset(state);
				dprint("escape [ read...");
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

	write(state->ofd, "\r\n", 2);
	rawmode_off(state);
	res = estrndup(state->buffer, state->bufend + 1);
	return res;

fail:
	write(state->ofd, "\r\n", 2);
	rawmode_off(state);
	return nil;
}

/*
 * line_editor is the terminal keycode -> internal keycode converter
 * It will insert everything printable, and passes everything else to the keymap.
 * I am toying with a 'bring your own read' version of this similar to linenoise
 * for integration with kqueue/select/etc. I don't think that will be at all
 * valuable for es, but if this has life outside of here it might be of value.
 */
char *
line_editor(EditorState *state)
{
	char c;
	char seq[6];
	enum { StateRead, StateDone, StateCancel } readstate = StateRead;
	int key = 0;
	Result r;
	char *str;
	size_t readn = 0;

	if(!state->initialized)
		return fallback_editor(state);
	if(state->force_fallback)
		return fallback_editor(state);

	rawmode_on(state);
	if(reset_editor(state) < 0)
		goto fail;

	while(readstate == StateRead) {
		key = 0;
		memset(&seq[0], 0, sizeof(seq));
		refresh(state);
		reconfigure_terminal(state);
		dprint("read 1...");
		if(read(state->ifd, &c, 1) < 0)
			goto fail;
		if(c > ' ' && c <= '~') {
			dprint("\n");
			insert_char(state, c);
			completion_reset(state);
			continue;
		} else if((c & 0b11000000) == 0b11000000) {
			/* utf-8 stuff */
			seq[0] = c;
			if((c & 0b11100000) == 0b11000000)
				readn = 1;
			else if((c & 0b11110000) == 0b11100000)
				readn = 2;
			else if((c & 0b11111000) == 0b11110000)
				readn = 3;
			else
				unreachable();

			dprint("utf8 read %lu...", readn);
			read(state->ifd, &seq[1], readn);
			dprint("\n");
			completion_reset(state);
			insert_n_char(state, &seq[0], readn + 1);
			continue;
		} else if((c & 0b11000000) == 0b10000000) {
			/* something got screwed up with utf-8
			 * just skip until we get something sensible
			 */
			continue;
		} else if(c == KeySpace) {
			key = KeySpace;
		} else if(c == KeyEnter) {
			key = KeyEnter;
		} else if(c == KeyBackspace) {
			key = KeyBackspace;
		} else if(c == KeyEscape) {
			dprint("escape read...");
			if(read(state->ifd, &seq[0], 1) < 0)
				continue;
			if(seq[0] == '[') {
				dprint("[ read...");
				if(read(state->ifd, &seq[1], 1) < 0)
					continue;
				switch(seq[1]) {
				default:
					if(state->dfd > 0)
						dprintf(state->dfd, "\ngot unknown code %c%c\n", seq[0], seq[1]);
					continue;
				case 'A':
					key = KeyArrowUp;
					break;
				case 'B':
					key = KeyArrowDown;
					break;
				case 'C':
					key = KeyArrowRight;
					break;
				case 'D':
					key = KeyArrowLeft;
					break;
				case 'H':
					key = KeyHome;
					break;
				case 'F':
					key = KeyEnd;
					break;
				case 'M':
				case 'P':
					key = KeyExtDelete;
					break;
				case '@':
				case 'L':
					key = KeyInsert;
					break;
				case 'Z':
					key = KeyShiftTab;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					dprint("number read...");
					if(read(state->ifd, &seq[2], 1) < 0)
						continue;
					if(seq[2] == '~') {
						switch(seq[1]) {
						default:
							if(state->dfd > 0)
								dprintf(state->dfd, "\ngot unknown code %c%c%c\n", seq[0], seq[1],
										seq[2]);
							continue;
						case '5':
							key = KeyPageUp;
							break;
						case '6':
							key = KeyPageDown;
							break;
						case '4':
							key = KeyEnd;
							break;
						case '3':
							key = KeyExtDelete;
							break;
						case '2':
							key = KeyInsert;
							break;
						case '1':
							key = KeyHome;
							break;
						}
					} else if(seq[2] >= '0' && seq[2] <= '9') {
						/* this is like wtf? I found this experimentally. It is at least
						 * true in konsole 25.08.1 on FreeBSD 14.3. subject to change?
						 */
						dprint("PF key read...");
						if(read(state->ifd, &seq[3], 1) < 0)
							continue;
						if(seq[3] != '~') {
							if(state->dfd > 0)
								dprintf(state->dfd, "\ngot unknown code %c%c%c%c\n", seq[0], seq[1],
										seq[2], seq[3]);
							continue;
						}
						if(seq[1] == '1') {
							switch(seq[2]) {
							default:
								if(state->dfd > 0)
									dprintf(state->dfd, "\ngot unknown code %c%c%c%c\n", seq[0],
											seq[1], seq[2], seq[3]);
								continue;
							case '1':
								key = KeyPF1;
								break;
							case '2':
								key = KeyPF2;
								break;
							case '3':
								key = KeyPF3;
								break;
							case '4':
								key = KeyPF4;
								break;
							case '5':
								key = KeyPF5;
								break;
							case '7':
								key = KeyPF6;
								break;
							case '8':
								key = KeyPF7;
								break;
							case '9':
								key = KeyPF8;
								break;
							}
						} else if(seq[1] == '2') {
							switch(seq[2]) {
							default:
								if(state->dfd > 0)
									dprintf(state->dfd, "\ngot unknown code %c%c%c%c\n", seq[0],
											seq[1], seq[2], seq[3]);
								continue;
							case '0':
								key = KeyPF9;
								break;
							case '1':
								key = KeyPF10;
								break;
							case '3':
								key = KeyPF11;
								break;
							case '4':
								key = KeyPF12;
								break;
							}
						} else {
							if(state->dfd > 0)
								dprintf(state->dfd, "\ngot unknown code %c%c%c%c\n", seq[0], seq[1],
										seq[2], seq[3]);
							continue;
						}
					} else if(seq[2] == ';' && seq[1] == '1') { // Ctrl+Arrow
						dprint("Ctrl+Arrow read 1...");
						if(read(state->ifd, &seq[3], 1) < 0)
							continue;
						if(seq[3] != '5') {
							if(state->dfd > 0)
								dprintf(state->dfd, "\ngot unknown code %c%c%c%c\n", seq[0], seq[1],
										seq[2], seq[3]);
							continue;
						}
						dprint("Ctrl+Arrow read 2...");
						if(read(state->ifd, &seq[4], 1) < 0)
							continue;
						switch(seq[4]) {
						default:
							dprint("\ngot unknown code %c%c%c%c%c\n", seq[0], seq[1], seq[2],
								   seq[3], seq[4]);
							continue;
						case 'A':
							key = KeyCtrlUp;
							break;
						case 'B':
							key = KeyCtrlDown;
							break;
						case 'C':
							key = KeyCtrlRight;
							break;
						case 'D':
							key = KeyCtrlLeft;
							break;
						}
					}
					break;
				}
			} else if(seq[0] == 'O') {
				dprint("O read...");
				if(read(state->ifd, &seq[1], 1) < 0)
					continue;
				switch(seq[1]) {
				default:
					if(seq[1] >= 'A' && seq[1] <= 'L') {
						key = KeyPF1 + (seq[1] - 'A');
					} else {
						if(state->dfd > 0)
							dprintf(state->dfd, "\ngot unknown code %c%c\n", seq[0], seq[1]);
						continue;
					}
					break;
				case ' ': /* sun application space */
					break;
				case 'I':
					key = KeyTab;
					break;
				case 'M':
					key = KeyEnter;
					break;
				case 'P':
					key = KeyPF1;
					break;
				case 'Q':
					key = KeyPF2;
					break;
				case 'R':
					key = KeyPF3;
					break;
				case 'S':
					key = KeyPF4;
					break;
				case 'H':
					key = KeyHome;
					break;
				case 'F':
					key = KeyEnd;
					break;
				}
			} else if(seq[0] >= 'A' && seq[0] <= 'Z') {
				switch(seq[0]) {
				case 'A':
					key = KeyArrowUp;
					break;
				case 'B':
					key = KeyArrowDown;
					break;
				case 'C':
					key = KeyArrowRight;
					break;
				case 'D':
					key = KeyArrowLeft;
					break;
				default:
					key = (seq[0] - 'E') + KeyAltE;
					break;
				}
			} else if(seq[0] >= 'a' && seq[0] <= 'z') {
				key = (seq[0] - 'a') + KeyAlta;
			} else {
				if(state->dfd > 0)
					dprintf(state->dfd, "\ngot code %c\n", seq[0]);
				continue;
			}
		} else {
			key = c;
		}

		dprint("\ngot key %s\n", key2name(key));
		r = runmapping(state, key);
		switch(status(r)) {
		default:
			unreachable();
			break;
		case -5:
			readstate = StateDone;
			break;
		case -4:
			goto fail;
			break;
		case -2:
			if(state->dfd > 0)
				dprintf(state->dfd, "got unmapped key %s\n", key2name(key));
			break;
		case -1:
			if(state->dfd > 0)
				dprintf(state->dfd, "got invalid key %d\n", key);
			break;
		case -3:
			readstate = StateCancel;
			break;
		case 0:
			if(r.ptr != nil) {
				str = r.ptr;
				memset(state->buffer, 0, state->bufend);
				memcpy(state->buffer, str, strlen(str));
				state->bufend = strlen(str);
				state->bufpos = state->bufend;
				free(str);
			}
			break;
		}
	}

	state->done_reading = 1;
	state->bufpos = state->bufend;
	refresh(state);
	write(state->ofd, "\r\n", 2);
	rawmode_off(state);

	if(readstate == StateCancel)
		return estrdup("");
	if(state->bufend == 0)
		return estrdup("");
	return estrndup(state->buffer, state->bufend + 1);

fail:
	write(state->ofd, "\r\n", 2);
	rawmode_off(state);
	return nil;
}
