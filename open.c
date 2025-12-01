/* open.c -- to insulate <fcntl.h> from the rest of es ($Revision: 1.1.1.1 $) */

#include <fcntl.h>
#define REQUIRE_FCNTL 1

#include "es.h"

#if NeXT
extern int open(const char *, int, ...);
#endif

/*
 * Opens a file with the necessary flags.  Assumes the following order:
 *	typedef enum {
 *		oOpen, oCreate, oAppend, oReadCreate, oReadTrunc oReadAppend
 *	} OpenKind;
 */

static int mode_masks[] = {
	O_RDONLY,								   /* oOpen */
	O_WRONLY | O_CREAT | O_TRUNC,			   /* oCreate */
	O_WRONLY | O_CREAT | O_APPEND,			   /* oAppend */
	O_RDWR | O_CREAT,						   /* oReadWrite */
	O_RDWR | O_CREAT | O_TRUNC,				   /* oReadCreate */
	O_RDWR | O_CREAT | O_APPEND,			   /* oReadAppend */
	O_RDONLY | O_CLOEXEC,					   /* ceOpen */
	O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,  /* ceCreate */
	O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, /* ceAppend */
	O_RDWR | O_CREAT | O_CLOEXEC,			   /* ceReadWrite */
	O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC,	   /* ceReadCreate */
	O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC,   /* ceReadAppend */
};

extern int
eopen(char *name, OpenKind k)
{
	assert((unsigned)k < arraysize(mode_masks));
	return open(name, mode_masks[k], 0666);
}
