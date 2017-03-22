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

PKG += unibilium
PKG += termkey
PKG += libtelnet

# layout
SUBDIR += examples
SUBDIR += examples/advent
SUBDIR += examples/router
SUBDIR += src
SUBDIR += src/io
SUBDIR += pc

INCDIR += include

DIR += ${BUILD}/lib

.include <subdir.mk>
.include <pkgconf.mk>
.include <pc.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <so.mk>
.include <part.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

