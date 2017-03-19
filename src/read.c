#include <cl/tree.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "internal.h"

enum readstate {
	STATE_NEW,
	STATE_COMMAND,
	STATE_FIELD
};

struct value {
	int id;
	char *value;

	struct value *next;
};

struct readctx {
	enum readstate state;
	const struct trie *t;
	int fields;
	struct value *values;
	int argc;
	const char **argv;
};

/*
 * The command is stored verbatim as input by the user, followed directly by
 * each token's content, each null terminated.
 *
 * Tokens have escape sequences expanded, string quotes skipped and inter-token
 * whitespace omitted. For example (in C's notation):
 *
 *  src = " abc \t  def 'xyz' \"x\\t\\n\""
 *  dst = "abc\0def\0xyz\0x\t\n"
 *
 * gives the four tokens "abc", "def", "xyz" and "x\t\n".
 *
 * The worst case for memory use here is a sequence of pipes, where every
 * character requires termination. So the worst case memory for dst is twice
 * src, plus one for the single terminator at the end of src.
 */
static int
parsecommand(struct cl_peer *p, const char *src, char *dst)
{
	struct lex_tok tok;

	assert(p != NULL);
	assert(src != NULL);
	assert(dst != NULL);

	p->rctx->t = p->tree->root;

	while (lex_next(&tok, &src, &dst) != NULL) {
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_PIPE:	/* not implemented */
			cl_printf(p, "syntax error at: %.*s\n",
				(int) (tok.src.end - tok.src.start), tok.src.start);

			return 0;

		case TOK_WORD:
			break;

		default:
			cl_printf(p, "command not found\n");

			return 0;
		}

		p->rctx->t = trie_walk(p->rctx->t, tok.dst.start, tok.dst.end - tok.dst.start);
		if (p->rctx->t == NULL) {
			cl_printf(p, "command not found\n");

			return 0;
		}

		p->rctx->t = trie_run(p, p->rctx->t, p->mode, ' ');
		if (p->rctx->t == NULL) {
			cl_printf(p, "command not found\n");

			return 0;
		}

		/* TODO: unneccessary when argv changes to contain the entire command */
		/* TODO: then also break on a string */
		if (p->rctx->t->command != NULL) {
			break;
		}
	}

	{
		if (p->rctx->t == p->tree->root) {
			return 0;
		}

		if (p->rctx->t == NULL) {
			cl_printf(p, "command not found\n");

			return 0;
		}

		if (p->rctx->t->command == NULL) {
			cl_printf(p, "command not found\n");

			return 0;
		}

		if (!p->tree->visible(p, p->mode, p->rctx->t->command->modes)) {
			cl_printf(p, "command not found\n");

			return 0;
		}
	}

	p->rctx->argc = 0;
	p->rctx->argv = NULL;

	while (lex_next(&tok, &src, &dst) != NULL) {
		switch (tok.type) {
		case TOK_ERROR:
		case TOK_PIPE:	/* not implemented */
			cl_printf(p, "syntax error at: %.*s\n",
				(int) (tok.src.end - tok.src.start), tok.src.start);

		case TOK_WORD:
		case TOK_STRING:
			break;
		}

		if (p->rctx->argc == INT_MAX) {
			free(p->rctx->argv);
			errno = ENOMEM;
			return -1;
		}

		if (p->rctx->argc % 8 == 0) {
			const char **tmp;

			tmp = realloc(p->rctx->argv, sizeof *p->rctx->argv * (p->rctx->argc + 8 + 1));
			if (tmp == NULL) {
				free(p->rctx->argv);
				return -1;
			}

			p->rctx->argv = tmp;
		}

		p->rctx->argv[p->rctx->argc++] = tok.dst.start;
	}

	if (p->rctx->argc == 0) {
		static const char *argv[] = { NULL };
		p->rctx->argv = argv;
	} else {
		p->rctx->argv[p->rctx->argc] = NULL;
	}

	return 1;
}

static void
fieldprompt(struct cl_peer *p)
{
	const struct cl_field *f;

	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->rctx->fields != 0);

	f = find_field(p->tree, p->rctx->fields & ~(p->rctx->fields - 1));

	assert(f != NULL);

	cl_printf(p, "%s: ", f->name);
}

struct readctx *
read_create(void)
{
	struct readctx *new;

	new = malloc(sizeof *new + 1);
	if (new == NULL) {
		return NULL;
	}

	new->state = STATE_NEW;

	return new;
}

void
read_destroy(struct readctx *rctx)
{
	assert(rctx != NULL);

	free(rctx);
}

const char *
read_get_field(struct readctx *rctx, int id)
{
	const struct value *v;

	assert(rctx != NULL);

	for (v = rctx->values; v != NULL; v = v->next) {
		if (v->id == id) {
			return v->value;
		}
	}

	return NULL;
}

static enum edit_flags
flags(enum readstate state)
{
	switch (state) {
	case STATE_NEW:
	case STATE_COMMAND: return EDIT_ECHO | EDIT_TRIE | EDIT_HIST;
	case STATE_FIELD:   return EDIT_ECHO; /* TODO: depends on the field */
	}

	return 0;
}

int
getc_main(struct cl_peer *p, const struct cl_event *event)
{
	assert(p != NULL);
	assert(p->tree != NULL);
	assert(p->tree->root != NULL);
	assert(p->rctx != NULL);
	assert(p->ectx != NULL);
	assert(event != NULL);

	{
		int r;

		r = edit_push(p, event, flags(p->rctx->state));
		if (r == -1) {
			return -1;
		}

		if (r == 0) {
			return 0;
		}
	}

	switch (p->rctx->state) {
	case STATE_NEW:
		p->rctx->state  = STATE_COMMAND;

		/* FALLTHROUGH */

	case STATE_COMMAND:
		cl_printf(p, "\n");

		{
			size_t count;
			char *src;
			char *buf;
			int r;

			buf = edit_release(p->ectx);

			if (buf == NULL) {
				p->tree->printprompt(p, p->mode);

				p->rctx->state = STATE_NEW;
				return 0;
			}

			/*
			 * Note there is no point in checking for an empty string here,
			 * as for example it could be entirely whitespace, and we'd still
			 * need to lex it to find out.
			 */
			count = strlen(buf);

			/* see parsecommand() */
			src = realloc(buf, count * 3 + 1);
			if (src == NULL) {
				/* TODO: free something? */
				return -1;
			}

			r = parsecommand(p, src, src + count + 1);
			if (r == -1) {
				/* TODO: free argv etc */
				return -1;
			}

			if (r == 0) {
				p->tree->printprompt(p, p->mode);

				p->rctx->state = STATE_NEW;
				return 0;
			}

			free(src);
		}

		p->rctx->fields = p->rctx->t->command->fields;
		p->rctx->values = NULL;
		p->rctx->state  = STATE_FIELD;

		goto firstfield;

	/* TODO: I'm not sure we even need fields. Can they be viewed as a
	 * different kind of prompt, but without word-tokenised whitespace input?
	 * i.e. without the trie? so we'd have STATE_FIELD instead of STATE_CHAR,
	 * and ignore the trie. but that's essentially what we're doing here. */
	case STATE_FIELD:
		cl_printf(p, "\n");

		p->rctx->values->value = edit_release(p->ectx);

		/* TODO: call validate callback here */

		p->rctx->fields &= p->rctx->fields - 1;

firstfield:

		if (p->rctx->fields == 0) {
			/* TODO: re-set EDIT_ECHO */

			p->rctx->t->command->callback(p, p->rctx->t->command->command,
				p->mode, p->rctx->argc, p->rctx->argv);

			/* TODO: free .values list */
			/* TODO: free argv etc */

			p->tree->printprompt(p, p->mode);

			if (p->rctx->argc > 0) {
				free(p->rctx->argv);
			}

			p->rctx->state = STATE_NEW;
			return 0;
		}

		{
			struct value *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				/* TODO: free .values list */
				/* TODO: free argv etc */
				return -1;
			}

			new->id   = p->rctx->fields & ~(p->rctx->fields - 1);
			new->next = p->rctx->values;

			p->rctx->values = new;

			/* TODO: set EDIT_ECHO as per this field */
		}

		fieldprompt(p);

		return 0;
	}

	/* UNREACHED */

	return -1;
}

