/* $Id$ */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

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

		for (i = 0; i < sizeof (*trie)->edge / sizeof *(*trie)->edge; i++) {
			(*trie)->edge[i] = NULL;
		}
	}

	if (*s == '\0') {
		assert((*trie)->command == NULL);

		(*trie)->command = command;

		return *trie;
	}

	return trie_add(&(*trie)->edge[(unsigned char) *s], s + 1, command);
}

