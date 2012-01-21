# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/include

SRCS+= cl.c
SRCS+= trie.c
SRCS+= field.c

SRCS+= read.c
SRCS+= plain.c
SRCS+= ecma48.c
SRCS+= telnet.c

.include <lx.obj.mk>


LIB= libcl
LIB_NS= cl_

.include <lx.lib.mk>

