all: ish
ish: ish.o lex.yy.o
	gcc -o ish ish.o lex.yy.o -lfl  
ish.o: ish.c
	gcc -Wall -g -c ish.c
lex.yy.o: lex.yy.c
	gcc -Wall -g -c lex.yy.c
lex.yy.c: lex.c
	flex lex.c
clean:
	rm -f *.o
	rm -f lex.yy.c
	rm -f ish
	