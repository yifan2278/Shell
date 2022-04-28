#build an executable named mysh from shell.c
all: shell.c
	gcc -g -Wall -o mysh shell.c

clean:
	$(RM) mysh
