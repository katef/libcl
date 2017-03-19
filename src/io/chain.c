#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

#include "../internal.h"

static int
chain_create(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *prev;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);

	prev = chctx - 1;

	assert(prev != NULL);
	assert(prev->ioapi != NULL);
	assert(prev->ioapi->create != NULL);

	return prev->ioapi->create(p, prev);
}

static void
chain_destroy(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->destroy != NULL);

	next->ioapi->destroy(p, next);
}

static ssize_t
chain_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	struct cl_chctx *prev;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(data != NULL);
	assert(len > 0);

	prev = chctx - 1;

	assert(prev != NULL);
	assert(prev->ioapi != NULL);
	assert(prev->ioapi->read != NULL);

	return prev->ioapi->read(p, prev, data, len);
}

static ssize_t
chain_send(struct cl_peer *p, struct cl_chctx chctx[],
	enum ui_output output)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->send != NULL);

	return next->ioapi->send(p, next, output);
}

static int
chain_vprintf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, va_list ap)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(fmt != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->vprintf != NULL);

	return next->ioapi->vprintf(p, next, fmt, ap);
}

static int
chain_printf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->vprintf != NULL);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = chctx->ioapi->vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

static const char *
chain_ttype(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);

	next = chctx + 1;

	assert(next->ioapi != NULL);
	assert(next->ioapi->ttype != NULL);

	return next->ioapi->ttype(p, next);
}

static const struct io io_chain = {
	chain_create,
	chain_destroy,
	chain_read,
	chain_send,
	chain_vprintf,
	chain_printf,
	chain_ttype
};

