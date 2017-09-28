.include "../share/mk/top.mk"

SRC += src/cl.c
SRC += src/trie.c
SRC += src/field.c
SRC += src/term.c
SRC += src/read.c
SRC += src/edit.c
SRC += src/lexer.c

CFLAGS.src/term.c += ${CFLAGS.unibilium}
DFLAGS.src/term.c += ${CFLAGS.unibilium}

LIB        += libcl
SYMS.libcl += src/libcl.syms

.for src in ${SRC:Msrc/*.c}
${BUILD}/lib/libcl.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libcl.opic: ${BUILD}/${src:R}.opic
.endfor

