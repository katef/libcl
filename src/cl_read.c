/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "internal.h"

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

static void
fieldprompt(struct cl_peer *p)
{
	const struct cl_field *f;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->fields != 0);

	f = find_field(p->tree, p->fields & ~(p->fields - 1));

	assert(f != NULL);

	cl_printf(p, "%s: ", f->name);
}

ssize_t
cl_read(struct cl_peer *p, const void *data, size_t len)
{
	const char *s;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);
	assert(data != NULL);

	/* TODO: keep p->t and the p->state enum private to this function somehow.
	maybe a readstate struct */

	/* XXX: when len == SIZE_MAX */

	for (s = data; s - (const char *) data < len; s++) {
		/* TODO: FSM; CSI lexing & telnet */

		if (*s == '\r') {
			continue;
		}

		switch (p->state) {
		case STATE_NEW:
			p->t      = p->tree->root;
			p->state  = STATE_CHAR;
			p->fields = 0;
			p->values = NULL;

			p->tree->printprompt(p, p->mode);

			if (*s == '\n') {
				continue;
			}

			p->state = STATE_CHAR;

			/* FALLTHROUGH */

		case STATE_CHAR:
			switch (*s) {
			case '\n':
				if (p->t->command == NULL) {
					cl_printf(p, "command not found\n");

					p->state = STATE_NEW;
					continue;
				}

				if (!p->tree->visible(p, p->mode, p->t->command->modes)) {
					cl_printf(p, "command not found\n");

					p->state = STATE_NEW;
					continue;
				}

				/* TODO: store argv, argc here */

				p->fields = p->t->command->fields;

				p->state = STATE_FIELD;

				goto firstfield;

			default:
				p->t = p->t->edge[(unsigned char) *s];
				if (p->t == NULL) {
					cl_printf(p, "command not found\n");
					return s - (const char *) data;
				}

				continue;
			}

		/* TODO: I'm not sure we even need fields. Can they be viewed as a
		 * different kind of prompt, but without word-tokenised whitespace input?
		 * i.e. without the trie? so we'd have STATE_FIELD instead of STATE_CHAR,
		 * and ignore the trie. but that's essentially what we're doing here. */
		case STATE_FIELD:
			switch (*s) {
			case '\n':
				p->values->value[p->count] = '\0';

				p->fields &= p->fields - 1;

firstfield:

				if (p->fields == 0) {
					p->t->command->callback(p, p->t->command->command, p->mode, 0, NULL);

					/* TODO: free .values list */

					p->state = STATE_NEW;
					continue;
				}

				{
					struct value *new;

					new = malloc(sizeof *new + 1);
					if (new == NULL) {
						/* TODO: free .values list */
						return -1;
					}

					new->id    = p->fields & ~(p->fields - 1);
					new->value = (char *) new + sizeof *new;
					new->next  = p->values;

					p->values = new;
					p->count  = 0;
				}

				fieldprompt(p);

				/* TODO: turn off echo if appropriate */

				continue;

			default:
				if (p->count % 32 == 0) {
					struct value *tmp;

					tmp = realloc(p->values, sizeof *p->values + p->count + 32);
					if (tmp == NULL) {
						/* TODO: free .values list */
						return -1;
					}

					tmp->value = (char *) tmp + sizeof *tmp;

					p->values = tmp;
				}

				p->values->value[p->count++] = *s;

				continue;
			}
		}
	}

	return s - (const char *) data;
}

