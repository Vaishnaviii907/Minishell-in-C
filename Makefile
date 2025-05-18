# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g -MMD -MP
LDFLAGS = -lreadline -lncurses

# Sources and object files
SRCS = main.c builtins.c parser.c executor.c suggest.c custom.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Output binary
TARGET = minishell

# Default target (build the project)
all: $(TARGET)

# Rule to create the target executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule to compile .c files into .o files and generate dependencies
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Include dependency files if they exist
-include $(DEPS)

# Clean up the build (remove object files, dependency files, and the executable)
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

# Rebuild the project (clean and build)
rebuild: clean all

# Phony targets (these aren't files)
.PHONY: all clean rebuil
