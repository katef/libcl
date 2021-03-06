/*
 * Copyright 2012-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "internal.h"

struct lex_tok *
lex_next(struct lex_tok *new, const char **src, char **dst)
{
	const char *s;
	char *d;

	enum {
		STATE_SPACE,
		STATE_PIPE,
		STATE_WORD,
		STATE_STR1,
		STATE_STR2,
		STATE_ESC
	} state;

	assert(new != NULL);
	assert(src != NULL);

	s = *src;
	d = dst ? *dst : NULL;

	assert(!strchr(s, '\r'));
	assert(!strchr(s, '\n'));
	assert(!strchr(s, '\v'));
	assert(!strchr(s, '\f'));
	assert(!strchr(s, '\t'));

	state = STATE_SPACE;

	new->dst.start = d;

	for (;; s++) {
		switch (state) {
		case STATE_SPACE:
			switch (isspace((unsigned char) *s) ? ' ' : *s) {
			case '\0':                                         return NULL;
			case ' ':                                          continue;
			case '\'': new->src.start = s; state = STATE_STR1; continue;
			case '\"': new->src.start = s; state = STATE_STR2; continue;
			case '|':  new->src.start = s; state = STATE_PIPE; break;
			default:   new->src.start = s; state = STATE_WORD; break;
			}

			if (d) *d++ = *s;
			continue;

		case STATE_PIPE:
			switch (*s) {
			default: goto done;
			}

		case STATE_WORD:
			switch (isspace((unsigned char) *s) ? ' ' : *s) {
			case '\0': goto done;
			case ' ':  goto done;
			case '\'': goto done;
			case '\"': goto done;
			case '|':  goto done;
			default:   break;
			}

			if (d) *d++ = *s;
			continue;

		case STATE_STR1: /* without escaping */
			switch (*s) {
			case '\0':       goto error;
			case '\'': s++;  goto done;
			default:         break;
			}

			if (d) *d++ = *s;
			continue;

		case STATE_STR2: /* with escaping */
			switch (*s) {
			case '\0':                    goto error;
			case '\"': s++;               goto done;
			case '\\': state = STATE_ESC; continue;
			default:                      break;
			}

			if (d) *d++ = *s;
			continue;

		case STATE_ESC:
			/* TODO: octal, hex etc */
			switch (*s) {
			case '\0':                                           goto error;
			case '\\': if (d) *d++ = '\\';   state = STATE_STR2; continue;
			case '\"': if (d) *d++ = '\"';   state = STATE_STR2; continue;
			case 'n':  if (d) *d++ = '\n';   state = STATE_STR2; continue;
			case 'r':  if (d) *d++ = '\r';   state = STATE_STR2; continue;
			case 't':  if (d) *d++ = '\t';   state = STATE_STR2; continue;
			case 'v':  if (d) *d++ = '\v';   state = STATE_STR2; continue;
			case 'f':  if (d) *d++ = '\f';   state = STATE_STR2; continue;
			case 'e':  if (d) *d++ = '\033'; state = STATE_STR2; continue;
			default:                                             goto error;
			}
		}
	}

error:

	state = -1;

done:

	*src = s;
	if (d) *d = '\0';

	new->dst.end = d;
	new->src.end = s;

	switch (state) {
	case STATE_PIPE:  new->type = TOK_PIPE;   return new;
	case STATE_WORD:  new->type = TOK_WORD;   return new;
	case STATE_STR1:  new->type = TOK_STRING; return new;
	case STATE_STR2:  new->type = TOK_STRING; return new;
	default:          new->type = TOK_ERROR;  return new;
	}
}

