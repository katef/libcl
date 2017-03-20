.include "../../share/mk/top.mk"

SRC += src/io/chain.c
SRC += src/io/start.c
SRC += src/io/end.c
SRC += src/io/ecma48.c
SRC += src/io/telnet.c

CFLAGS_src/io/ecma48.c += ${CFLAGS_TERMKEY}
DFLAGS_src/io/ecma48.c += ${CFLAGS_TERMKEY}

CFLAGS_src/io/telnet.c += ${CFLAGS_LIBTELNET}
DFLAGS_src/io/telnet.c += ${CFLAGS_LIBTELNET}

.for src in ${SRC:Msrc/io/*.c}
${BUILD}/lib/libcl.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libcl.opic: ${BUILD}/${src:R}.opic
.endfor

