/* $Id$ */

#ifndef LIBCL_INTERNAL_H
#define LIBCL_INTERNAL_H

#include <sys/types.h>

#include <limits.h>
#include <stddef.h>
#include <stdarg.h>

enum ui_event {
	UI_CODEPOINT,

	UI_BACKSPACE,
	UI_DELETE,
	UI_DELETE_TO_EOL,
	UI_DELETE_WORD,
	UI_CANCEL,

	UI_HIST_PREV,
	UI_HIST_NEXT,

	UI_CURSOR_LEFT,
	UI_CURSOR_RIGHT,
	UI_CURSOR_LEFT_WORD,
	UI_CURSOR_RIGHT_WORD,
	UI_CURSOR_EOL,
	UI_CURSOR_SOL
};

struct cl_peer;
struct cl_command;

struct cl_tree {
	struct trie *root;

	size_t command_count;
	const struct cl_command *commands;
	size_t field_count;
	const struct cl_field *fields;

	int (*printprompt)(struct cl_peer *p, int mode);
	int (*visible)(struct cl_peer *p, int mode, int modes);
	int (*vprintf)(struct cl_peer *p, const char *fmt, va_list ap);
};

struct cl_event {
	enum ui_event type;

	union {
		const char *utf8;	/* UI_CODEPOINT */
	} u;
};

struct ioctx;

struct io {
	struct ioctx *(*create)(int fd);
	void          (*destroy)(struct ioctx *p);
	ssize_t       (*read)(struct cl_peer *p, struct ioctx *ioctx, const void *data, size_t len);
};

struct trie_command {
	const char *command;
	int modes;
	int fields;

	void (*callback)(struct cl_peer *p, const char *command, int mode,
		int argc, char *argv[]);
};

struct trie {
	struct trie *edge[UCHAR_MAX];

	struct trie_command *command;
};

struct cl_peer {
	struct cl_tree *tree;
	int mode;

	struct readctx *rctx;
	struct ioctx  *ioctx;
	struct io io;

	void *opaque;
};

struct trie *
trie_add(struct trie **trie, const char *s, const struct cl_command *command);

const struct cl_field *
find_field(struct cl_tree *t, int id);

struct readctx *read_create(void);
void read_destroy(struct readctx *read);
const char *read_get_field(struct readctx *rc, int id);
int getc_main(struct cl_peer *p, struct cl_event *event);

extern struct io io_plain;
extern struct io io_ecma48;
extern struct io io_telnet;

#endif

