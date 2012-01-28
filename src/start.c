/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

#include "internal.h"

static ssize_t
start_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	size_t i;

	assert(p != NULL);
	assert(data != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi->read == start_read);

	(void) chctx;

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

static ssize_t
start_send(struct cl_peer *p, struct cl_chctx chctx[],
	enum ui_output output)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi->send == start_send);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->send != NULL);

	return next->ioapi->send(p, next, output);
}

static int
start_vprintf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, va_list ap)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi->vprintf == start_vprintf);
	assert(fmt != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->vprintf != NULL);

	return next->ioapi->vprintf(p, next, fmt, ap);
}

static int
start_printf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi->printf == start_printf);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = start_vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

struct io io_start = {
	NULL,
	NULL,
	start_read,
	start_send,
	start_vprintf,
	start_printf
};

