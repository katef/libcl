#include <assert.h>
#include <stddef.h>

#include <cl/tree.h>

#include "internal.h"

const struct cl_field *
find_field(struct cl_tree *t, int id)
{
	size_t i;

	assert(t != NULL);

	for (i = 0; i < t->field_count; i++) {
		if (t->fields[i].id == id) {
			return &t->fields[i];
		}
	}

	return NULL;
}

