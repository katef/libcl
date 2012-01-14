/* $Id$ */

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

	struct peer *next;
};

static int
validate_name(struct cl_peer *peer, int id, const char *value)
{
	const char *valid= "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	                   "abcdefghijklmnopqrstuvwxyz" \
	                   "0123456789" "_";

	assert(id == FIELD_USERNAME);
	assert(peer != NULL);

	(void) id;

	if (strlen(value) == 0) {
		cl_printf(peer, "a name may not be empty\n");
		return 0;
	}

	/* or a regexp... /[0-9a-zA-Z_]+/ */
	if (value[strcspn(value, valid)] != '\0') {
		cl_printf(peer, "a name may only contain [0-9a-zA-Z_]\n");
		return 0;
	}

	return 1;
}

static void
cmd_motd(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "show motd"));
	assert(mode == MODE_CONNECTED);
	assert(argc >= 0);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	cl_printf(peer, MOTD);
}

static void
cmd_login(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	const char *user;
	const char *pass;
	static int attempts;

	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "login"));
	assert(mode == MODE_CONNECTED);
	assert(argc >= 0);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	user = cl_get_field(peer, FIELD_USERNAME);
	pass = cl_get_field(peer, FIELD_PASSWORD);

	assert(user != NULL);
	assert(pass != NULL);

	if (0 == strcmp(user, USERNAME) && 0 == strcmp(pass, PASSWORD)) {
		cl_set_mode(peer, MODE_DISABLED);
		attempts = 0;
		return;
	}

	if (attempts > 3) {
		cl_printf(peer, "invalid password");
		attempts = 0;
		return;
	}

	attempts++;
	cl_again(peer);
}

static void
cmd_enable(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	const char *pass;
	static int attempts;

	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "enable"));
	assert(mode == MODE_DISABLED);
	assert(argc >= 0);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	pass = cl_get_field(peer, FIELD_PASSWORD);

	assert(pass != NULL);

	if (0 == strcmp(pass, PASSWORD)) {
		cl_set_mode(peer, MODE_ENABLED);
		attempts = 0;
		return;
	}

	if (attempts > 3) {
		cl_printf(peer, "invalid password");
		attempts = 0;
		return;
	}

	attempts++;
	cl_again(peer);
}

static void
cmd_config_term(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "configure terminal"));
	assert(mode == MODE_ENABLED);
	assert(argc >= 0);

	(void) cmd;
	(void) mode;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	cl_set_mode(peer, MODE_CONFIGURE);
}

/* argv[] are arguments after the "enable" command: "enable x y z". nothing to do with fields */
static void
cmd_config(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "interface")
	    || 0 == strcmp(cmd, "effect")
	    || 0 == strcmp(cmd, "link"));
	assert(mode == MODE_CONFIGURE);
	assert(argc >= 0);

	(void) cmd;
	(void) mode;

	if (argc != 1) {
		cl_printf(peer, "invalid cardinality");
		/* or you could: cl_help(peer, mode) */
		return;
	}

	assert(strlen(argv[0]) != 0);	/* because libcl doesn't permit "" here */

	/* TODO: hacky to pass FIELD_USERNAME here */
	if (!validate_name(peer, FIELD_USERNAME, argv[0])) {
		return;
	}

	/* TODO: do something with argv[0] */

	switch (cmd[0]) {
	case 'i': cl_set_mode(peer, MODE_INTERFACE); break;
	case 'e': cl_set_mode(peer, MODE_EFFECT);    break;
	case 'l': cl_set_mode(peer, MODE_LINK);      break;
	}
}

static void
cmd_exit(struct cl_peer *peer, const char *cmd, int mode, int argc, char *argv[])
{
	assert(peer != NULL);
	assert(cmd != NULL);
	assert(0 == strcmp(cmd, "exit"));
	assert(argc >= 0);

	(void) cmd;
	(void) argv;

	if (argc != 0) {
		cl_printf(peer, "invalid cardinality");
		return;
	}

	/* XXX: i dislike this; we have mode knowledge scattered all over the place,
	 * where it clearly could be automated into a tree */
	switch (mode) {
	case MODE_INTERFACE: cl_set_mode(peer, MODE_CONFIGURE); break;
	case MODE_EFFECT:    cl_set_mode(peer, MODE_CONFIGURE); break;
	case MODE_LINK:      cl_set_mode(peer, MODE_CONFIGURE); break;
	case MODE_CONFIGURE: cl_set_mode(peer, MODE_ENABLED);   break;
	case MODE_ENABLED:   cl_set_mode(peer, MODE_DISABLED);  break;
	case MODE_DISABLED:  cl_close(peer);                    break;
	}
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

static int
printprompt(struct cl_peer *peer, int mode)
{
	const char *something = "TODO";	/* TODO: from model of efcli data */

	assert(peer != NULL);
/* TODO: assert(single bit in mode); */

	switch (mode) {
	case MODE_CONNECTED: return cl_printf(peer, ">");	/* XXX: not for pre-motd. make a MODE_MOTD? */
	case MODE_DISABLED:  return cl_printf(peer, ">");
	case MODE_ENABLED:   return cl_printf(peer, "#");
	case MODE_CONFIGURE: return cl_printf(peer, "config#");
	case MODE_INTERFACE: return cl_printf(peer, "config(%s)#", something);
	case MODE_LINK:      return cl_printf(peer, "config(%s)#", something);
	case MODE_EFFECT:    return cl_printf(peer, "config(%s)#", something);
	}

	/* UNREACHED */
	return -1;
}

const struct cl_field fields[] = {
	{ FIELD_USERNAME, "username", 1, validate_name },
	{ FIELD_PASSWORD, "password", 0, NULL          }
};

const struct cl_command commands[] = {
	{ "show motd",          MODE_CONNECTED, 0,                               cmd_motd,  NULL },
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

	{ "help", ~0U, 0, cmd_help, "usage" },

	/*
	 * The "exit" command would be under ~0U for all modes, but I wanted to show
	 * how to provide different usage text for one specific mode.
	 */
	{ "exit", ~MODE_DISABLED, 0, cmd_exit, "return to the previous mode" },
	{ "exit",  MODE_DISABLED, 0, cmd_exit, "disconnect"                  }
};

static void
addpeer(struct peer **peers, int fd, struct cl_peer *peer)
{
	struct peer *new;

	assert(fd != -1);
	assert(peer != NULL);
	assert(peers != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		perror("malloc");
		return;
	}

	new->fd   = fd;
	new->peer = peer;

	new->next = *peers;
	*peers = new;
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

int
main(int argc, char **argv)
{
	int s;
	in_addr_t a;
	in_port_t p;
	struct sockaddr_in sin;
	struct cl_tree *tree;

	tree = cl_create(sizeof tree, commands, sizeof fields, fields,
		printprompt, cl_visible);
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

		peers = NULL;

		FD_ZERO(&master);
		FD_SET(s, &master);

		maxfd = s;

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

				i = accept(s, NULL, 0);
				if (-1 == i) {
					perror ("accept");
					return 1;
				}

				FD_SET(i, &master);
				maxfd = MAX(maxfd, i);

				/* XXX: store peer where? linked list caller-sider? */
				peer = cl_accept(tree);
				if (peer == NULL) {
					perror ("cl_accept");
					return 1;
				}

				addpeer(&peers, i, peer);

				cl_set_mode(peer, MODE_CONNECTED);

				/* XXX: loop until everything is read? no, by contract a single
				 * command will be consumed per cl_read() call. no, can't do that;
				 * we might not *have* a full command. So this is pot luck... */
				cl_read(peer, "show motd\n", 10);
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

				if (!cl_read(peer, buf, (size_t) n)) {
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

