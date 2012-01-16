/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"

struct cl_tree *
cl_create(size_t command_count, const struct cl_command commands[],
	size_t field_count, const struct cl_field fields[],
	int (*printprompt)(struct cl_peer *p, int mode),
	int (*visible)(struct cl_peer *p, int mode, const struct cl_command *command),
	int (*vprintf)(struct cl_peer *p, const char *fmt, va_list ap))
{
	size_t i;
	struct cl_tree *new;

	assert((command_count == 0) == (commands == NULL));
	assert((field_count   == 0) == (fields   == NULL));
	assert(printprompt != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->root          = NULL;
	new->printprompt   = printprompt;
	new->visible       = visible;
	new->vprintf       = vprintf;

	new->commands      = commands;
	new->command_count = command_count;
	new->fields        = fields;
	new->field_count   = field_count;

	for (i = 0; i < command_count; i++) {
		struct trie *node;
		size_t j;

		/* things to assert about .command: does not start with space; all alnum or ' '; all isprint;
		 * no non-' ' whitespace */
		node = trie_add(&new->root, commands[i].command, &commands[i]);
		if (node == NULL) {
			cl_destroy(new);
			return NULL;
		}

		for (j = 0; j < field_count; j++) {
			/* TODO: do we need to store these in any other form? probably not... */
		}
	}

	return new;
}

void
cl_destroy(struct cl_tree *t)
{
}

struct cl_peer *
cl_accept(struct cl_tree *t)
{
	struct cl_peer *new;

	assert(t != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->tree   = t;
	new->mode   = 0;
	new->state  = STATE_NEW;
	new->opaque = NULL;

	return new;
}

void
cl_close(struct cl_peer *p)
{
}

void
cl_set_opaque(struct cl_peer *p, void *opaque)
{
	assert(p != NULL);

	p->opaque = opaque;
}

void *
cl_get_opaque(struct cl_peer *p)
{
	assert(p != NULL);

	return p->opaque;
}

const char *
cl_get_field(struct cl_peer *p, int id)
{
	return NULL;
}

int
cl_printf(struct cl_peer *p, const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = cl_vprintf(p, fmt, ap);
	va_end(ap);

	return n;
}

int
cl_vprintf(struct cl_peer *p, const char *fmt, va_list ap)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(fmt != NULL);

	return p->tree->vprintf(p, fmt, ap);
}

void
cl_set_mode(struct cl_peer *p, int mode)
{
	assert(p != NULL);

	p->mode = mode;
}

void
cl_again(struct cl_peer *p)
{
}

void
cl_help(struct cl_peer *p, int mode)
{
}

