#ifndef COMMAND_H
#define COMMAND_H

int is_builtin(const char *cmd);
void handle_builtin(char **args);
void suggest_commands(const char *input);  // Optional, if you want to use it from main.c

#endif
