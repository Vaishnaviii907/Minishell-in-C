#ifndef PARSER_H
#define PARSER_H

#include "executor.h"  // brings in Command struct

int parse_pipeline(const char *input, Command **cmds, int *num_cmds);
void free_commands(Command *cmds, int num_cmds);
char **parse_input(char *input);

#endif

