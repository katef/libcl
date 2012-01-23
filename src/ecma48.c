/* $Id$ */

#include <sys/types.h>

#include <cl/tree.h>

#include <termkey.h>

#include <assert.h>
#include <stddef.h>
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
ecma48_recv(struct cl_peer *p, struct ioctx *ioctx, const void *data, size_t len)
{
	TermKeyKey key;
	size_t n;

	assert(p != NULL);
	assert(ioctx->tk != NULL);
	assert(data != NULL);

	if (len == 0) {
		return 0;
	}

	if (len > SSIZE_MAX) {
		errno = EINVAL;
		return -1;
	}

	n = termkey_push_bytes(ioctx->tk, data, len);
	if ((size_t) -1 == n) {
		return -1;
	}

	assert(n > 0);

	while (termkey_getkey(ioctx->tk, &key) == TERMKEY_RES_KEY) {
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
ecma48_send(struct cl_peer *p, struct ioctx *ioctx, const struct cl_output *output)
{
	ssize_t n = 0;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->vprintf != NULL);
	assert(ioctx != NULL);
	assert(output != NULL);

	switch (output->type) {
	case OUT_PRINTF:
		n = p->tree->vprintf(p, output->u.printf.fmt, output->u.printf.ap);

		if (ioctx->save != -1) {
			if (ioctx->save > INT_MAX - n) {
				errno = ENOMEM;
				return -1;
			}

			if (n != -1) {
				ioctx->save += n;
			}
		}

		break;

	case OUT_BACKSPACE_AND_DELETE:
		if (p->term.cub1 == NULL) {
			return 0;
		}

		if (p->term.dch1 != NULL) {
			n = p->tree->printf(p, "%s%s",
				p->term.cub1,
				p->term.dch1);
		} else {
			n = p->tree->printf(p, "%s %s",
				p->term.cub1,
				p->term.cub1);
		}

		break;

	case OUT_SAVE:
		if (p->term.sc == NULL || p->term.rc == NULL) {
			ioctx->save = 0;
			return 0;
		}

		n = p->tree->printf(p, "%s",
			p->term.sc);

		break;

	case OUT_RESTORE_AND_DELETE_TO_EOL:
		if (p->term.sc == NULL || p->term.rc == NULL) {
			ssize_t x;

			for (n = 0; ioctx->save-- > 0; n += x) {
				struct cl_output o;

				o.type = OUT_BACKSPACE_AND_DELETE;

				x = ecma48_send(p, ioctx, &o);
				if (x == -1) {
					return -1;
				}
			}

			break;
		}

		n = p->tree->printf(p, "%s",
			p->term.rc);

		break;
	}

	return n;
}

struct io io_ecma48 = {
	ecma48_create,
	ecma48_destroy,
	ecma48_recv,
	ecma48_send
};

