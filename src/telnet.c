/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>

#include "internal.h"

static ssize_t
telnet_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	assert(p != NULL);
	assert(chctx != NULL);
	assert(data != NULL);

	/* TODO: ... */

	return (chctx - 1)->ioapi->read(p, chctx - 1, data, len);
}

struct io io_telnet = {
	NULL,
	NULL,
	telnet_read,
	NULL,
	NULL,
	NULL
};

