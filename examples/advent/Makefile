# $Id: Makefile 17 2012-01-14 23:01:14Z kate $

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/include

SRCS+= advent.c

.include <lx.obj.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src

LIBS+= -lcl
LIBS+= -ltermkey
LIBS+= -ltelnet	# XXX: don't like this here
LIBS+= -lunibilium

.include <lx.prog.mk>

