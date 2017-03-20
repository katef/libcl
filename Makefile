.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test::
install:: all
clean::

# things to override
CC      ?= gcc
BUILD   ?= build
PREFIX  ?= /usr/local
PKGCONF ?= pkgconf # or pkg-config

CFLAGS_UNIBILIUM != ${PKGCONF} unibilium --cflags
LIBS_UNIBILIUM   != ${PKGCONF} unibilium --libs
CFLAGS_TERMKEY   != ${PKGCONF} termkey   --cflags
LIBS_TERMKEY     != ${PKGCONF} termkey   --libs
CFLAGS_LIBTELNET != ${PKGCONF} libtelnet --cflags
LIBS_LIBTELNET   != ${PKGCONF} libtelnet --libs

# layout
SUBDIR += examples
SUBDIR += examples/advent
SUBDIR += examples/router
SUBDIR += src
SUBDIR += src/io

INCDIR += include

DIR += ${BUILD}/lib

.include <subdir.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <so.mk>
.include <part.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

