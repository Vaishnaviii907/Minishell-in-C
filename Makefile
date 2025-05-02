CC      = gcc
CFLAGS  = -Wall -Werror
SOURCES = main.c builtins.c
OBJECTS = $(SOURCES:.c=.o)
EXEC    = minishell

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXEC)
