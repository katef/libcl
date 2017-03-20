.include "../../share/mk/top.mk"

SRC += examples/router/router.c

PROG += router

LFLAGS_router += ${BUILD}/lib/libcl.a
LFLAGS_router += ${LIBS_UNIBILIUM} # XXX: should be in -lcl
LFLAGS_router += ${LIBS_LIBTELNET} # XXX: should be in -lcl
LFLAGS_router += ${LIBS_TERMKEY}   # XXX: should be in -lcl

.for lib in ${LIB:Mlibcl}
${BUILD}/bin/router: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mexamples/router/*.c}
${BUILD}/bin/router: ${BUILD}/${src:R}.o
.endfor

