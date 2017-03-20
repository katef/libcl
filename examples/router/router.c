/*
 * Copyright 2012-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 600
#endif

/* for INADDR_NONE */
#ifdef __APPLE__
# define _DARWIN_C_SOURCE
#endif

#if defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__sun)
# undef  HAVE_SALEN
#elif defined(__sun)
# undef  HAVE_SALEN
#else
# define HAVE_SALEN
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cl/tree.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>	/* XXX: C99, for SIZE_MAX */
#include <limits.h>
#include <errno.h>

/* XXX: va_copy is C99; this workaround is not portable */
#ifndef va_copy
# if defined(__GNUC__) || defined(__clang__)
#  define va_copy(dst, src) __builtin_va_copy(dst, src)
# else
#  define va_copy(dst, src) ((dst) = (src))
# endif
#endif

#define MOTD "This is an example command server for libcl. Type help for help."

#define USERNAME "alice"
#define PASSWORD "hello"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum {
	MODE_CONNECTED = 1 << 0,
	MODE_DISABLED  = 1 << 1,
	MODE_ENABLED   = 1 << 2,
	MODE_CONFIGURE = 1 << 3,
	MODE_INTERFACE = 1 << 4,
	MODE_EFFECT    = 1 << 5,
	MODE_LINK      = 1 << 6
};

enum {
	FIELD_USERNAME = 1 << 0,
	FIELD_PASSWORD = 1 << 1
};

struct peer {
	int fd;
	struct cl_peer *peer;
	char item[32];

	struct peer *next;
};

static struct peer *
addpeer(struct peer **peers, int fd, struct cl_peer *peer)
{
	struct peer *new;

	assert(fd != -1);
	assert(peer != NULL);
	assert(peers != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		perror("malloc");
		return NULL;
	}

	new->fd   = fd;
	new->peer = peer;

	new->next = *peers;
	*peers = new;

	return new;
}

static struct cl_peer *
findpeer(struct peer *peers, int fd)
{
	struct peer *p;

	assert(fd != -1);

	for (p = peers; p != NULL; p = p->next) {
		if (p->fd == fd) {
			return p->peer;
		}
	}

	return NULL;
}

static int
validate_name(struct cl_peer *p, int id, const char *value)
{
	const char *valid= "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	                   "abcdefghijklmnopqrstuvwxyz" \
	                   "0123456789" "_";

	assert(p != NULL);
	assert(id == FIELD_USERNAME);
	assert(value != NULL);

	(void) id;

	if (strlen(value) == 0) {
		cl_printf(p, "a name may not be empty\n");
		return 0;
	}

	/* or a regexp... /[0-9a-zA-Z_]+/ */
	if (value[strspn(value, valid)] != '\0') {
		cl_printf(p, "a name may only contain [0-9a-zA-Z_]\n");
		return 0;
	}

	return 1;
}

static int
motd(struct cl_peer *p)
{
	assert(p != NULL);

	cl_printf(p, "%s\n", MOTD);

	return 0;
}

static void
cmd_motd(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "show motd"));
	assert(mode == MODE_CONNECTED);
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	motd(p);
}

static void
cmd_login(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	const char *user;
	const char *pass;
	static int attempts;

	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "login"));
	assert(mode == MODE_CONNECTED);
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	user = cl_get_field(p, FIELD_USERNAME);
	pass = cl_get_field(p, FIELD_PASSWORD);

	assert(user != NULL);
	assert(pass != NULL);

	if (0 == strcmp(user, USERNAME) && 0 == strcmp(pass, PASSWORD)) {
		cl_set_mode(p, MODE_DISABLED);
		attempts = 0;
		return;
	}

	printf("invalid login for %s\n", user);

	if (attempts > 3) {
		cl_printf(p, "invalid password\n");
		attempts = 0;
		return;
	}

	attempts++;
	cl_again(p);
}

static void
cmd_enable(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	const char *pass;
	static int attempts;

	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "enable"));
	assert(mode == MODE_DISABLED);
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	pass = cl_get_field(p, FIELD_PASSWORD);

	assert(pass != NULL);

	if (0 == strcmp(pass, PASSWORD)) {
		cl_set_mode(p, MODE_ENABLED);
		attempts = 0;
		return;
	}

	if (attempts > 3) {
		cl_printf(p, "invalid password\n");
		attempts = 0;
		return;
	}

	attempts++;
	cl_again(p);
}

static void
cmd_config_term(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "configure terminal"));
	assert(mode == MODE_ENABLED);
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	cl_set_mode(p, MODE_CONFIGURE);
}

/* argv[] are arguments after the "enable" command: "enable x y z". nothing to do with fields */
static void
cmd_config(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "interface")
	    || 0 == strcmp(cmd, "effect")
	    || 0 == strcmp(cmd, "link"));
	assert(mode == MODE_CONFIGURE);
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) mode;

	if (argc != 1) {
		cl_printf(p, "invalid cardinality\n");
		/* or you could: cl_help(p, mode) */
		return;
	}

	assert(strlen(argv[0]) != 0);	/* because libcl doesn't permit "" here */

	/* TODO: hacky to pass FIELD_USERNAME here */
	if (!validate_name(p, FIELD_USERNAME, argv[0])) {
		return;
	}

	{
		struct peer *peer;

		peer = cl_get_opaque(p);

		assert(peer != NULL);
		assert(argv[0] != NULL);
		assert(strlen(argv[0]) > 0);

		snprintf(peer->item, sizeof peer->item, "%s", argv[0]);
	}

	switch (cmd[0]) {
	case 'i': cl_set_mode(p, MODE_INTERFACE); break;
	case 'e': cl_set_mode(p, MODE_EFFECT);    break;
	case 'l': cl_set_mode(p, MODE_LINK);      break;
	}
}

static void
cmd_exit(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "exit"));
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	/* XXX: i dislike this; we have mode knowledge scattered all over the place,
	 * where it clearly could be automated into a tree */
	switch (mode) {
	case MODE_INTERFACE: cl_set_mode(p, MODE_CONFIGURE); break;
	case MODE_EFFECT:    cl_set_mode(p, MODE_CONFIGURE); break;
	case MODE_LINK:      cl_set_mode(p, MODE_CONFIGURE); break;
	case MODE_CONFIGURE: cl_set_mode(p, MODE_ENABLED);   break;
	case MODE_ENABLED:   cl_set_mode(p, MODE_DISABLED);  break;
	case MODE_DISABLED:  cl_set_mode(p, MODE_CONNECTED); break;
	case MODE_CONNECTED: cl_close(p);                    break;
	}
}

static void
cmd_help(struct cl_peer *p, const char *cmd, int mode, int argc, const char *argv[])
{
	assert(p != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "help"));
	assert(argc >= 0);
	assert(argv != NULL);
	assert(argv[argc] == NULL);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(p, "invalid cardinality\n");
		return;
	}

	/* TODO: switch on optional argv[0], for "help config" calls cl_help(p, MODE_CONFIGURE) */

	cl_help(p, mode);
}

static int
printprompt(struct cl_peer *p, int mode)
{
	struct peer *peer;

	assert(p != NULL);

	peer = cl_get_opaque(p);

	assert(peer != NULL);

/* TODO: assert(single bit in mode); */

	switch (mode) {
	case MODE_CONNECTED: return cl_printf(p, "> ");	/* XXX: not for pre-motd. make a MODE_MOTD? */
	case MODE_DISABLED:  return cl_printf(p, "> ");
	case MODE_ENABLED:   return cl_printf(p, "# ");
	case MODE_CONFIGURE: return cl_printf(p, "config# ");
	case MODE_INTERFACE: return cl_printf(p, "config(%s)# ", peer->item);
	case MODE_LINK:      return cl_printf(p, "config(%s)# ", peer->item);
	case MODE_EFFECT:    return cl_printf(p, "config(%s)# ", peer->item);
	}

	/* UNREACHED */
	return -1;
}

const struct cl_field fields[] = {
	{ FIELD_USERNAME, "username", 1, validate_name },
	{ FIELD_PASSWORD, "password", 0, NULL          }
};

const struct cl_command commands[] = {
	{ "show motd",          MODE_CONNECTED, 0,                               cmd_motd,  "message of the day" },
	{ "login",              MODE_CONNECTED, FIELD_USERNAME | FIELD_PASSWORD, cmd_login, NULL },

	{ "enable",             MODE_DISABLED,  FIELD_PASSWORD, cmd_enable, "usage" },

	{ "configure terminal", MODE_ENABLED,   0, cmd_config_term, "usage" },
	{ "interface",          MODE_CONFIGURE, 0, cmd_config,      "usage" },
	{ "effect",             MODE_CONFIGURE, 0, cmd_config,      "usage" },
	{ "link",               MODE_CONFIGURE, 0, cmd_config,      "usage" },

/* TODO:
	{ "set name",           MODE_INTERFACE | MODE_EFFECT | MODE_LINK, 0, cmd_setname,  "usage" },
	{ "set description",    MODE_INTERFACE | MODE_EFFECT | MODE_LINK, 0, cmd_setdesc,  "usage" },
	{ "set mtu",            MODE_INTERFACE,                           0, cmd_setmtu,   "usage" },
	{ "set level",                           MODE_EFFECT,             0, cmd_setlevel, "usage" },
*/

	/*
	 * The "exit" command would be under ~0U for all modes, but I wanted to show
	 * how to provide different usage text for particular modes.
	 */
	{ "exit", ~MODE_DISABLED & ~MODE_CONNECTED, 0, cmd_exit, "return to the previous mode" },
	{ "exit",  MODE_DISABLED,                   0, cmd_exit, "logout"                      },
	{ "exit",  MODE_CONNECTED,                  0, cmd_exit, "disconnect"                  },

	{ "help", ~0U, 0, cmd_help, NULL }
};

static int
vpeerprintf(struct cl_peer *p, const char *fmt, va_list ap)
{
	struct peer *peer;
	va_list ap1;
	char a[1024];
	char *buf = a;
	int n;

	assert(p != NULL);
	assert(fmt != NULL);

	peer = cl_get_opaque(p);

	assert(peer != NULL);
	assert(peer->fd != -1);

	/* XXX: va_copy is C99 */
	/* XXX: also broken as an argument on macos/amd64 */
	va_copy(ap1, ap);

	/* XXX: vsnprintf is C99 */
	n = vsnprintf(a, sizeof a, fmt, ap);

	if (n == 0) {
		return 0;
	}

	if (n > SSIZE_MAX) {
		errno = EINVAL;
		return -1;
	}

	assert(n != -1);
	assert(n < SIZE_MAX);
	assert(n < SSIZE_MAX);

	if (n > sizeof a) {
		buf = malloc(n + 1);
		if (buf == NULL) {
			return -1;
		}

		vsnprintf(buf, n + 1, fmt, ap1);
		va_end(ap1);
	}

	n = (int) write(peer->fd, buf, n);

	if (buf != a) {
		free(buf);
	}

	return n;
}

int
main(int argc, char **argv)
{
	int s;
	in_addr_t a;
	in_port_t p;
	struct sockaddr_in sin;
	struct cl_tree *tree;

	tree = cl_create(sizeof commands / sizeof *commands, commands,
		sizeof fields / sizeof *fields, fields,
		NULL, motd, printprompt, cl_visible, vpeerprintf);
	if (tree == NULL) {
		perror("cl_create");
		return 1;
	}

	if (argc != 3) {
		fprintf(stderr, "usage: <ip> <port>\n");
		return 1;
	}

	/* Port */
	{
		long int l;
		char *ep;

		l = strtol(argv[2], &ep, 10);
		if (*ep) {
			fprintf(stderr, "invalid port\n");
			return 1;
		}

		if (l < 0 || (unsigned long int) l > 0xffffUL) {
			fprintf(stderr, "port out of range\n");
			return 1;
		}

		p = l;
	}

	/* Address */
	{
		a = inet_addr(argv[1]);
		if (INADDR_NONE == a) {
			fprintf(stderr, "malformed address\n");
			return 1;
		}

		memset(&sin, 0, sizeof sin);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(p);
		sin.sin_addr.s_addr = a;
#ifdef HAVE_SALEN
		sin.sin_len = sizeof sin;
#endif
	}

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == s) {
		perror("socket");
		return 1;
	}

	{
		const int ov = 1;

		if (-1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof ov)) {
			perror("setsockopt");
			close(s);
			return 1;
		}

		if (-1 == bind(s, (void *) &sin, sizeof sin)) {
			perror("bind");
			close(s);
			return 1;
		}

		if (-1 == listen(s, 1)) {
			perror("listen");
			close(s);
			return 1;
		}
	}

	{
		fd_set master;
		int maxfd;
		struct peer *peers;

		FD_ZERO(&master);
		FD_SET(s, &master);

		maxfd = s;
		peers = NULL;

		for (;;) {
			int i;
			fd_set curr;

			curr = master;

			if (-1 == select(maxfd + 1, &curr, NULL, NULL, NULL)) {
				perror("select");
				return 1;
			}

			if (FD_ISSET(s, &curr)) {
				struct cl_peer *peer;
				struct peer *new;

				i = accept(s, NULL, 0);
				if (-1 == i) {
					perror ("accept");
					return 1;
				}

				FD_SET(i, &master);
				maxfd = MAX(maxfd, i);

				peer = cl_accept(tree, CL_TELNET);
				if (peer == NULL) {
					perror ("cl_accept");
					return 1;
				}

				new = addpeer(&peers, i, peer);
				if (new == NULL) {
					perror ("addpeer");
					return -1;
				}

				cl_set_opaque(peer, new);
				cl_set_mode(peer, MODE_CONNECTED);

				if (-1 == cl_ready(peer)) {
					perror ("cl_ready");
					return 1;
				}
			}

			for (i = 0; i <= maxfd; ++i) {
				struct cl_peer *peer;
				char buf[BUFSIZ];
				ssize_t n;

				if (i == s || !FD_ISSET(i, &curr)) {
					continue;
				}

				n = read(i, buf, sizeof buf);
				if (-1 == n) {
					perror("read");
					FD_CLR(i, &master);
					close(i);
					continue;
				}

				assert(n >= 0);

				if (n == 0) {
					continue;
				}

				peer = findpeer(peers, i);

				assert(peer != NULL);

				if (-1 == cl_read(peer, buf, (size_t) n)) {
					perror("cl_read");

					/* TODO: remove peer from peers ll */
					FD_CLR(i, &master);
					close(i);
					continue;
				}
			}
		}
	}

	/* NOTREACHED */

	cl_destroy(tree);

	close(s);

	return 0;
}

