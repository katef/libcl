.include "../../share/mk/top.mk"

SRC += examples/advent/advent.c

PROG += advent

LFLAGS_advent += ${BUILD}/lib/libcl.a
LFLAGS_advent += ${LIBS_unibilium} # XXX: should be in -lcl
LFLAGS_advent += ${LIBS_libtelnet} # XXX: should be in -lcl
LFLAGS_advent += ${LIBS_termkey}   # XXX: should be in -lcl

.for lib in ${LIB:Mlibcl}
${BUILD}/bin/advent: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mexamples/advent/*.c}
${BUILD}/bin/advent: ${BUILD}/${src:R}.o
.endfor

