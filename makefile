all: interpreter gifmaker gifmaker2
	
interpreter:	interpreter.o parse_util.o matmulti.o mat4.o
	gcc interpreter.o parse_util.o matmulti.o mat4.o -o interpreter -lm -Wall
	
interpreter.o:	interpreter.c
	gcc -c interpreter.c -lm -Wall
	
parse_util.o: parse_util.c
	gcc -c parse_util.c -Wall
	
matmulti.o: 	matmulti.c
	gcc -c matmulti.c -Wall
	
mat4.o: 	mat4.c
	gcc -c mat4.c -Wall
	
gifmaker:	gifmaker.c
	gcc gifmaker.c -o gifmaker
	
gifmaker2:	gifmaker2.c
	gcc gifmaker2.c -o gifmaker2
