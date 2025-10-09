#include <sys/ttycom.h>
#include <termios.h>

#define EDITINITIALBUFSZ 4096

enum {
	/* base keys */
	KeyNull = 0,
	KeyCtrlA = 1,
	KeyCtrlB = 2,
	KeyCtrlC = 3,
	KeyCtrlD = 4,
	KeyCtrlE = 5,
	KeyCtrlF = 6,
	KeyCtrlG = 7,
	KeyCtrlH = 8,
	KeyCtrlI = 9,
	KeyCtrlJ = 10,
	KeyCtrlK = 11,
	KeyCtrlL = 12,
	KeyCtrlM = 13,
	KeyCtrlN = 14,
	KeyCtrlO = 15,
	KeyCtrlP = 16,
	KeyCtrlQ = 17,
	KeyCtrlR = 18,
	KeyCtrlS = 19,
	KeyCtrlT = 20,
	KeyCtrlU = 21,
	KeyCtrlV = 22,
	KeyCtrlW = 23,
	KeyCtrlX = 24,
	KeyCtrlY = 25,
	KeyCtrlZ = 26,

	KeyEscape = 27,
	KeyBackspace = 127,
	KeyDelete = 127,
	KeyTab = 9,
	KeyEnter = 13,

	/* extended keys */
	ExtKeyOffset = 1000,
	KeyArrowLeft = 1000,
	KeyArrowRight = 1001,
	KeyArrowUp = 1002,
	KeyArrowDown = 1003,
	KeyPageUp = 1004,
	KeyPageDown = 1005,
	KeyHome = 1006,
	KeyEnd = 1007,
	KeyInsert = 1008,
	KeyExtDelete = 1009,
	ExtPFOffset = 1010,
	KeyPF1 = 1010,
	KeyPF2 = 1011,
	KeyPF3 = 1012,
	KeyPF4 = 1013,
	
};

typedef struct Position Position;
typedef struct EditorState EditorState;
typedef struct HistoryEntry HistoryEntry;
typedef struct Wordpos Wordpos;

struct Position{
	int cols;
	int lines;
};

struct Wordpos {
	size_t start;
	size_t end;
};

struct EditorState {
	int ifd;
	int ofd;
	int dfd;
	struct termios old_termios;
	int rawmode;
	Position size;
	Position position;
	Position last_end;
	char *prompt1;
	size_t prompt1sz;
	char *prompt2;
	size_t prompt2sz;
	int which_prompt;
	char *buffer;
	size_t bufsz;
	size_t bufpos;
	size_t bufend;
	int initialized;
	char *histbuf;
	size_t histbufsz;
	int inhistory;
	HistoryEntry *history;
	HistoryEntry *cur;
	// completions list and generator
	char **completions;
	size_t completionsi;
	size_t completionssz;
	char** (*completions_hook)(char*,int, int);
	// completer state
	char *completebuf;
	size_t lastcomplen;
	char *wordbreaks;
	int in_completion;
	Wordpos pos;
	char *comp_prefix;
	char *comp_suffix;
};

struct HistoryEntry {
	char *str;
	size_t len;
	HistoryEntry *next;
	HistoryEntry *prev;
};

/* basics */
extern int rawmode_on(EditorState *state);
extern int rawmode_off(EditorState *state);
extern int initialize_editor(EditorState *state, int ifd, int ofd);
extern void editor_debugging(EditorState *state, int dfd);
extern void free_editor(EditorState *state);
extern int reset_editor(EditorState *state);
extern void refresh(EditorState *state);
extern void set_prompt1(EditorState *state, char *str);
extern void set_prompt2(EditorState *state, char *str);
extern void set_complete_hook(EditorState *state, char** (*hook)(char*, int, int));

/* motions and editing */
extern void refresh(EditorState *state);
extern int insert_char(EditorState *state, char c);
extern int backspace_char(EditorState *state);
extern int delete_char(EditorState *state);
extern void cursor_move_left(EditorState *state);
extern void cursor_move_right(EditorState *state);
extern void cursor_move_home(EditorState *state);
extern void cursor_move_end(EditorState *state);

/* history */
extern void history_add(EditorState *state, char *str);
extern void history_clear(EditorState *state);
extern void history_cancel(EditorState *state);
extern void history_next(EditorState *state);
extern void history_prev(EditorState *state);

/* completions */
extern void completion_reset(EditorState *state);
extern void completion_next(EditorState *state);
extern void completion_prev(EditorState *state);

/* editors */
extern char *basic_editor(EditorState *state);

