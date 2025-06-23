# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g -MMD -MP
LDFLAGS = -lreadline -lncurses
GTKFLAGS = `pkg-config --cflags --libs gtk+-3.0`

# Sources and object files for terminal version
SRCS = main.c builtins.c parser.c executor.c suggest.c custom.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Sources and object files for GUI version
GUI_SRCS = gui.c shell_core.c shell_interface.c builtins.c custom.c executor.c parser.c suggest.c
GUI_OBJS = $(GUI_SRCS:.c=.o)
GUI_DEPS = $(GUI_OBJS:.o=.d)

# Output binaries
TARGET = minishell
gui_TARGET = gui

# Default target (build both versions)
all: $(TARGET) $(gui_TARGET)

# Terminal version
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# GUI version
$(gui_TARGET): $(GUI_OBJS)
	$(CC) $(GUI_OBJS) -o $(gui_TARGET) $(GTKFLAGS)

# Rule to compile .c files into .o files and generate dependencies
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

gui.o: gui.c
	$(CC) $(CFLAGS) $(GTKFLAGS) -c gui.c -o gui.o

shell_core.o: shell_core.c
	$(CC) $(CFLAGS) $(GTKFLAGS) -c shell_core.c -o shell_core.o

shell_interface.o: shell_interface.c
	$(CC) $(CFLAGS) $(GTKFLAGS) -c shell_interface.c -o shell_interface.o

# Include dependency files if they exist
-include $(DEPS)
-include $(GUI_DEPS)

# Clean up the build (remove object files, dependency files, and the executables)
clean:
	rm -f $(OBJS) $(DEPS) $(GUI_OBJS) $(GUI_DEPS) $(TARGET) $(gui_TARGET)

# Rebuild the project (clean and build)
rebuild: clean all

# Phony targets (these aren't files)
.PHONY: all clean rebuild
