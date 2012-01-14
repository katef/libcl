/* $Id$ */

#ifndef LIBCL_TREE_H
#define LIBCL_TREE_H

#include <sys/types.h>

#include <stddef.h>
#include <stdarg.h>

struct cl_peer;
struct cl_tree;

struct cl_field {
	int id;
	const char *name;
	int echo;

	int (*validate)(struct cl_peer *p, int id, const char *value);
};

struct cl_command {
	const char *command;
	int modes;
	int fields;

	void (*callback)(struct cl_peer *p, const char *command, int mode,
		int argc, char *argv[]);

	const char *usage;
};

struct cl_tree *cl_create(size_t command_count, const struct cl_command commands[],
	size_t field_count, const struct cl_field fields[],
	int (*printprompt)(struct cl_peer *p, int mode),
	int (*visible)(struct cl_peer *p, int mode, const struct cl_command *command));
void cl_destroy(struct cl_tree *t);

struct cl_peer *cl_accept(struct cl_tree *t);
void cl_close(struct cl_peer *p);

const char *cl_get_field(struct cl_peer *p, int id);

int cl_printf(struct cl_peer *p, const char *fmt, ...);
int cl_vprintf(struct cl_peer *p, const char *fmt, va_list ap);

ssize_t cl_read(struct cl_peer *p, const void *data, size_t len);
void cl_set_mode(struct cl_peer *p, int mode);

void cl_again(struct cl_peer *p);

void cl_help(struct cl_peer *p, int mode);

int cl_visible(struct cl_peer *p, int mode, const struct cl_command *command);

#endif

