/*
 * Copyright 2012-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <cl/tree.h>

#include "internal.h"

struct trie *
trie_add(struct trie **trie, const char *s, const struct cl_command *command)
{
	assert(trie != NULL);
	assert(s != NULL);
	assert(*s == '\0' || strcspn(s, "\t\v\f\r\n") != 0);
	assert(command != NULL);

	if (*trie == NULL) {
		size_t i;

		*trie = malloc(sizeof **trie);
		if (*trie == NULL) {
			return NULL;	
		}

		(*trie)->command = NULL;

		/* TODO: assert(SIZE_MAX > UCHAR_MAX); */

		for (i = 0; i < sizeof (*trie)->edge / sizeof *(*trie)->edge; i++) {
			(*trie)->edge[i] = NULL;
		}
	}

	if (*s == '\0') {
		/*
		 * A command may be already populated within the trie in the case of
		 * different usages given for the same path. If so, these are required
		 * to have the same callback and fields, and modes which do not overlap
		 * so that we may intersect our incoming command with what is present.
		 */
		if ((*trie)->command == NULL) {
			(*trie)->command = malloc(sizeof *(*trie)->command);
			if ((*trie)->command == NULL) {
				return NULL;
			}

			(*trie)->command->command  = command->command;
			(*trie)->command->modes    = 0;
			(*trie)->command->fields   = command->fields;
			(*trie)->command->callback = command->callback;
		}

		assert(0 == strcmp((*trie)->command->command, command->command)); /* by definition */
		assert(0 == ((*trie)->command->modes & command->modes));
		assert((*trie)->command->callback == command->callback);
		assert((*trie)->command->fields   == command->fields);

		(*trie)->command->modes |= command->modes;

		return *trie;
	}

	return trie_add(&(*trie)->edge[(unsigned char) *s], s + 1, command);
}

const struct trie *
trie_walk(const struct trie *trie, const char *s, size_t len)
{
	assert(trie != NULL);
	assert(s != NULL);
	assert(len > 0);

	do {
		trie = trie->edge[(unsigned char) *s];
		if (trie == NULL) {
			return NULL;
		}

		s++;
		len--;
	} while (len > 0);

	return trie;
}

static const struct trie *
trie_next(struct cl_peer *p, const struct trie *trie, int mode, char c,
	const struct trie **prev)
{
	size_t i;

	assert(trie != NULL);
	assert(p->tree->visible != NULL);
	assert(prev != NULL);

	if (trie->command != NULL && p->tree->visible(p, mode, trie->command->modes)) {
		assert(trie->command->command != NULL);

		if (*prev == NULL) {
			return trie;
		}

		if (*prev == trie) {
			*prev = NULL;
		}
	}

	/* TODO: assert(SIZE_MAX > UCHAR_MAX); */

	for (i = 0; i < sizeof trie->edge / sizeof *trie->edge; i++) {
		const struct trie *next;

		if (trie->edge[i] == NULL) {
			continue;
		}

		/* TODO: i don't like that this has knowledge of a specific char */
		if (i == c) {
			assert(trie->edge[i] != NULL);

			return trie->edge[i];
		}

		next = trie_next(p, trie->edge[i], mode, c, prev);
		if (next != NULL) {
			return next;
		}
	}

	return NULL;
}

const struct trie *
trie_cycle(struct cl_peer *p, const struct trie *trie, int mode, char c,
	const struct trie *prev)
{
	const struct trie *next;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->visible != NULL);
	assert(trie != NULL);

	next = trie_next(p, trie, mode, c, &prev);
	if (next != NULL) {
		return next;
	}

	return trie_next(p, trie, mode, c, &next);
}

const struct trie *
trie_run(struct cl_peer *p, const struct trie *trie, int mode, char c)
{
	assert(p != NULL);
	assert(trie != NULL);

	(void) c;

	trie = trie_cycle(p, trie, mode, ' ', NULL);
	if (trie == NULL) {
		return NULL;
	}

	if (trie->command != NULL && trie != trie_cycle(p, trie, mode, ' ', trie)) {
		return NULL;
	}

	return trie;
}

void
trie_help(struct cl_peer *p, const struct trie *trie, int mode)
{
	size_t i;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->commands != NULL);
	assert(trie != NULL);

	if (trie->command != NULL) {
		size_t i;

		for (i = 0; i < p->tree->command_count; i++) {
			assert(trie->command->command != NULL);
			assert(p->tree->commands[i].command != NULL);

			if (0 != strcmp(p->tree->commands[i].command, trie->command->command)) {
				continue;
			}

			if (!p->tree->visible(p, mode, p->tree->commands[i].modes)) {
				continue;
			}

			if (p->tree->commands[i].usage == NULL) {
				cl_printf(p, "  %-18s\n", p->tree->commands[i].command);
			} else {
				cl_printf(p, "  %-18s - %s\n", p->tree->commands[i].command,
					p->tree->commands[i].usage);
			}
		}
	}

	/* TODO: assert(SIZE_MAX > UCHAR_MAX); */

	for (i = 0; i < sizeof trie->edge / sizeof *trie->edge; i++) {
		if (trie->edge[i] == NULL) {
			continue;
		}

		trie_help(p, trie->edge[i], mode);
	}
}

