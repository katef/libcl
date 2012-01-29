/* $Id$ */

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "internal.h"

enum readstate {
	STATE_NEW,
	STATE_CHAR,
	STATE_FIELD
};

struct value {
	int id;
	char *value;

	struct value *next;
};

struct readctx {
	enum readstate state;
	const struct trie *t;
	int fields;
	struct value *values;
	size_t count;
};

static void *
append(void *base, size_t basesz, size_t *count, const char *s)
{
	const size_t blocksz = 32;
	size_t n;
	size_t prev;

	assert(base != NULL);
	assert(basesz > 0);
	assert(count != NULL);
	assert(s != NULL);

	n = strlen(s);

	assert(n != 0);
	assert(n < blocksz);

	if (*count > INT_MAX - n) {
		errno = ENOMEM;
		return NULL;
	}

	prev = *count;

	/* we're assigning up here because realloc below may move count */
	*count += n;

	if ((*count - 1) % blocksz < n) {
		base = realloc(base, basesz + prev + blocksz);
		if (base == NULL) {
			return NULL;
		}
	}

	memcpy((char *) base + basesz + prev, s, n);

	return base;
}

static void
fieldprompt(struct cl_peer *p)
{
	const struct cl_field *f;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->rctx->fields != 0);

	f = find_field(p->tree, p->rctx->fields & ~(p->rctx->fields - 1));

	assert(f != NULL);

	cl_printf(p, "%s: ", f->name);
}

struct readctx *
read_create(void)
{
	struct readctx *new;

	new = malloc(sizeof *new + 1);
	if (new == NULL) {
		return NULL;
	}

	new->state = STATE_NEW;

	return new;
}

void
read_destroy(struct readctx *rctx)
{
	assert(rctx != NULL);

	free(rctx);
}

const char *
read_get_field(struct readctx *rctx, int id)
{
	const struct value *v;

	assert(rctx != NULL);

	for (v = rctx->values; v != NULL; v = v->next) {
		if (v->id == id) {
			return v->value;
		}
	}

	return NULL;
}

int
getc_main(struct cl_peer *p, struct cl_event *event)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);
	assert(p->rctx != NULL);
	assert(event != NULL);

	if (event->type == UI_CODEPOINT && event->u.utf8[0] == '\r') {
		return 0;
	}

	switch (p->rctx->state) {
	case STATE_NEW:
		p->rctx->state  = STATE_CHAR;
		p->rctx->fields = 0;
		p->rctx->values = NULL;
		p->rctx->count  = 0;

		/* FALLTHROUGH */

	case STATE_CHAR:
		switch (event->type) {
		case UI_CODEPOINT:
			break;

		case UI_BACKSPACE:
			/* XXX: this should walk back one unicode character's worth, not just one byte */

			if (p->rctx->count == 0) {
				return 0;
			}

			if (-1 == p->chctx->ioapi->send(p, p->chctx + 0, OUT_BACKSPACE_AND_DELETE)) {
				return -1;
			}

			p->rctx->count--;

			return 0;

		case UI_DELETE:
		case UI_DELETE_TO_EOL:
		case UI_DELETE_WORD:
		case UI_CANCEL:	/* ^C to abort current command */
			/* TODO: not implemented */
			return 0;

		case UI_HIST_PREV:
		case UI_HIST_NEXT:
			/* TODO: not implemented */
			return 0;

		case UI_CURSOR_LEFT:
		case UI_CURSOR_RIGHT:
		case UI_CURSOR_LEFT_WORD:
		case UI_CURSOR_RIGHT_WORD:
		case UI_CURSOR_EOL:
		case UI_CURSOR_SOL:
			/* TODO: not implemented */
			return 0;
		}

		switch (event->u.utf8[0]) {
		case '?':
			/* TODO: clear current (prompt) line instead? */
			cl_printf(p, "?\n");

			/* TODO: not implemented to show context-sensitive help for command arguments */
			cl_help(p, p->mode);

			p->tree->printprompt(p, p->mode);

			assert(p->rctx->count <= INT_MAX);

			cl_printf(p, "%.*s", (int) p->rctx->count, (const char *) p->rctx + sizeof *p->rctx);

			p->rctx->state = STATE_CHAR;
			return 0;

		case '\t':
			/* TODO: not implemented */
			return 0;

		case '\n':
			cl_printf(p, "\n");

			{
				unsigned char *u;

				u = (unsigned char *) p->rctx + sizeof *p->rctx;

				u[p->rctx->count] = '\0';

				for (p->rctx->t = p->tree->root; *u != '\0'; u++) {
					/* TODO: skip excess whitespace here */

					p->rctx->t = p->rctx->t->edge[*u];
					if (p->rctx->t == NULL) {
						cl_printf(p, "command not found\n");

						p->tree->printprompt(p, p->mode);

						p->rctx->state = STATE_NEW;
						return 0;
					}
				}
			}

			if (p->rctx->t == p->tree->root) {
				p->tree->printprompt(p, p->mode);

				return 0;
			}

			if (p->rctx->t->command == NULL) {
				cl_printf(p, "command not found\n");

				p->rctx->state = STATE_NEW;
				return 0;
			}

			if (!p->tree->visible(p, p->mode, p->rctx->t->command->modes)) {
				cl_printf(p, "command not found\n");

				p->tree->printprompt(p, p->mode);

				p->rctx->state = STATE_NEW;
				return 0;
			}

			/* TODO: store argv, argc here */

			p->rctx->fields = p->rctx->t->command->fields;
			p->rctx->state  = STATE_FIELD;

			goto firstfield;

		default:
			/* TODO: maybe not for IO_PLAIN */
			cl_printf(p, "%s", event->u.utf8);

			{
				struct readctx *tmp;

				tmp = append(p->rctx, sizeof *p->rctx,
					&p->rctx->count, event->u.utf8);
				if (tmp == NULL) {
					return -1;
				}

				p->rctx = tmp;
			}

			return 0;
		}

	/* TODO: I'm not sure we even need fields. Can they be viewed as a
	 * different kind of prompt, but without word-tokenised whitespace input?
	 * i.e. without the trie? so we'd have STATE_FIELD instead of STATE_CHAR,
	 * and ignore the trie. but that's essentially what we're doing here. */
	case STATE_FIELD:
		switch (event->type) {
		case UI_HIST_PREV:
		case UI_HIST_NEXT:
			/* no effect when entering fields */
			return 0;

		case UI_CODEPOINT:
			break;

		case UI_CANCEL:	/* ^C to abort current command */
			/* TODO: not implemented; abandon all fields and go back to STATE_NEW */
			return 0;

		case UI_BACKSPACE:
		case UI_DELETE:
		case UI_DELETE_TO_EOL:
		case UI_DELETE_WORD:
			/* TODO: not implemented */
			return 0;

		case UI_CURSOR_LEFT:
		case UI_CURSOR_RIGHT:
		case UI_CURSOR_LEFT_WORD:
		case UI_CURSOR_RIGHT_WORD:
		case UI_CURSOR_EOL:
		case UI_CURSOR_SOL:
			/* TODO: no effect when enterining non-echoing fields */
			/* TODO: not implemented */
			return 0;
		}

		switch (event->u.utf8[0]) {
		case '\n':
			cl_printf(p, "\n");

			p->rctx->values->value = (char *) p->rctx->values + sizeof *p->rctx->values;
			p->rctx->values->value[p->rctx->count] = '\0';

			p->rctx->fields &= p->rctx->fields - 1;

firstfield:

			if (p->rctx->fields == 0) {
				p->rctx->t->command->callback(p, p->rctx->t->command->command, p->mode, 0, NULL);

				/* TODO: free .values list */

				p->tree->printprompt(p, p->mode);

				p->rctx->state = STATE_NEW;
				return 0;
			}

			{
				struct value *new;

				new = malloc(sizeof *new + 1);
				if (new == NULL) {
					/* TODO: free .values list */
					return -1;
				}

				new->id    = p->rctx->fields & ~(p->rctx->fields - 1);
				new->value = (char *) new + sizeof *new;
				new->next  = p->rctx->values;

				p->rctx->values = new;
				p->rctx->count  = 0;
			}

			fieldprompt(p);

			/* TODO: turn off echo if appropriate */

			return 0;

		default:
			/* TODO: only if echoing for this field */
			cl_printf(p, "%s", event->u.utf8);

			{
				struct value *tmp;

				tmp = append(p->rctx->values, sizeof *p->rctx->values,
					&p->rctx->count, event->u.utf8);
				if (tmp == NULL) {
					/* TODO: free .values list */
					return -1;
				}

				p->rctx->values = tmp;
			}

			return 0;
		}
	}

	/* UNREACHED */

	return -1;
}

