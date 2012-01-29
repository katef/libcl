# $Id: Makefile.inc 17 2012-01-14 23:01:14Z kate $
# generic Makefile.inc

.if defined(_BASEDIR_RELATIVE)
_BASEDIR_RELATIVE:=	${_BASEDIR_RELATIVE}/..
.else
_BASEDIR_RELATIVE:=	..
.endif

.include "../Makefile.inc"

