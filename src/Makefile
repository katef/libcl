# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/include

SRCS+= cl.c
SRCS+= trie.c
SRCS+= field.c
SRCS+= term.c

SRCS+= start.c
SRCS+= end.c
SRCS+= read.c
SRCS+= ecma48.c
SRCS+= telnet.c

.include <lx.obj.mk>


LIB= libcl
LIB_NS= cl_

.include <lx.lib.mk>

