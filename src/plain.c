/* $Id$ */

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>

#include "internal.h"

int
getc_plain(struct cl_peer *p, char c)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);

	return getc_main(p, c);
}

