.include "../../share/mk/top.mk"

SRC += examples/router/router.c

PROG += router

LFLAGS_router += ${BUILD}/lib/libcl.a
LFLAGS_router += ${LIBS_unibilium} # XXX: should be in -lcl
LFLAGS_router += ${LIBS_libtelnet} # XXX: should be in -lcl
LFLAGS_router += ${LIBS_termkey}   # XXX: should be in -lcl

.for lib in ${LIB:Mlibcl}
${BUILD}/bin/router: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mexamples/router/*.c}
${BUILD}/bin/router: ${BUILD}/${src:R}.o
.endfor

