/* $Id$ */

/*
 * TODO: alarm() for timeouts (is this neccessary?)
 *
 * TODO: so this is a two-pass lexer.
 * 1. deal with signals, escape sequences, utf-8 sequences etc; it produces a stream of single codepoints (and their modes)
 * 2. take in the stream of codepoints, and produce TOK_WORD etc lexemes
 *
 * can replace (1) with an equivalent telnet-speaking lexer instead of a terminal-speaking lexer
 *
 *
 * Line editor: deals with:
 *	- history
 *	- ^W to delete word etc
 *	- displaying a prompt (callback)
 *	- mapping interrupts (^C, '?' etc for help) to callbacks
 *	- a single line is a linked list of words; the command parser can use these in callbacks too
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <unistd.h>

#include "char.h"

enum token {
	TOK_WORD,
	TOK_ENTER,
	TOK_QMARK,
	TOK_CTRLC
};

static void printmode(int mode) {
	size_t i;

	struct {
		enum char_mode mode;
		char *name;
	} a[] = {
		{ MODE_SHIFT, "shift" },
		{ MODE_CTRL,  "ctrl"  },
		{ MODE_CMD,   "cmd"   }
	};

	if (mode != 0) {
		printf(", mode=%d ", mode);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (mode & a[i].mode) {
			printf("(%s) ", a[i].name);
		}
	}
}

/* TODO: lookup table */
static const char *specialname(int special) {
	switch (special) {
	case SPECIAL_NUL: return "NUL";
	case SPECIAL_SOH: return "SOH";
	case SPECIAL_STX: return "STX";
	case SPECIAL_ETX: return "ETX";
	case SPECIAL_EOT: return "EOT";
	case SPECIAL_ENQ: return "ENQ";
	case SPECIAL_ACK: return "ACK";
	case SPECIAL_BEL: return "BEL";
	case SPECIAL_BS:  return "BS";
	case SPECIAL_HT:  return "HT";
	case SPECIAL_NL:  return "NL";
	case SPECIAL_VT:  return "VT";
	case SPECIAL_NP:  return "NP";
	case SPECIAL_CR:  return "CR";
	case SPECIAL_SO:  return "SO";
	case SPECIAL_SI:  return "SI";
	case SPECIAL_DLE: return "DLE";
	case SPECIAL_DC1: return "DC1";
	case SPECIAL_DC2: return "DC2";
	case SPECIAL_DC3: return "DC3";
	case SPECIAL_DC4: return "DC4";
	case SPECIAL_NAK: return "NAK";
	case SPECIAL_SYN: return "SYN";
	case SPECIAL_ETB: return "ETB";
	case SPECIAL_CAN: return "CAN";
	case SPECIAL_EM:  return "EM";
	case SPECIAL_SUB: return "SUB";
	case SPECIAL_ESC: return "ESC";
	case SPECIAL_FS:  return "FS";
	case SPECIAL_GS:  return "GS";
	case SPECIAL_RS:  return "RS";
	case SPECIAL_US:  return "US";

	case SPECIAL_UP:     return "up";
	case SPECIAL_DOWN:   return "down";
	case SPECIAL_LEFT:   return "left";
	case SPECIAL_RIGHT:  return "right";
	case SPECIAL_END:    return "end";
	case SPECIAL_HOME:   return "home";
	case SPECIAL_PGUP:   return "pgup";
	case SPECIAL_PGDOWN: return "pgdown";
	}

	return "";
}

int main(void) {
	char_term_init(STDIN_FILENO);

	for (;;) {
		struct character c;

		c = char_getnext_term(STDIN_FILENO);

		switch (c.type) {
		case TYPE_ERROR:
			printf("<ERROR %s>\n", c.u.error);
			return 0;

		case TYPE_EOF:
			printf("<EOF>\n");
			return 0;

		case TYPE_SIGNAL:
			printf("<SIGNAL %d>\n", c.u.signal);
			continue;

		case TYPE_SPECIAL:
			printf("<SPECIAL %d>", c.u.special);

			if (c.u.special < 32) {
				printf(" <^%c (%d)>", c.u.special + 'A' - 1, c.u.special);
			}

			printf(" %s", specialname(c.u.special));

			printmode(c.mode);

			printf("\n");
			continue;

		case TYPE_LITERAL:
			if (c.u.c.codepoint < 32) {
				printf("<LITERAL #%" PRIu32 ">", c.u.c.codepoint);
			} else if (c.u.c.codepoint > 0xFF) {
				printf("<LITERAL %s U%" PRIX32 ">", c.u.c.c, c.u.c.codepoint);
			} else {
				printf("<LITERAL %s #%" PRIu32 ">", c.u.c.c, c.u.c.codepoint);
			}

			printmode(c.mode);
			printf("\n");

			switch (c.u.c.codepoint) {
			case 'q':
				printf("<QUIT>\n");
				return 0;
			}

			continue;
		}
	}

	return 0;
}

