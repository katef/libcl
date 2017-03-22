.include "../../share/mk/top.mk"

SRC += examples/advent/advent.c

PROG += advent

LFLAGS.advent += ${BUILD}/lib/libcl.a
LFLAGS.advent += ${LIBS.unibilium} # XXX: should be in -lcl
LFLAGS.advent += ${LIBS.libtelnet} # XXX: should be in -lcl
LFLAGS.advent += ${LIBS.termkey}   # XXX: should be in -lcl

.for lib in ${LIB:Mlibcl}
${BUILD}/bin/advent: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mexamples/advent/*.c}
${BUILD}/bin/advent: ${BUILD}/${src:R}.o
.endfor

