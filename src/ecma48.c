/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <termkey.h>

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>

#include "internal.h"

struct ioctx {
	TermKey *tk;
	int save;
};

static struct ioctx *
ecma48_create(int fd)
{
	struct ioctx *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	/* TODO: TERMKEY_FLAG_UTF8? CTRLC? */
	new->tk = termkey_new(fd, 0);
	if (new->tk == NULL) {
		free(new);
		return NULL;
	}

	new->save = -1;

	return new;
}

static void
ecma48_destroy(struct ioctx *ioctx)
{
	assert(ioctx != NULL);
	assert(ioctx->tk != NULL);

	termkey_destroy(ioctx->tk);
	free(ioctx);
}

static ssize_t
ecma48_recv(struct cl_peer *p, struct cl_chctx chctx[],
	const void *data, size_t len)
{
	struct cl_chctx *prev;
	TermKeyKey key;
	size_t n;

	assert(p != NULL);
	assert(chctx != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioctx->tk != NULL);
	assert(chctx->ioapi->read == ecma48_recv);
	assert(data != NULL);

	prev = chctx - 1;

	(void) prev;

	if (len == 0) {
		return 0;
	}

	if (len > SSIZE_MAX) {
		errno = EINVAL;
		return -1;
	}

	n = termkey_push_bytes(chctx->ioctx->tk, data, len);
	if ((size_t) -1 == n) {
		return -1;
	}

	assert(n > 0);

	while (termkey_getkey(chctx->ioctx->tk, &key) == TERMKEY_RES_KEY) {
		struct cl_event e;

		switch (key.type) {
		case TERMKEY_TYPE_MOUSE:
			continue;

		case TERMKEY_TYPE_FUNCTION:
			switch (key.code.number) {
			case 1: e.type = UI_CODEPOINT; e.u.utf8 = "?";  break;

			default:
				continue;
			}

		case TERMKEY_TYPE_KEYSYM:
			assert(key.code.sym != TERMKEY_SYM_NONE);

			switch (key.code.sym) {
			case TERMKEY_SYM_UNKNOWN:
				continue;

			/* TODO: stateful line editing */
			case TERMKEY_SYM_INSERT:
			case TERMKEY_SYM_ESCAPE:
			case TERMKEY_SYM_UNDO:
			case TERMKEY_SYM_REPLACE:
				continue;

			case TERMKEY_SYM_BACKSPACE: e.type = UI_BACKSPACE;     break;
			case TERMKEY_SYM_DEL:       e.type = UI_DELETE;        break;
			case TERMKEY_SYM_UP:        e.type = UI_HIST_PREV;     break;
			case TERMKEY_SYM_DOWN:      e.type = UI_HIST_NEXT;     break;
			case TERMKEY_SYM_LEFT:      e.type = UI_CURSOR_LEFT;   break;
			case TERMKEY_SYM_RIGHT:     e.type = UI_CURSOR_RIGHT;  break;
			case TERMKEY_SYM_BEGIN:     e.type = UI_CURSOR_SOL;    break;
			case TERMKEY_SYM_DELETE:    e.type = UI_DELETE;        break;
			case TERMKEY_SYM_HOME:      e.type = UI_CURSOR_SOL;    break;
			case TERMKEY_SYM_END:       e.type = UI_CURSOR_EOL;    break;
			case TERMKEY_SYM_CLEAR:     e.type = UI_DELETE_TO_EOL; break;
			case TERMKEY_SYM_EXIT:      e.type = UI_CANCEL;        break;
			case TERMKEY_SYM_REDO:      e.type = UI_CANCEL;        break;
			case TERMKEY_SYM_CANCEL:    e.type = UI_CANCEL;        break;
			case TERMKEY_SYM_RESTART:   e.type = UI_CANCEL;        break;

			case TERMKEY_SYM_TAB:       e.type = UI_CODEPOINT; e.u.utf8 = "\t"; break;
			case TERMKEY_SYM_ENTER:     e.type = UI_CODEPOINT; e.u.utf8 = "\n"; break;
			case TERMKEY_SYM_SPACE:     e.type = UI_CODEPOINT; e.u.utf8 = " ";  break;
			case TERMKEY_SYM_HELP:      e.type = UI_CODEPOINT; e.u.utf8 = "?";  break;

			default:
				continue;
			}

			break;

		case TERMKEY_TYPE_UNICODE:
			e.type   = UI_CODEPOINT;
			e.u.utf8 = key.utf8;

			if (key.modifiers & TERMKEY_KEYMOD_CTRL) {
				switch (e.u.utf8[0]) {
				case 'c': e.type = UI_CANCEL;      break;
				case 'w': e.type = UI_DELETE_WORD; break;
				case 'h': e.type = UI_BACKSPACE;   break;

				/* TODO: emacs-style line editing */

				default:
					continue;
				}
			}

			break;
		}

		if (e.type == UI_CODEPOINT && key.modifiers != 0) {
			continue;
		}

		if (-1 == getc_main(p, &e)) {
			return -1;
		}
	}

	return n;
}

static ssize_t
ecma48_send(struct cl_peer *p, struct cl_chctx chctx[],
	enum ui_output output)
{
	struct cl_chctx *next;
	ssize_t n = 0;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioapi->send == ecma48_send);
	assert(chctx->ioctx != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);
	assert(next->ioapi->printf != NULL);

	switch (output) {
	case OUT_BACKSPACE_AND_DELETE:
		assert(chctx->ioctx->save == -1);

		if (p->term.cub1 == NULL) {
			return 0;
		}

		if (p->term.dch1 != NULL) {
			n = next->ioapi->printf(p, next, "%s%s",
				p->term.cub1,
				p->term.dch1);
		} else {
			n = next->ioapi->printf(p, next, "%s %s",
				p->term.cub1,
				p->term.cub1);
		}

		break;

	case OUT_SAVE:
		assert(chctx->ioctx->save == -1);

		if (p->term.sc == NULL || p->term.rc == NULL) {
			chctx->ioctx->save = 0;
			return 0;
		}

		n = next->ioapi->printf(p, next, "%s",
			p->term.sc);

		break;

	case OUT_RESTORE_AND_DELETE_TO_EOL:
		assert(chctx->ioctx->save != -1);

		if (p->term.sc == NULL || p->term.rc == NULL) {
			ssize_t x;

			for (n = 0; chctx->ioctx->save-- > 0; n += x) {
				x = ecma48_send(p, chctx, OUT_BACKSPACE_AND_DELETE);
				if (x == -1) {
					return -1;
				}
			}

			break;
		}

		n = chctx->ioapi->printf(p, chctx, "%s",
			p->term.rc);

		break;
	}

	return n;
}

static int
ecma48_vprintf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, va_list ap)
{
	struct cl_chctx *next;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioapi->vprintf == ecma48_vprintf);
	assert(fmt != NULL);

	next = chctx + 1;

	assert(next != NULL);
	assert(next->ioapi != NULL);

	n = next->ioapi->vprintf(p, next, fmt, ap);

	if (chctx->ioctx->save != -1) {
		if (chctx->ioctx->save > INT_MAX - n) {
			errno = ENOMEM;
			return -1;
		}

		if (n != -1) {
			chctx->ioctx->save += n;
		}
	}

	return n;
}

static int
ecma48_printf(struct cl_peer *p, struct cl_chctx chctx[],
	const char *fmt, ...)
{
	va_list ap;
	int n;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(chctx->ioctx != NULL);
	assert(chctx->ioapi->printf == ecma48_printf);
	assert(fmt != NULL);

	va_start(ap, fmt);
	n = ecma48_vprintf(p, chctx, fmt, ap);
	va_end(ap);

	return n;
}

struct io io_ecma48 = {
	ecma48_create,
	ecma48_destroy,
	ecma48_recv,
	ecma48_send,
	ecma48_vprintf,
	ecma48_printf
};

