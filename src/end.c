/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

#include "internal.h"

static int
end_create(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *prev;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->create == end_create);

	prev = chctx - 1;

	assert(prev != NULL);
	assert(prev->ioapi != NULL);
	assert(prev->ioapi->create != NULL);

	return prev->ioapi->create(p, prev);
}

static void
end_destroy(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->destroy == end_destroy);

	next = chctx + 1;

	(void) p;
	(void) next;
}

static ssize_t
end_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	struct cl_chctx *prev;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->read == end_read);
	assert(data != NULL);
	assert(len > 0);

	prev = chctx - 1;

	assert(prev != NULL);
	assert(prev->ioapi != NULL);
	assert(prev->ioapi->read != NULL);

	return prev->ioapi->read(p, prev, data, len);
}

static ssize_t
end_send(struct cl_peer *p, struct cl_chctx chctx[],
	enum ui_output output)
{
	ssize_t n = 0;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->send == end_send);

	(void) chctx;

	switch (output) {
	case OUT_BACKSPACE_AND_DELETE:
		/* TODO: disregard for plain? */
		n = chctx->ioapi->printf(p, chctx, "\b \b");
		break;

	case OUT_SAVE:
		/* TODO: disregard for plain? */
		/* TODO: no, no reason why we can't fake it using backspace, as ecma48 does */
		break;

	case OUT_RESTORE_AND_DELETE_TO_EOL:
		n = chctx->ioapi->printf(p, chctx, "\r\n");
		break;
	}

	return n;
}

static int
end_vprintf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, va_list ap)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->vprintf == end_vprintf);
	assert(fmt != NULL);

	(void) chctx;

	return p->tree->vprintf(p, fmt, ap);
}

static int
end_printf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->printf == end_printf);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = end_vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

static const char *
end_ttype(struct cl_peer *p, struct cl_chctx chctx[])
{
	const char *ttype;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->ttype == end_ttype);

	(void) chctx;

	if (p->tree->ttype == NULL) {
		return "unknown";
	}

	ttype = p->tree->ttype(p);
	if (ttype == NULL) {
		return "unknown";
	}

	return ttype;
}

struct io io_end = {
	end_create,
	end_destroy,
	end_read,
	end_send,
	end_vprintf,
	end_printf,
	end_ttype
};

