# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/include

SRCS+= router.c

.include <lx.obj.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src

LIBS+= -lcl
LIBS+= -ltermkey
LIBS+= -ltelnet
LIBS+= -lunibilium

.include <lx.prog.mk>

