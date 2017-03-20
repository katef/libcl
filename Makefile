.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test::
install:: all
clean::

# things to override
CC     ?= gcc
BUILD  ?= build
PREFIX ?= /usr/local

CFLAGS_UNIBILIUM != pkg-config unibilium --cflags
LIBS_UNIBILIUM   != pkg-config unibilium --libs
CFLAGS_TERMKEY   != pkg-config termkey   --cflags
LIBS_TERMKEY     != pkg-config termkey   --libs
CFLAGS_LIBTELNET != pkg-config libtelnet --cflags
LIBS_LIBTELNET   != pkg-config libtelnet --libs

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

