/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <libtelnet.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "internal.h"

struct ioctx {
	struct cl_peer *p;
	struct cl_chctx *chctx;
	telnet_t *tt;
};

static void
ready(struct cl_chctx *chctx)
{
	struct cl_chctx *prev;

	assert(chctx != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioctx->p != NULL);
	assert(chctx->ioctx->p->ttype != NULL);

	prev = chctx - 1;

	assert(prev != NULL);
	assert(prev->ioapi != NULL);
	assert(prev->ioapi->create != NULL);

	/* TODO: handle error */
	prev->ioapi->create(chctx->ioctx->p, prev);
}

static void
handler(telnet_t *tt, telnet_event_t *event, void *opaque)
{
	struct cl_chctx *chctx;

	assert(tt != NULL);
	assert(event != NULL);
	assert(opaque != NULL);

	chctx = opaque;

	assert(chctx != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioctx->p != NULL);
	assert(chctx->ioctx->tt == tt);

	switch (event->type) {
	case TELNET_EV_TTYPE:
		assert(event->ttype.name != NULL);

		if (event->ttype.name == NULL || strlen(event->ttype.name) == 0) {
			break;
		}

		if (chctx->ioctx->p->ttype != NULL) {
			/* TODO: handle terminal types changing */
			break;
		}

		/* TODO: find out if this $TERM is supported; if not, continue to the next */

		{
			char *s;

			/* XXX: strdup? */
			/* XXX: allocate after struct for free() convenience */
			s = strdup(event->ttype.name);

			for (chctx->ioctx->p->ttype = s; *s != '\0'; s++) {
				*s = tolower((unsigned char) *s);
			}

			/* TODO: now init ecma48 */
			/* TODO: handle error */
			ready(chctx);
		}

		break;

	case TELNET_EV_DATA:

		/*
		 * Here we have incoming data before a response for our TTYPE query.
		 * RFC 854 suggests we buffer this (and presumbaly refrain from sending
		 * data, too) in order to 'hide the "uncertainty period" from the user.'
		 *
		 * TODO: consider buffering data. For now we're just falling through to
		 * pick a terminal type supplied from the next I/O handler.
		 */
		if (chctx->ioctx->p->ttype == NULL) {
			struct cl_chctx *next;

			next = chctx + 1;

			assert(next->ioapi != NULL);
			assert(next->ioapi->ttype != NULL);

			chctx->ioctx->p->ttype = next->ioapi->ttype(chctx->ioctx->p, next);

			assert(chctx->ioctx->p->ttype != NULL);

			/* TODO: now init ecma48 */
			/* TODO: handle error */
			ready(chctx);
		}

		{
			struct cl_chctx *prev;

			prev = chctx - 1;

			assert(prev->ioapi != NULL);
			assert(prev->ioapi->read != NULL);

			/* TODO: handle error */
			(void) prev->ioapi->read(chctx->ioctx->p, prev,
				event->data.buffer, event->data.size);
		}
		return;

	case TELNET_EV_SEND:
/* TODO: this can occur before ecma48 is initialised, but that's okay. it only occurs after cl_ready(),
so the user's callbacks will operate to write to the wire */
		{
			struct cl_chctx *next;

			next = chctx + 1;

			assert(next->ioapi != NULL);
			assert(next->ioapi->printf != NULL);

			/* TODO: really want a .write() here, for binary */
			/* TODO: handle error */
			/* XXX: cast to int for printf */
			(void) next->ioapi->printf(chctx->ioctx->p, next, "%.*s",
				(int) event->data.size, event->data.buffer);
		}
		return;

	case TELNET_EV_ENVIRON:
	case TELNET_EV_IAC:
	case TELNET_EV_WILL:
	case TELNET_EV_WONT:
	case TELNET_EV_DO:
	case TELNET_EV_DONT:
	case TELNET_EV_SUBNEGOTIATION:
	case TELNET_EV_COMPRESS:
	case TELNET_EV_ZMP:
	case TELNET_EV_MSSP:
	case TELNET_EV_WARNING:
	case TELNET_EV_ERROR:
		/* TODO: ... */
		break;
	}
}

static int
cltelnet_create(struct cl_peer *p, struct cl_chctx chctx[])
{
	size_t i;

	static const telnet_telopt_t opts[] = {
		{ TELNET_TELOPT_ECHO,  TELNET_WILL, TELNET_DO },
		{ TELNET_TELOPT_SGA,   TELNET_WILL, TELNET_DO },
		{ TELNET_TELOPT_TTYPE, TELNET_WILL, TELNET_DO },
		{ -1, 0, 0 }
	};

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx == NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->create == cltelnet_create);

	chctx->ioctx = malloc(sizeof *chctx->ioctx);
	if (chctx->ioctx == NULL) {
		return -1;
	}

	chctx->ioctx->p = p;

	chctx->ioctx->tt = telnet_init(opts, handler, 0, chctx);
	if (chctx->ioctx->tt == NULL) {
		free(chctx->ioctx);
		chctx->ioctx = NULL;
		return -1;
	}

	/* XXX: I don't understand why I'm having to do this here, as well as in the table above */
	for (i = 0; i < sizeof opts / sizeof *opts; i++) {
		telnet_negotiate(chctx->ioctx->tt, opts[i].us,  opts[i].telopt);
		telnet_negotiate(chctx->ioctx->tt, opts[i].him, opts[i].telopt);
	}

/* XXX:
telnet_begin_newenviron(ioctx->tt, TELNET_ENVIRON_SEND);
telnet_newenviron_value(ioctx->tt, 
*/

	telnet_ttype_send(chctx->ioctx->tt);

	return 0;
}


static void
cltelnet_destroy(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->destroy == cltelnet_destroy);

	if (chctx->ioctx != NULL) {
		assert(chctx->ioctx->tt != NULL);

		telnet_free(chctx->ioctx->tt);

		free(chctx->ioctx);
	}

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->destroy != NULL);

	next->ioapi->destroy(p, next);
}

static ssize_t
cltelnet_read(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	struct cl_chctx *prev;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioctx->p != NULL);
	assert(chctx->ioctx->tt != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->read == cltelnet_read);
	assert(data != NULL);

	prev = chctx - 1;

	telnet_recv(chctx->ioctx->tt, data, len);

	/* TODO: really all of len consumed? */
	return len;
}

static ssize_t
cltelnet_send(struct cl_peer *p, struct cl_chctx chctx[],
	enum ui_output output)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx != NULL);

	next = chctx + 1;

	(void) next;

	switch (output) {
	case OUT_BACKSPACE_AND_DELETE:
	case OUT_SAVE:
	case OUT_RESTORE_AND_DELETE_TO_EOL:
		/* TODO: nothing sensible we can do here? ecma48 should have done this for us */
		break;
	}

	/* TODO: currently this should have never been reached.
	TODO: though... this function is relevant for e.g. sending
	a "please turn off local echo" UI_ thingy */

	return 0;
}

static int
cltelnet_vprintf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, va_list ap)
{
	struct cl_chctx *next;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioctx->p != NULL);
	assert(chctx->ioctx->tt != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->vprintf == cltelnet_vprintf);
	assert(fmt != NULL);

	next = chctx + 1;

	(void) next;

	return telnet_vprintf(chctx->ioctx->tt, fmt, ap);
}

static int
cltelnet_printf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioapi != NULL);
	assert(chctx->ioapi->printf == cltelnet_printf);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = cltelnet_vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

static const char *
cltelnet_ttype(struct cl_peer *p, struct cl_chctx chctx[])
{
	struct cl_chctx *next;

	assert(p != NULL);

	next = chctx + 1;

	assert(next->ioapi != NULL);
	assert(next->ioapi->ttype != NULL);

	return next->ioapi->ttype(p, next);
}

struct io io_telnet = {
	cltelnet_create,
	cltelnet_destroy,
	cltelnet_read,
	cltelnet_send,
	cltelnet_vprintf,
	cltelnet_printf,
	cltelnet_ttype
};

