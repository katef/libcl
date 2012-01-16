/* $Id$ */

#ifndef LIBCL_INTERNAL_H
#define LIBCL_INTERNAL_H

#include <limits.h>
#include <stddef.h>
#include <stdarg.h>

enum readstate {
	STATE_NEW, STATE_CHAR, STATE_FIELD
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
	int (*visible)(struct cl_peer *p, int mode, const struct cl_command *command);
	int (*vprintf)(struct cl_peer *p, const char *fmt, va_list ap);
};

struct trie {
	struct trie *edge[UCHAR_MAX];

	const struct cl_command *command;
};

struct cl_peer {
	struct cl_tree *tree;
	int mode;

	/* XXX: private to cl_read.c */
	enum readstate state;
	struct trie *t;
	int fields;

	void *opaque;
};

struct trie *
trie_add(struct trie **trie, const char *s, const struct cl_command *command);

const struct cl_field *
find_field(struct cl_tree *t, int id);

#endif

