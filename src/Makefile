.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/include

SRCS+= cl.c
SRCS+= trie.c
SRCS+= field.c
SRCS+= term.c
SRCS+= read.c
SRCS+= edit.c
SRCS+= lexer.c

SRCS+= io/start.c
SRCS+= io/end.c
SRCS+= io/ecma48.c
SRCS+= io/telnet.c

.include <lx.obj.mk>


LIB= libcl
LIB_NS= cl_

.include <lx.lib.mk>

