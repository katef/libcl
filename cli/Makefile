# $Id$

CFLAGS=	-std=c89 -pedantic -Wall

a.out: char_term.o char.o lexer.o main.o
	$(CC) $(CFLAGS) -o $@ char_term.o char.o lexer.o main.o

clean:
	rm a.out char_term.o char.o lexer.o main.o

