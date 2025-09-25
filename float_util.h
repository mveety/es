extern double termtof(Term *term, char *funname, int arg);
extern double listtof(List *list, char *funname, int arg);
extern List *floattolist(double num, char *funname);
extern int64_t termtoint(Term *term, char *funname, int arg);
extern int64_t listtoint(List *list, char *funname, int arg);
