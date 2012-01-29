/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "internal.h"

static int
start_create(struct cl_peer *p, struct cl_chctx chctx[])
{
	assert(p != NULL);
	assert(p->ttype != NULL);
	assert(strlen(p->ttype) > 0);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->create == start_create);

	(void) chctx;

	if (p->ttype == NULL) {
		/* TODO: explain that this happens e.g. without telnet or ecma48 (i.e. just plain...) */
		p->ttype = chctx->ioapi->ttype(p, chctx);
	}

	assert(p->ttype != NULL);

	p->tctx = term_create(&p->term, p->ttype);
	if (p->tctx == NULL) {
		return -1;
	}

	/* TODO: now we have tctx, cl_printf is permitted henceforth */

/* TODO:
    motd is the first place the user is allowed to write (by cl_printf or equivalent) to the peer
    if we're here, then either:
    - there is no incoming data we had to wait for (in the case of a tty)
    - or (in the case of telnet), we waited for $TERM, and the application cl_read() it to us
*/

	if (p->tree->motd != NULL) {
		if (-1 == p->tree->motd(p)) {
			return -1;
		}
	}

	if (-1 == p->tree->printprompt(p, p->mode)) {
		return -1;
	}

	return 0;
}

static void
start_destroy(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->destroy == start_destroy);

	if (p->tctx != NULL) {
		term_destroy(p->tctx);
	}

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->destroy != NULL);

	next->ioapi->destroy(p, next);
}

static ssize_t
start_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	size_t i;

	assert(p != NULL);
	assert(data != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
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
	assert(chctx->ioapi != NULL);
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
	assert(chctx->ioapi != NULL);
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
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->printf == start_printf);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = start_vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

static const char *
start_ttype(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);

	next = chctx + 1;

	assert(next->ioapi != NULL);
	assert(next->ioapi->ttype != NULL);

	return next->ioapi->ttype(p, next);
}

struct io io_start = {
	start_create,
	start_destroy,
	start_read,
	start_send,
	start_vprintf,
	start_printf,
	start_ttype
};

