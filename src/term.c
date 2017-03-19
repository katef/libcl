#include <cl/tree.h>

#include <unibilium.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

struct termctx {
	unibi_term *ut;
};

struct termctx *
term_create(struct cl_term *term, const char *name)
{
	struct termctx *new;
	size_t i;

	struct {
		int required;
		enum unibi_string id;
		size_t offset;
	} a[] = {
		{ 1, unibi_cursor_left,      offsetof(struct cl_term, cub1) },
		{ 0, unibi_delete_character, offsetof(struct cl_term, dch1) },
		{ 0, unibi_clr_eol,          offsetof(struct cl_term,   el) },
		{ 0, unibi_save_cursor,      offsetof(struct cl_term,   sc) },
		{ 0, unibi_restore_cursor,   offsetof(struct cl_term,   rc) }
	};

	assert(term != NULL);
	assert(name != NULL);
	assert(strlen(name) > 0);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->ut = unibi_from_term(name);
	if (new->ut == NULL) {
		free(new);
		return NULL;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		const char **p = (const char **) ((char *) term + a[i].offset);

		*p = unibi_get_str(new->ut, a[i].id);

		if (a[i].required && *p == NULL) {
			unibi_destroy(new->ut);
			free(new);
			return NULL;
		}
	}

	return new;
}

void
term_destroy(struct termctx *tctx)
{
	assert(tctx != NULL);
	assert(tctx->ut != NULL);

	unibi_destroy(tctx->ut);

	free(tctx);
}

