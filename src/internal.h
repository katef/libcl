/*
 * Copyright 2012-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LIBCL_INTERNAL_H
#define LIBCL_INTERNAL_H

#include <sys/types.h>

#include <limits.h>
#include <stddef.h>
#include <stdarg.h>

enum ui_event {
	UI_CODEPOINT,
	UI_HELP,

	UI_BACKSPACE,
	UI_DELETE,
	UI_DELETE_LINE,
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

enum ui_output {
	OUT_BACKSPACE_AND_DELETE,
	OUT_SAVE,
	OUT_RESTORE_AND_DELETE_TO_EOL
};

enum edit_flags {
	EDIT_ECHO = 1 << 0,
	EDIT_TRIE = 1 << 1,
	EDIT_HIST = 1 << 2
};

struct cl_peer;
struct cl_command;

struct cl_tree {
	struct trie *root;

	size_t command_count;
	const struct cl_command *commands;
	size_t field_count;
	const struct cl_field *fields;

	int (*ready)(struct cl_peer *p);
	int (*motd)(struct cl_peer *p);
	const char *(*ttype)(struct cl_peer *p);
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

struct cl_term {
	const char *cub1; /* cursor left */
	const char *dch1; /* delete character */
	const char *el;   /* clear to EOL */
	const char *sc;   /* save cursor */
	const char *rc;   /* restore cursor */
};

struct ioctx;
struct cl_chctx;

struct io {
	int         (*create)(struct cl_peer *p, struct cl_chctx *chctx);
	void        (*destroy)(struct cl_peer *p, struct cl_chctx *chctx);
	ssize_t     (*read)(struct cl_peer *p, struct cl_chctx *chctx,
	                    const void *data, size_t len);
	ssize_t     (*send)(struct cl_peer *p, struct cl_chctx *chctx,
	                    enum ui_output output);
	int         (*vprintf)(struct cl_peer *p, struct cl_chctx *chctx,
	                      const char *fmt, va_list ap);
	int         (*printf)(struct cl_peer *p, struct cl_chctx *chctx,
	                      const char *fmt, ...);
	const char *(*ttype)(struct cl_peer *p, struct cl_chctx *chctx);
};

struct cl_chctx {
	const struct io *ioapi;
	struct ioctx    *ioctx;
};

struct trie_command {
	const char *command;
	int modes;
	int fields;

	void (*callback)(struct cl_peer *p, const char *command, int mode,
		int argc, const char *argv[]);
};

struct trie {
	struct trie *edge[UCHAR_MAX + 1];

	struct trie_command *command;
};

struct termctx;

struct cl_chain;

struct cl_peer {
	struct cl_tree *tree;
	const char *ttype;
	int mode;

	struct cl_term term;

	struct termctx *tctx;
	struct readctx *rctx;
	struct editctx *ectx;
	struct cl_chctx *chctx;

	const struct cl_chain *chain;

	void *opaque;
};

enum lex_type {
	TOK_ERROR,
	TOK_WORD,
	TOK_STRING,
	TOK_PIPE
};

struct lex_tok {
	enum lex_type type;
	struct {
		const char *start;
		const char *end;
	} src, dst;
};

struct trie *
trie_add(struct trie **trie, const char *s, const struct cl_command *command);
const struct trie *
trie_walk(const struct trie *trie, const char *s, size_t len);
const struct trie *
trie_cycle(struct cl_peer *p, const struct trie *trie, int mode, char c,
	const struct trie *prev);

/* returns NULL for ambiguity */
const struct trie *
trie_run(struct cl_peer *p, const struct trie *trie, int mode, char c);

void
trie_help(struct cl_peer *p, const struct trie *trie, int mode);

const struct cl_field *
find_field(struct cl_tree *t, int id);

struct readctx *read_create(void);
void read_destroy(struct readctx *read);
const char *read_get_field(struct readctx *rc, int id);
int getc_main(struct cl_peer *p, const struct cl_event *event);

struct termctx *term_create(struct cl_term *term, const char *name);
void term_destroy(struct termctx *t);

struct lex_tok *
lex_next(struct lex_tok *new, const char **src, char **dst);

struct editctx *edit_create(void);
void edit_destroy(struct editctx *ectx);
char *edit_release(struct editctx *ectx);
int edit_push(struct cl_peer *p, const struct cl_event *event, enum edit_flags flags);

#endif

