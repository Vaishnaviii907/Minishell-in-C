#include <stddef.h>
#ifndef SHELL_INTERFACE_H
#define SHELL_INTERFACE_H

int minishell_execute(const char *cmd, char *output, size_t out_size);

#endif