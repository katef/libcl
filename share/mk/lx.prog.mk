.if !defined(OBJS)
.BEGIN::
	@${ECHO} '$${OBJS} must be set' >&2
	@${EXIT} 1
.endif

.if !defined(PROG)
.BEGIN::
	@${ECHO} '$${PROG} must be set' >&2
	@${EXIT} 1
.endif

.for obj in ${OBJS}
${OBJ_SDIR}/${PROG}: ${OBJ_SDIR}/${obj}
.endfor

${OBJ_SDIR}/${PROG}:
	@${MKDIR} ${OBJ_SDIR}
	${CC} -o ${.TARGET} ${LDFLAGS} ${.ALLSRC} ${LIBS}

all:: ${OBJ_SDIR}/${PROG}


CLEAN+= ${OBJ_SDIR}/${PROG}

.include <lx.clean.mk>

