#include <cl/tree.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "internal.h"

struct editctx {
	size_t count;
	char *buf;
};

static int
append(struct editctx *ectx, const char *s)
{
	const size_t blocksz = 64;
	size_t n;

	assert(ectx != NULL);
	assert(s != NULL);

	n = strlen(s);

	assert(n != 0);
	assert(n < blocksz);

/* TODO: check again... */
	if ((ectx->count + n - 1) % blocksz < n) {
		char *tmp;

		tmp = realloc(ectx->buf, ectx->count + blocksz);
		if (tmp == NULL) {
			return -1;
		}

		ectx->buf = tmp;
	}

	memcpy(ectx->buf + ectx->count, s, n + 1);

	ectx->count += n;

	return 0;
}

struct editctx *
edit_create(void)
{
	struct editctx *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->count = 0;
	new->buf   = NULL;

	return new;
}

void
edit_destroy(struct editctx *ectx)
{
	assert(ectx != NULL);

	free(ectx->buf);
	free(ectx);
}

char *
edit_release(struct editctx *ectx)
{
	char *p;

	assert(ectx != NULL);

	if (ectx->count == 0) {
		return NULL;
	}

	p = ectx->buf;

	ectx->count = 0;
	ectx->buf = NULL;

	return p;
}

static int
edit_backspace(struct cl_peer *p, size_t n)
{
	assert(p != NULL);
	assert(p->ectx != NULL);
	assert(n > 0);

	if (p->ectx->count == 0) {
		return 0;
	}

	/* XXX: this should walk back n unicode characters worth, not just n bytes */

	assert(p->ectx->buf != NULL);
	assert(p->ectx->count >= n);

	p->ectx->count -= n;
	p->ectx->buf[p->ectx->count] = '\0';

	if (n == 1) {
		if (-1 == p->chctx->ioapi->send(p, p->chctx + 0, OUT_BACKSPACE_AND_DELETE)) {
			return -1;
		}

		return 0;
	}

	/* XXX: placeholder for "go left X characters and delete to EOL */
	while (n-- > 0) {
		if (-1 == p->chctx->ioapi->send(p, p->chctx + 0, OUT_BACKSPACE_AND_DELETE)) {
			return -1;
		}
	}

	return 0;
}

static int
edit_backword(struct cl_peer *p)
{
	struct lex_tok tok[2];
	const char *src;
	size_t i;

	assert(p != NULL);
	assert(p->ectx != NULL);

	if (p->ectx->count == 0) {
		return 0;
	}

	assert(p->ectx->buf != NULL);

	src = p->ectx->buf;

	tok[0].src.start = tok[0].src.end = src;
	tok[1].src.start = tok[1].src.end = src;

	for (i = 0; lex_next(&tok[i], &src, NULL) != NULL; i = !i) {
		if (tok[i].type == TOK_ERROR) {
			break;
		}
	}

	/* leave a single trailing space if we're just deleting one
	 * token, as long as it's not SOL */
	if (tok[i].src.end != tok[i].src.start) {
		tok[i].src.end += isspace((unsigned char) *tok[i].src.end);
	}

	return edit_backspace(p, strlen(tok[i].src.end));
}

static const struct trie *
edit_walk(struct cl_peer *p, const struct trie *trie, const char *src)
{
	struct lex_tok tok;

	assert(p != NULL);
	assert(trie != NULL);

	if (p->ectx->count == 0) {
		return trie;
	}

	assert(src != NULL);

	while (lex_next(&tok, &src, NULL) != NULL) {
		const struct trie *t;

		if (tok.type != TOK_WORD) {
			break;
		}

		t = trie_walk(trie, tok.src.start, tok.src.end - tok.src.start);
		if (t == NULL) {
			break;
		}

		trie = t;

		t = trie_walk(trie, " ", 1);
		if (t == NULL) {
			break;
		}

		trie = t;
	}

	assert(trie != NULL);

	return trie;
}

int
edit_push(struct cl_peer *p, const struct cl_event *event, enum edit_flags flags)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);
	assert(p->ectx != NULL);
	assert(event != NULL);

	switch (event->type) {
	case UI_CODEPOINT:
		break;

	case UI_BACKSPACE:
		return edit_backspace(p, 1);

	case UI_DELETE_LINE:
		return edit_backspace(p, p->ectx->count);

	case UI_DELETE_WORD:
		return edit_backword(p);

	case UI_DELETE:
	case UI_DELETE_TO_EOL:
	case UI_CANCEL:	/* ^C to abort current command */
		/* TODO: not implemented */
		return 0;

	case UI_HIST_PREV:
	case UI_HIST_NEXT:
		if (~flags & EDIT_HIST) {
			return 0;
		}

		/* TODO: not implemented */
		return 0;

	case UI_CURSOR_LEFT:
	case UI_CURSOR_RIGHT:
	case UI_CURSOR_LEFT_WORD:
	case UI_CURSOR_RIGHT_WORD:
	case UI_CURSOR_EOL:
	case UI_CURSOR_SOL:
		if (~flags & EDIT_ECHO) {
			return 0;
		}

		/* TODO: not implemented */
		return 0;

	case UI_HELP:
		if (~flags & EDIT_TRIE) {
			return 0;
		}

		cl_printf(p, "?\n");

		trie_help(p, edit_walk(p, p->tree->root, p->ectx->buf), p->mode);

		{
			p->tree->printprompt(p, p->mode);

			if (p->ectx->count > 0) {
				cl_printf(p, "%s", p->ectx->buf);
			}
		}

		return 0;
	}

	switch (event->u.utf8[0]) {
	case '\0':
	case '\r':
	case '\033':
		return 0;

	case '\n':
		return 1;

	case '\t':
		if (flags & EDIT_TRIE) {
			/* TODO: tab completion not implemented */
		}

		return 0;

	case '?':
		if (flags & EDIT_TRIE) {
			struct cl_event e;

			e.type = UI_HELP;

			return edit_push(p, &e, flags);
		}

		/* FALLTHROUGH */

	default:
		if (flags & EDIT_ECHO) {
			cl_printf(p, "%s", event->u.utf8);
		}

		if (-1 == append(p->ectx, event->u.utf8)) {
			return -1;
		}

		return 0;
	}
}

