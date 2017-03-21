.include "../share/mk/top.mk"

SRC += src/cl.c
SRC += src/trie.c
SRC += src/field.c
SRC += src/term.c
SRC += src/read.c
SRC += src/edit.c
SRC += src/lexer.c

CFLAGS_src/term.c += ${CFLAGS_unibilium}
DFLAGS_src/term.c += ${CFLAGS_unibilium}

LIB += libcl

.for src in ${SRC:Msrc/*.c}
${BUILD}/lib/libcl.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libcl.opic: ${BUILD}/${src:R}.opic
.endfor

