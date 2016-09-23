# build an executable named myprog from myprog.c
all: p1.c
	gcc -g -Wall -O0 -o p1 p1.c
test: p1
	./p1
assembly: p1.c
	gcc -S -O0 p1.c -o p1.S
	cat p1.S
clean: 
	$(RM) p1 p1.S
cpp: p1.c
	gcc -E -nostdinc p1.c
