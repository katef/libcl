/* $Id$ */

/*
 * TODO: LICENCE
 */

#ifndef libcl 
#define libcl

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
	int  mode;
	int  field;

	void (*callback)(struct cl_peer *p, const char *command, int mode, int argc, char *argv[]),
};

struct cl_tree * cl_create(size_t command_count, struct cl_command commands[],
	size_t field_count, struct cl_field fields[]);
void cl_destroy(struct cl_tree *t);

struct cl_peer * cl_accept(struct cl_tree *t);
void cl_close(struct cl_peer *p);

const char* cl_get_field(cl_peer *p, int id);

void cl_printf(struct cl_peer *p, const char *fmt, ...);
void cl_vprintf(struct cl_peer *p, const char *fmt, va_list ap);

ssize_t cl_read(struct cl_peer *p, size_t len, const void *data);
void cl_set_mode(struct cl_peer *p, int mode);
void cl_again(struct cl_peer *p);

#endif

