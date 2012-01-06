/* $Id$ */

/*
 * TODO: draw this entire thing as an FSM
 */

/*
                      Binary Format and Split Bytes
Code Point Range      Byte 1      Byte 2      Byte 3      Byte 4

U+000000...U+00007F   0bxxxxxxx
                      0b0xxxxxxx

U+000080...U+0007FF   0byyyyyxxxxxx
                      0b110yyyyy, 0b10xxxxxx

U+000800...U+00FFFF   0bzzzzyyyyyyxxxxxx 
                      0b1110zzzz, 0b10yyyyyy, 0b10xxxxxx
                      
U+010000...U+10FFFF   0bvvvzzzzzzyyyyyyxxxxxx 
                      0b11110vvv, 0b10zzzzzz, 0b10yyyyyy, 0b10xxxxxx
For example, these 3 Unicode characters, U+004D, U+0061 and U+10000 will be converted into 0x4D61F0908080 when UTF-8 is used.


TODO: rewrite as char_rterm.c; get rid of signal handling. don't use select(); timeout using tcsetattr instead.
add signal handlers to restore terminal on exit, and/or atexit().

*/

#include <sys/select.h>
#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "char.h"

#define ESC_TIMEOUT 50 /* ms */


volatile sig_atomic_t sig;

static void char_sig_handler_term(int s) {
	sig = s;
}

void char_term_init(int fd) {
	{
		struct termios tp;

		if (0 != tcgetattr(fd, &tp)) {
			perror("tcgetattr");
			exit(EXIT_FAILURE);
		}

		tp.c_lflag &= ~ECHO;
		tp.c_lflag &= ~ICANON;

		if (0 != tcsetattr(fd, TCSANOW, &tp)) {
			perror("tcsetattr");
			exit(EXIT_FAILURE);
		}
	}

	if (SIG_ERR == signal(SIGINT, char_sig_handler_term)) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
}

struct character char_getnext_term(int fd) {
	int mode;

	enum {
		STATE_CHAR,
		STATE_UTF8,
		STATE_ESC,
		STATE_ESC2,
		STATE_CSI
	} state;

	union {
		struct {
			int count;
			int index;
			char c[4];
		} utf8;

		struct {
			int param[3];
			int index;
		} csi;
	} u;

	assert(fd != -1);

	state = STATE_CHAR;

	mode = 0;

	for (;;) {
		char c;

		/* TODO: possibly centralise common to both char_term.c and char_net.c */
		{
			int r;
			fd_set fds;
			static struct timeval tv = { 0, ESC_TIMEOUT * 1000 };
			struct timeval *tvp;

/* TODO: according to SUS3: http://www.opengroup.org/onlinepubs/009695399/functions/read.html
"In 4.3 BSD, a read() or write() that is interrupted by a signal before transferring any data does not by default return an [EINTR] error, but is restarted. In 4.2 BSD, 4.3 BSD, and the Eighth Edition, there is an additional function, select(), whose purpose is to pause until specified activity (data to read, space to write, and so on) is detected on specified file descriptors. It is common in applications written for those systems for select() to be used before read() in situations (such as keyboard input) where interruption of I/O due to a signal is desired."
*/
/* TODO: .: use select()... then read() */
/* TODO: also use select() to *conditionally* timeout for just plain "esc" */

/* TODO: maybe timeouts need to be better than this; just \e[ should produce cmd-[. buffer somehow? */
/* cardinal rule: holding down a key must always repeat the same output.
it's ok to not have a mode set in some situations, as long as it's consistent. e.g. cmd-[ can be interpreted as just [. rationalle: some machines don't pass through certian modes anyway.
*/

			switch (state) {
			case STATE_ESC:
			case STATE_CSI:
				tvp = &tv;
				break;

			default:
				tvp = NULL;
			}

			FD_SET(fd, &fds);
			r = select(fd + 1, &fds, NULL, NULL, tvp);
			if (r == -1 && errno == EINTR) {
				return char_new_signal(sig);
			} else if (r == 0) {
				switch (state) {
				case STATE_ESC:
					return char_new_special(0, SPECIAL_ESC);

				case STATE_CSI:
					assert(c == '[');
					return char_new_literal(0, c);

				default:
					continue;
				}
			} else if (r != 1) {
				return char_new_error(strerror(errno));
			}

			r = read(fd, &c, sizeof c);
			if (r == 0) {
				return char_new_eof();
			} else if (r == -1 && errno == EINTR) {
				return char_new_signal(sig);
			} else if (r != 1) {
				return char_new_error(strerror(errno));
			}
		}

		switch (state) {
		case STATE_CHAR:
state_char:

			/* TODO: perhaps centralise this */
			/* TODO: make unicode handling conditional */
			{
				size_t i;

				static const struct {
					int mask;
					int value;
				} a[] = {
					{ 0xC0, 0x80 },	/* 0b10xxxxxx */
					{ 0x80, 0x00 },	/* 0b0xxxxxxx (ASCII) */
					{ 0xE0, 0xC0 },	/* 0b110yyyyy, 0b10xxxxxx */
					{ 0xF0, 0xE0 },	/* 0b1110zzzz, 0b10yyyyyy, 0b10xxxxxx */
					{ 0xF8, 0xF0 },	/* 0b11110vvv, 0b10zzzzzz, 0b10yyyyyy, 0b10xxxxxx */
				};

				u.utf8.count = 0;

				for (i = 0; i < sizeof a / sizeof *a; i++) {
					if (((unsigned char) c & a[i].mask) == a[i].value) {
						u.utf8.count = i;
						break;
					}
				}
			}

			switch (u.utf8.count) {
			case 1:
				if (c == SPECIAL_ESC) {
					state = STATE_ESC;
					continue;
				}

				if (c > 0 && c < 32) {
					return char_new_special(mode, c);
				}

				/* TODO: set MODE_SHIFT for simple chars, too? */
				return char_new_literal(mode, c);

			case 2:
			case 3:
			case 4:
				u.utf8.c[0] = c;
				u.utf8.index = 1;
				state = STATE_UTF8;
				continue;

			case 0:
				return char_new_error("stray follow byte");

			default:
				return char_new_error("invalid first byte");
			}

		case STATE_UTF8:
			if (((unsigned char) c & 0xC0) != 0x80) {
				return char_new_error("invalid follow byte");	/* TODO: terminology */
			}

			assert(u.utf8.index > 0);
			assert(u.utf8.index < u.utf8.count);

			u.utf8.c[u.utf8.index] = c;
			u.utf8.index++;

			if (u.utf8.index == u.utf8.count) {
				return char_new_utf8(mode, u.utf8.count, u.utf8.c);
			}

			continue;

		case STATE_ESC:
			switch (c) {
			case '[':
				state = STATE_CSI;
				u.csi.index = 0;

				/* TODO: find a better way... memset by int, perhaps */
				u.csi.param[0] = 0;
				u.csi.param[1] = 0;
				u.csi.param[2] = 0;

				continue;

			case SPECIAL_ESC:
				state = STATE_ESC2;
				continue;

			default:
				mode = MODE_CMD;
				goto state_char;
			}

		case STATE_ESC2:
			switch (c) {
			case SPECIAL_ESC:	/* held down */
				return char_new_special(0, c);

			default:
				mode = MODE_CMD;
				goto state_char;
			}

		case STATE_CSI:
			switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				u.csi.param[u.csi.index] *= 10;
				u.csi.param[u.csi.index] += c - '0';
				continue;

			case SPECIAL_ESC:	/* held down */
				return char_new_literal(0, '[');

			case '[':	/* held down */
				assert(c == '[');
				return char_new_special(0, SPECIAL_ESC);

			/* TODO: are following CSI params separated by ':' instead? */
			case ';':
				u.csi.index++;
				/* TODO: assert on u.csi.index */
				continue;

			case '~':
				if (u.csi.index > 1) {
					return char_new_error("CSI ~: too many arguments");
				}

				return char_new_special(
					u.csi.index > 0 ? u.csi.param[1] - 1 : 0,
					'0' + u.csi.param[0]);

			case SPECIAL_UP:
			case SPECIAL_DOWN:
			case SPECIAL_RIGHT:
			case SPECIAL_LEFT:
			case SPECIAL_END:
			case SPECIAL_HOME:
				return char_new_special(
					u.csi.index > 0 ? u.csi.param[1] - 1 : 0,
					c);

			default:
				return char_new_error("unrecognised CSI");
			}
		}
	}
}

