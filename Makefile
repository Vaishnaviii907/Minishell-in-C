CC = gcc
CFLAGS = -Wall

all: minishell

minishell: myShell.c
	$(CC) $(CFLAGS) myShell.c -o minishell

clean:
	rm -f minishell
