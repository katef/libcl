/*
 * Copyright 2012-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

#include "../internal.h"
#include "chain.c"

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

const struct io io_end = {
	chain_create,
	chain_destroy,
	chain_read,
	end_send,
	end_vprintf,
	chain_printf,
	end_ttype
};

