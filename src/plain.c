/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>

#include "internal.h"

static ssize_t
read_plain(struct cl_peer *p, struct ioctx *ioctx, const void *data, size_t len)
{
	size_t i;

	assert(p != NULL);
	assert(p->ioctx != NULL);
	assert(data != NULL);

	for (i = 0; i < len; i++) {
		struct cl_event e;
		char s[2];

		s[0] = * ((char *) data + i);
		s[1] = '\0';

		e.type   = UI_CODEPOINT;
		e.u.utf8 = s;

		if (-1 == getc_main(p, &e)) {
			return -1;
		}
	}

	return i;
}

struct io io_plain = {
	NULL,
	NULL,
	read_plain
};

