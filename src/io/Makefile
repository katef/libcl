.include "../../share/mk/top.mk"

SRC += src/io/chain.c
SRC += src/io/start.c
SRC += src/io/end.c
SRC += src/io/ecma48.c
SRC += src/io/telnet.c

CFLAGS.src/io/ecma48.c += ${CFLAGS.termkey}
DFLAGS.src/io/ecma48.c += ${CFLAGS.termkey}

CFLAGS.src/io/telnet.c += ${CFLAGS.libtelnet}
DFLAGS.src/io/telnet.c += ${CFLAGS.libtelnet}

.for src in ${SRC:Msrc/io/*.c}
${BUILD}/lib/libcl.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libcl.opic: ${BUILD}/${src:R}.opic
.endfor

