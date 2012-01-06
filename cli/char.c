/* $Id$ */

#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "char.h"


struct character char_new_literal(int mode, char c) {
	struct character new;

	new.type = TYPE_LITERAL;
	new.mode = mode;
	new.u.c.c[0] = c;
	new.u.c.c[1] = '\0';
	new.u.c.codepoint = (signed char) c;

	return new;
}

struct character char_new_utf8(int mode, int count, char c[4]) {
	struct character new;
	size_t i;

	assert(count > 1);
	assert(count < 5);

	new.type = TYPE_LITERAL;
	new.mode = mode;

	memcpy(new.u.c.c, c, count);
	new.u.c.c[count] = '\0';

	{
		static const int a[] = { 0, 0, 0x7F, 0x0F, 0x07 };

		new.u.c.codepoint = c[0] & a[count];
	}

	for (i = 1; i < count; i++) {
		new.u.c.codepoint <<= 6;
		new.u.c.codepoint |= c[i] & 0x3F;
	}

	/* XXX: codepoint is assembled incorrectly... */

	return new;
}

struct character char_new_special(int mode, int s) {
	struct character new;

	new.type = TYPE_SPECIAL;
	new.mode = mode;
	new.u.special = s;

	return new;
}

struct character char_new_signal(int s) {
	struct character new;

	new.type = TYPE_SIGNAL;
	new.u.signal = s;

	return new;
}

struct character char_new_error(const char *e) {
	struct character new;

	assert(e != NULL);

	new.type = TYPE_ERROR;
	new.u.error = e;

	/* TODO: resync stream to the next start byte */

	return new;
}

struct character char_new_eof(void) {
	struct character new;

	new.type = TYPE_EOF;

	return new;
}

