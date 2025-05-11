# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lreadline -lncurses

# Sources and object files
SRCS = main.c builtins.c parser.c executor.c
OBJS = $(SRCS:.c=.o)

# Output binary
TARGET = minishell

# Default target (build the project)
all: $(TARGET)

# Rule to create the target executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build (remove object files and the executable)
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild the project (clean and build)
rebuild: clean all

# Phony targets (these aren't files)
.PHONY: all clean rebuild
