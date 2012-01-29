/* $Id$ */

#include <cl/tree.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

struct termios old;

enum {
	MODE_ALIVE
};

static void
cmd_look(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "look"));
	assert(argc >= 0);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	cl_printf(peer, "There is nothing to look at.\n");
}

static void
cmd_go(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "go"));
	assert(argc >= 0);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	cl_printf(peer, "There is nowhere to go.\n");
}

static void
cmd_quit(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "quit"));
	assert(argc >= 0);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	fclose(stdin);
}

static void
cmd_help(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "help"));
	assert(argc >= 0);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	/* TODO: switch on optional argv[0], for "help config" calls cl_help(peer, MODE_CONFIGURE) */

	cl_help(peer, mode);
}

static const char *
ttype(struct cl_peer *peer)
{
	assert(peer != NULL);

	return getenv("TERM");
}

static int
printprompt(struct cl_peer *peer, int mode)
{
	assert(peer != NULL);

	/* TODO: various (non-bitmap) modes here: alive, hungry, raining, etc */

	return cl_printf(peer, "> ");
}

static int
visible(struct cl_peer *p, int mode, int modes)
{
	assert(p != NULL);

	return 1;
}

static int
vpeerprintf(struct cl_peer *p, const char *fmt, va_list ap)
{
	assert(p != NULL);
	assert(fmt != NULL);

	return vprintf(fmt, ap);
}

static void
restore_termios(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
}

static void
set_termios(void)
{
	struct termios new;

	if (-1 == tcgetattr(STDIN_FILENO, &new)) {
		perror("tcgetattr");
		exit(1);
	}

	old = new;

	new.c_iflag &= ~IXON;
	new.c_iflag &= ~INLCR;
	new.c_iflag &= ~ICRNL;
	new.c_lflag &= ~ICANON;
	new.c_lflag &= ~ECHO;
	new.c_lflag &= ~ISIG;
	new.c_cc[VMIN]  = 1;
	new.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSANOW, &new);

	if (-1 == atexit(restore_termios)) {
		perror("atexit");
		exit(1);
	}
}

int
main(int argc, char **argv)
{
	struct cl_tree *tree;
	struct cl_peer *peer;
	int c;

	const struct cl_command commands[] = {
		{ "look", 0, 0, cmd_look, "look at something" },
		{ "go",   0, 0, cmd_go,   "go somewhere"      },
	
		{ "quit", 0, 0, cmd_quit, NULL },
		{ "help", 0, 0, cmd_help, NULL }
	};

	set_termios();

	tree = cl_create(sizeof commands / sizeof *commands, commands, 0, NULL,
		ttype, NULL, printprompt, visible, vpeerprintf);
	if (tree == NULL) {
		perror("cl_create");
		return 1;
	}

	if (argc != 1) {
		fprintf(stderr, "usage: <no arguments>\n");
		return 1;
	}

	peer = cl_accept(tree, CL_ECMA48);
	if (peer == NULL) {
		perror ("cl_accept");
		return 1;
	}

	cl_set_mode(peer, MODE_ALIVE);

	if (-1 == cl_ready(peer)) {
		return 0;
	}

	while (c = getchar(), c != EOF) {
		char a = c;

		if (-1 == cl_read(peer, &a, 1)) {
			perror("cl_read");
			cl_close(peer);
			break;
		}
	}

	cl_destroy(tree);

	return 0;
}

