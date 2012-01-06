/* $Id$ */

#ifndef CHAR_H
#define CHAR_H

enum char_mode {
	MODE_SHIFT = 1 << 0,
	MODE_CTRL  = 1 << 2,
	MODE_CMD   = 1 << 3
};

enum char_type {
	TYPE_EOF,
	TYPE_LITERAL,
	TYPE_SPECIAL,
	TYPE_SIGNAL,
	TYPE_ERROR
};

enum char_special {
	/* ASCII control codes */
	SPECIAL_NUL =  0,
	SPECIAL_SOH =  1,
	SPECIAL_STX =  2,
	SPECIAL_ETX =  3,
	SPECIAL_EOT =  4,
	SPECIAL_ENQ =  5,
	SPECIAL_ACK =  6,
	SPECIAL_BEL =  7,
	SPECIAL_BS  =  8,
	SPECIAL_HT  =  9,
	SPECIAL_NL  = 10,
	SPECIAL_VT  = 11,
	SPECIAL_NP  = 12,
	SPECIAL_CR  = 13,
	SPECIAL_SO  = 14,
	SPECIAL_SI  = 15,
	SPECIAL_DLE = 16,
	SPECIAL_DC1 = 17,
	SPECIAL_DC2 = 18,
	SPECIAL_DC3 = 19,
	SPECIAL_DC4 = 20,
	SPECIAL_NAK = 21,
	SPECIAL_SYN = 22,
	SPECIAL_ETB = 23,
	SPECIAL_CAN = 24,
	SPECIAL_EM  = 25,
	SPECIAL_SUB = 26,
	SPECIAL_ESC = 27,
	SPECIAL_FS  = 28,
	SPECIAL_GS  = 29,
	SPECIAL_RS  = 30,
	SPECIAL_US  = 31,

	/* CSI control codes */
	SPECIAL_UP     = 'A',
	SPECIAL_DOWN   = 'B',
	SPECIAL_RIGHT  = 'C',
	SPECIAL_LEFT   = 'D',

	SPECIAL_END    = 'F',
	SPECIAL_HOME   = 'H',

	/* '~' function keys */
	SPECIAL_PGUP   = '5',
	SPECIAL_PGDOWN = '6'
};

struct character {
	enum char_type type;
	unsigned int mode;

	union {
		struct {
			uint32_t codepoint;
			char c[4];	/* null-terminated UTF-8 byte sequence */
		} c;	/* TODO: rename "literal" */

		enum char_special special;

		int signal;

		const char *error;
	} u;
};


struct character char_new_literal(int mode, char c);
struct character char_new_utf8(int mode, int count, char c[4]);
struct character char_new_special(int mode, int s);
struct character char_new_signal(int s);
struct character char_new_error(const char *e);
struct character char_new_eof(void);


void char_term_init(int fd);
struct character char_getnext_term(int fd);

#endif

