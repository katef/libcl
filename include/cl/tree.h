/* $Id$ */

#ifndef LIBCL_TREE_H
#define LIBCL_TREE_H

#include <sys/types.h>

#include <stddef.h>
#include <stdarg.h>

/*
 * I/O protocol. This informs cl_read()'s parsing of bytes passed in.
 *
 *  CL_PLAIN  - Data is taken as-is, with no special parsing. Command line
 *              navigation will not be provided.
 *  CL_ECMA48 - CSI sequences as used by terminals to encode special keys;
 *              This is typical for stdin connected to a terminal.
 *  CL_TELNET - The telnet protocol. Typically used for TCP sockets.
 */
enum cl_io {
	CL_PLAIN,
	CL_ECMA48,
	CL_TELNET
};

struct cl_peer;
struct cl_tree;

/*
 * Specification of a single field. Each field has a unique ID, which must be a
 * power of two, as field IDs are masked together. The user is prompted to enter
 * a value for each field when required.
 *
 *  id       - A single-bit unique ID. e.g. (1 << 3)
 *  name     - A human-readable name for the field. This is used to prompt the user.
 *             e.g. "password".
 *  echo     - true if the user's input is to be visible as it is entered.
 *  validate - Returns true if the given value is permissable. If not, the user
 *             will be prompted again, until .validate() returns true.
 */
struct cl_field {
	int id;
	const char *name;
	int echo;

	/*
	 *  p     - The peer responsible for instantiating this call.
	 *  id    - The field ID for which the given value is to be validated.
	 *  value - A string of the user-given value, given verbatim as entered
	 *          by the user. This includes preceding and trailing whitespace.
	 *          Storage persists until validate() returns.
	 */
	int (*validate)(struct cl_peer *p, int id, const char *value);
};

/*
 * Specification of a command. Each command is a list of space-seperated words
 * which collectively form paths within a tree.
 *
 *  command  - List of words. Each word is seperated by a single space, ' '.
 *             Non-space characters are taken to be part of a word.
 *             The list may not begin or end with a space.
 *  modes    - A mask of user-defined modes for which this command is available.
 *  fields   - A mask of user-defined fields for which the user is prompted on
 *             execution of this command. Each bit within this mask corresponds
 *             to a unique field ID as per struct cl_field.
 *  callback - The function to call when the command is executed.
 *  usage    - A user-facing description. This is displayed by cl_help(). May be
 *             NULL if not required.
 *
 * A command may be specified multiple times, each with different
 * (non-overlapping) sets of modes. Each instance of the same command is
 * required to have the same set of fields and callback. This permits different
 * usage texts for each set of modes. For example:
 *
 *  { "exit", 1 << 0, 0, cmd_exit, "return to previous mode"    },
 *  { "exit", 1 << 1, 0, cmd_exit, "disconnect from the server" }
 */
struct cl_command {
	const char *command;
	int modes;
	int fields;

	/*
	 *  p       - The peer responsible for instantiating this call.
	 *  command - A string of the command executed. This is given in canonical
	 *            form, as specified by struct cl_command (i.e. it may differ
	 *            from exactly what the user entered).
	 *            Storage persists until validate() returns.
	 *  mode    - The mode under which this command is currently being executed.
	 *            This is a single bit from the mask specified by struct cl_command.
	 *  argc    - Argument count. This is the number of elements populated in argv.
	 *  argv    - Argument vector. Each element is a string of a user-given argument
	 *            following the command. These are vervatim as the user entered,
	 *            but with whitespace trimmed.
 	 */
	void (*callback)(struct cl_peer *p, const char *command, int mode,
		int argc, char *argv[]);

	const char *usage;
};

/*
 * Construct a command tree. A command tree is returned, or NULL on error.
 * A command tree is active until destroyed by cl_destroy().
 *
 *  command_count - The number of elements present in commands[].
 *  commands      - Command specifications from which the tree will be formed.
 *  field_count   - The number of elements present in fields[].
 *  fields        - Field specifications from which the tree will be formed.
 *
 * Various callbacks are given, which are called by the library when it needs to
 * do certian things. These are:
 *
 *  printprompt - To print the user's prompt. Typically this function will call
 *                cl_printf() in order to print to the user. The current mode
 *                is passed; this permits varying prompts per mode.
 *                Returns the number of characters printed, or -1 on error.
 *  visible     - Returns true if mode is present within the mask of modes.
 *  vprintf     - Called to perform the gruntwork of printing to the given peer.
 *                Typically this will involve application-specific data
 *                retrieved by cl_get_opaque(). Returns the number of bytes
 *                printed, or -1 on error.
 *
 * Storage for the commands and fields arrays is required to persist until a
 * call to cl_destroy().
 */
struct cl_tree *cl_create(size_t command_count, const struct cl_command commands[],
	size_t field_count, const struct cl_field fields[],
	int (*printprompt)(struct cl_peer *p, int mode),
	int (*visible)(struct cl_peer *p, int mode, int modes),
	int (*vprintf)(struct cl_peer *p, const char *fmt, va_list ap));
void cl_destroy(struct cl_tree *t);

/*
 * Accept a new peer. This is an analogue of POSIX's accept(2) on a listening
 * socket. A new peer instance is returned, or NULL on error.
 *
 * A peer is active until closed by cl_close().
 *
 * Each peer carries its own independent state for its user interface, including
 * its mode and prompt.
 *
 *  t  - The command tree for which the new peer will execute commands.
 *       The command tree is required to persist until a call to cl_close().
 *  io - The protocol by which user input is given. See enum cl_io for details.
 */
struct cl_peer *cl_accept(struct cl_tree *t, enum cl_io io);
void cl_close(struct cl_peer *p);

/*
 * Associate and retrieve application-specific data with a peer. This pointer is
 * passed opaquely through the API. It is intended to be retrieved within
 * callbacks, should an application require its own data per peer.
 */
void cl_set_opaque(struct cl_peer *p, void *opaque);
void *cl_get_opaque(struct cl_peer *p);

/*
 * Retrieve a field value.
 *
 * This function may only be called from within a cl_command callback function.
 *
 * Returns a string of the user-given value, given verbatim as entered
 * by the user. This includes preceding and trailing whitespace.
 * Storage persists until the next call to cl_get_field.
 *
 *  p  - The peer responsible for prompting for this field.
 *  id - The field ID for which the given value is to be validated.
 */
const char *cl_get_field(struct cl_peer *p, int id);

/*
 * Print to the user.
 *
 * This will cause an arbitary number of calls (possibly none) to the vprintf
 * callback passed to cl_create().
 *
 * Returns the number of bytes successfully printed, or -1 on error.
 *
 *  p        - The peer to which to print.
 *  fmt, ... - A format string and variadic arguments as per printf(3).
 */
int cl_printf(struct cl_peer *p, const char *fmt, ...);
int cl_vprintf(struct cl_peer *p, const char *fmt, va_list ap);

/*
 * Pass a sequence of incoming bytes from the user to libcl for reading.
 * The bytes passed are an arbitary sequence (i.e. not neccessarily a
 * null-terminated string), in the spirit of read(2).
 *
 * Returns the number of bytes consumed, or -1 on error.
 *
 * If enough data is given to complete a command, cl_read() guarentees to
 * consume the entire byte sequence representing that command.
 *
 *  p    - The peer responsible for instantiating this call.
 *  data - A pointer to the first byte of a sequence of len bytes to consume.
 *  len  - The number of bytes available. This must be at least 1.
 *
 * Storage for data need not persist after cl_read() returns.
 */
ssize_t cl_read(struct cl_peer *p, const void *data, size_t len);

/*
 * Change the current mode for a given peer.
 *
 * Modes are application-specific values, which are masked together, as in
 * struct cl_command. These serve to provide a mechanism for conditionally-
 * visible commands and a prompt which varies according to the user's position
 * in the command tree.
 *
 *  p    - The peer for which to change mode.
 *  mode - An application-specific mode. This has no particular meaning to
 *         libcl, other than to be passed to various callbacks.
 */
void cl_set_mode(struct cl_peer *p, int mode);

/*
 * Re-call the current command callback, as if the same command were executed
 * again by the user. The command callback will be called after it returns.
 *
 * Retrieve a field value. This function may only be called from within a
 * cl_command callback function.
 *
 *  p - The peer responsible for instantiating this call.
 */
void cl_again(struct cl_peer *p);

/*
 * Display command usage to the user.
 *
 * This will cause an arbitary number of calls (possibly none) to the vprintf
 * callback passed to cl_create().
 *
 * Help may be displayed for a mode other than the peer's current mode. This is
 * intended to provide a mechanism for requesting help on specific topics, e.g.
 * by way of a "help such-and-such" command.
 *
 *  p    - The peer responsible for instantiating this call.
 *  mode - An application-specific mode.
 */
void cl_help(struct cl_peer *p, int mode);

/*
 * Returns true if the given single-bit mode is within a set of modes masked
 * together. This function is provided for convenience, suitable for use as
 * cl_create()'s visible() callback. See cl_create() for details.
 */
int cl_visible(struct cl_peer *p, int mode, int modes);

#endif

