compileAll:
	gcc mainConcorrente.c -o mainConcorrente -pthread -lm -Wall
	gcc mainConcorrenteBuffer.c Parser.c -o mainConcorrenteBuffer -pthread -lm -Wall
	gcc mainSequencial.c Parser.c -o mainSequencial -pthread -lm -Wall