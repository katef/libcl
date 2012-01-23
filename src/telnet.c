/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>

#include "internal.h"

static ssize_t
telnet_read(struct cl_peer *p, struct ioctx *ioctx, const void *data, size_t len)
{
	assert(p != NULL);
	assert(ioctx != NULL);
	assert(data != NULL);

	/* TODO: ... */

	return io_ecma48.read(p, ioctx, data, len);
}

struct io io_telnet = {
	NULL,
	NULL,
	telnet_read,
	NULL
};

