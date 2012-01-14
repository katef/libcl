/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>	/* XXX */

#include "internal.h"

/* You don't have to use this; if you provide your own then your modes needn't
 * be a bitmap. You could always return true, or perhaps have some
 * application-specific special cases.
 */
int
cl_visible(struct cl_peer *p, int mode, const struct cl_command *command)
{
	assert(p != NULL);
	assert(command != NULL);

	if (mode == 0) {
		return p->t->command->modes == 0;
	}

	return p->t->command->modes & p->mode;
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

	printf("field %s: \n", f->name);
}

ssize_t
cl_read(struct cl_peer *p, const void *data, size_t len)
{
	const char *s;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);
	assert(data != NULL);

	if (len == 0) {
		return 0;
	}

	/* TODO: keep p->t and the p->state enum private to this function somehow.
	maybe a readstate struct */

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
					printf("command not found\n");

					p->state = STATE_NEW;
					continue;
				}

				if (!p->tree->visible(p, p->mode, p->t->command)) {
					printf("command not found for this mode\n");

					p->state = STATE_NEW;
					continue;
				}

				printf("command acquired: %s\n", p->t->command->command);

				p->fields = p->t->command->fields;

				p->state = STATE_FIELD;

				goto nextfield;

			default:
				p->t = p->t->edge[(unsigned char) *s];
				if (p->t == NULL) {
					printf("command not found for '%c'\n", *s);
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
				/* TODO: store argv, argc here */

				p->fields &= p->fields - 1;

nextfield:

				if (p->fields == 0) {
					printf("executing callback\n");

					p->t->command->callback(p, p->t->command->command, p->mode, 0, NULL);

					p->state = STATE_NEW;
					continue;
				}

				fieldprompt(p);

				continue;

			default:
				/* TODO: push character somewhere for field ID p->fields & ~(p->fields - 1) */

				continue;
			}
		}
	}

	return s - (const char *) data;
}

