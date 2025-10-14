#include <sys/ttycom.h>
#include <termios.h>

#define EDITINITIALBUFSZ 4096

enum {
	/* base keys */
	KeyMax = 128,
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
	ExtKeyMax = 1071,
#define ExtendedKeys 71
	KeyArrowLeft = 1000,  // x
	KeyArrowRight = 1001, // x
	KeyArrowUp = 1002,	  // x
	KeyArrowDown = 1003,  // x
	KeyPageUp = 1004,	  // x
	KeyPageDown = 1005,	  // x
	KeyHome = 1006,		  // x
	KeyEnd = 1007,		  // x
	KeyInsert = 1008,	  // x
	KeyExtDelete = 1009,  // x
	ExtPFOffset = 1010,
	KeyPF1 = 1010,		// x
	KeyPF2 = 1011,		// x
	KeyPF3 = 1012,		// x
	KeyPF4 = 1013,		// x
	KeyPF5 = 1014,		// x
	KeyPF6 = 1015,		// x
	KeyPF7 = 1016,		// x
	KeyPF8 = 1017,		// x
	KeyPF9 = 1018,		// x
	KeyPF10 = 1019,		// x
	KeyPF11 = 1020,		// x
	KeyPF12 = 1021,		// x
	KeyShiftTab = 1022, // x
	KeyAlta = 1023,		// x
	KeyAltb = 1024,		// x
	KeyAltc = 1025,		// x
	KeyAltd = 1026,		// x
	KeyAlte = 1027,		// x
	KeyAltf = 1028,		// x
	KeyAltg = 1029,		// x
	KeyAlth = 1030,		// x
	KeyAlti = 1031,		// x
	KeyAltj = 1032,		// x
	KeyAltk = 1033,		// x
	KeyAltl = 1034,		// x
	KeyAltm = 1035,		// x
	KeyAltn = 1036,		// x
	KeyAlto = 1037,		// x
	KeyAltp = 1038,		// x
	KeyAltq = 1039,		// x
	KeyAltr = 1040,		// x
	KeyAlts = 1041,		// x
	KeyAltt = 1042,		// x
	KeyAltu = 1043,		// x
	KeyAltv = 1044,		// x
	KeyAltw = 1045,		// x
	KeyAltx = 1046,		// x
	KeyAlty = 1047,		// x
	KeyAltz = 1048,		// x
	KeyAltE = 1049,
	KeyAltF = 1050,
	KeyAltG = 1051,
	KeyAltH = 1052,
	KeyAltI = 1053,
	KeyAltJ = 1054,
	KeyAltK = 1055,
	KeyAltL = 1056,
	KeyAltM = 1057,
	KeyAltN = 1058,
	KeyAltO = 1059,
	KeyAltP = 1060,
	KeyAltQ = 1061,
	KeyAltR = 1062,
	KeyAltS = 1063,
	KeyAltT = 1064,
	KeyAltU = 1065,
	KeyAltV = 1066,
	KeyAltW = 1067,
	KeyAltX = 1068,
	KeyAltY = 1069,
	KeyAltZ = 1070,
};

typedef struct Position Position;
typedef struct EditorState EditorState;
typedef struct HistoryEntry HistoryEntry;
typedef struct Wordpos Wordpos;
typedef struct Mapping Mapping;
typedef struct Keymap Keymap;

struct Position {
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
	char *term;
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
	char **(*completions_hook)(char *, int, int);
	// completer state
	char *completebuf;
	size_t lastcomplen;
	char *wordbreaks;
	int in_completion;
	Wordpos pos;
	char *comp_prefix;
	char *comp_suffix;
	// keymapping
	Keymap *keymap;
};

struct HistoryEntry {
	char *str;
	size_t len;
	HistoryEntry *next;
	HistoryEntry *prev;
};

struct Mapping {
	char *(*hook)(EditorState *, int, void *);
	void (*base_hook)(EditorState *);
	void *aux;
	int breakkey;
	int reset_completion;
};

struct Keymap {
	Mapping base_keys[128];
	Mapping ext_keys[ExtendedKeys];
};

/* basics */
extern int rawmode_on(EditorState *state);
extern int rawmode_off(EditorState *state);
/* NOTE: initialize_editor calls getenv to get $TERM */
extern int initialize_editor(EditorState *state, int ifd, int ofd);
extern void editor_debugging(EditorState *state, int dfd);
extern void free_editor(EditorState *state);
extern int reset_editor(EditorState *state);
extern void refresh(EditorState *state);
extern void set_prompt1(EditorState *state, char *str);
extern void set_prompt2(EditorState *state, char *str);
extern void set_complete_hook(EditorState *state, char **(*hook)(char *, int, int));

/* motions and editing */
extern void refresh(EditorState *state);
extern void insert_char(EditorState *state, char c);
extern void insert_n_char(EditorState *state, char *str, size_t n);
extern void backspace_char(EditorState *state);
extern void delete_char(EditorState *state);
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

/* keymapping */
extern int bindmapping(EditorState *state, int key, Mapping mapping);
extern char *key2name(int key);
extern int name2key(char *name);

/* editors */
extern char *basic_editor(EditorState *state);
extern char *line_editor(EditorState *state);
