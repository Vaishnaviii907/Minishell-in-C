#include "shell_core.h"
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "custom.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void minishell_run_command(const char *input) {
    if (!input || strlen(input) == 0) return;

    if (strchr(input, '|')) {
        Command *cmds;
        int num_cmds;
        if (parse_pipeline((char *)input, &cmds, &num_cmds) == 0) {
            execute_pipeline(cmds, num_cmds);
            free_commands(cmds, num_cmds);
        } else {
            fprintf(stderr, "Error parsing pipeline.\n");
        }
    } else if (strchr(input, '>') || strchr(input, '<')) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error forking process for redirection");
            return;
        }
        if (pid == 0) {
            handle_redirection((char *)input);
            exit(EXIT_FAILURE);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        char **args = parse_input((char *)input);
        if (args && args[0]) {
            if (is_builtin(args[0])) {
                handle_builtin(args);
            } else if (is_custom_command(args[0])) {
                handle_custom_command(args);
            } else {
                launch_external(args);
            }
        }
        if (args) {
            for (int i = 0; args[i]; i++) free(args[i]);
            free(args);
        }
    }
} 