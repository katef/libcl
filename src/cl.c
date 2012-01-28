/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "internal.h"

extern struct io io_start;
extern struct io io_ecma48;
extern struct io io_telnet;
extern struct io io_end;

static const struct cl_chain {
	enum cl_io io;
	size_t n;
	const struct io *ioapi[4];
} io_chains[] = {
	{ CL_PLAIN,  2, { &io_start,                         &io_end } },
	{ CL_ECMA48, 3, { &io_start, &io_ecma48,             &io_end } },
	{ CL_TELNET, 4, { &io_start, &io_ecma48, &io_telnet, &io_end } }
#if 0
	{ CL_SSL,    4, { &io_start, &io_ecma48, &io_ssl,    &io_end } },
	{ CL_SSH,    4, { &io_start, &io_ecma48, &io_ssh,    &io_end } }
#endif
};

static const struct cl_chain *
findchain(enum cl_io io)
{
	size_t i;

	for (i = 0; i < sizeof io_chains / sizeof *io_chains; i++) {
		if (io_chains[i].io == io) {
			return &io_chains[i];
		}
	}

	return NULL;
}

struct cl_tree *
cl_create(size_t command_count, const struct cl_command commands[],
	size_t field_count, const struct cl_field fields[],
	int (*printprompt)(struct cl_peer *p, int mode),
	int (*visible)(struct cl_peer *p, int mode, int modes),
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

		/* things to assert about .command: does not start with space;
		 * all alnum or ' '; all isprint; no non-' ' whitespace; is not empty */
		node = trie_add(&new->root, commands[i].command, &commands[i]);
		if (node == NULL) {
			cl_destroy(new);
			return NULL;
		}
	}

	return new;
}

void
cl_destroy(struct cl_tree *t)
{
}

struct cl_peer *
cl_accept(struct cl_tree *t, int fd, enum cl_io io)
{
	struct cl_peer *new;
	size_t i;

	assert(t != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->chain = findchain(io);
	if (new->chain == NULL) {
		free(new);
		return NULL;
	}

	new->chctx = malloc(sizeof *new->chctx * new->chain->n);
	if (new->chctx == NULL) {
		free(new);
		return NULL;
	}

	new->rctx = read_create();
	if (new->rctx == NULL) {
		free(new->chctx);
		free(new);
		return NULL;
	}

	new->tree   = t;
	new->mode   = 0;
	new->opaque = NULL;

	/* TODO: get terminal name from I/O layer... telnet, getenv, etc */
	new->tctx = term_create(&new->term, "xterm");
	if (new->tctx == NULL) {
		free(new->chctx);
		read_destroy(new->rctx);
		free(new);
		return NULL;
	}

	for (i = 0; i < new->chain->n; i++) {
		new->chctx[i].ioapi = new->chain->ioapi[i];
		new->chctx[i].ioctx = NULL;
	}

	for (i = 0; i < new->chain->n; i++) {
		if (new->chain->ioapi[i]->create == NULL) {
			continue;
		}

		new->chctx[i].ioctx = new->chain->ioapi[i]->create(fd);
		if (new->chctx[i].ioctx == NULL) {
			cl_close(new);
			return NULL;
		}
	}

	if (-1 == new->tree->printprompt(new, new->mode)) {
		cl_close(new);
		return NULL;
	}

	return new;
}

void
cl_close(struct cl_peer *p)
{
	size_t i;

	assert(p != NULL);
	assert(p->tctx != NULL);
	assert(p->rctx != NULL);
	assert(p->chain != NULL);

	term_destroy(p->tctx);
	read_destroy(p->rctx);

	for (i = 0; i < p->chain->n; i++) {
		if (p->chain->ioapi[i]->destroy != NULL) {
			p->chain->ioapi[i]->destroy(p->chctx[i].ioctx);
		}
	}

	free(p->chctx);
	free(p);
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
	assert(p != NULL);

	return read_get_field(p->rctx, id);
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
	assert(fmt != NULL);

	return p->chctx->ioapi->vprintf(p, &p->chctx[0], fmt, ap);
}

ssize_t
cl_read(struct cl_peer *p, const void *data, size_t len)
{
	struct cl_chctx *tail;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->chctx != NULL);
	assert(data != NULL);
	assert(len <= SSIZE_MAX);

	if (len == 0) {
		return 0;
	}

	tail = &p->chctx[p->chain->n - 1];

	assert(tail->ioapi != NULL);

	return tail->ioapi->read(p, tail, data, len);
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
	size_t i;

	assert(p != NULL);
	assert(p->tree != NULL);

	for (i = 0; i < p->tree->command_count; i++) {
		if (!p->tree->visible(p, mode, p->tree->commands[i].modes)) {
			continue;
		}

		if (p->tree->commands[i].usage == NULL) {
			cl_printf(p, "  %-20s\n", p->tree->commands[i].command);
		} else {
			cl_printf(p, "  %-20s - %s\n", p->tree->commands[i].command, p->tree->commands[i].usage);
		}
	}
}

/* You don't have to use this; if you provide your own then your modes needn't
 * be a bitmap. You could always return true, or perhaps have some
 * application-specific special cases.
 */
int
cl_visible(struct cl_peer *p, int mode, int modes)
{
	assert(p != NULL);

	if (mode == 0) {
		return modes == 0;
	}

	return modes & mode;
}

