#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "suggest.h"  
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "custom.h"


#define MAX_LINE 1024

/* Colored prompt with current working directory */
char *get_input(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        return NULL;
    }

    char *prompt;
    if (!getenv("MINISHELL_GUI")) {
        const char *color_reset = "\033[0m";
        const char *color_blue = "\033[34m";
        const char *color_green = "\033[32m";
        prompt = malloc(strlen(cwd) + 30);
        snprintf(prompt, strlen(cwd) + 30, "%s%s %s> %s", color_blue, cwd, color_green, color_reset);
    } else {
        prompt = malloc(strlen(cwd) + 10);
        snprintf(prompt, strlen(cwd) + 10, "%s> ", cwd);
    }

    char *buf = readline(prompt);
    free(prompt);

    if (!buf) {
        perror("Error reading input");
        return NULL;
    }

    if (strlen(buf) > 0) {
        add_history(buf);
    }
    return buf;
}

/* Parse input into tokens (quote & backslash aware) */

/* External command launcher with suggestion */


/* Redirection handler */

/* Main shell loop */
int main(void) {
    char *input;
    char **args;

    while (1) {
        input = get_input();
        if (!input) {
            fprintf(stderr, "Error getting input, exiting.\n");
            break;
        }

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        if (strchr(input, '|')) {
            Command *cmds;
            int num_cmds;

            if (parse_pipeline(input, &cmds, &num_cmds) == 0) {
                execute_pipeline(cmds, num_cmds);
                free_commands(cmds, num_cmds);
            } else {
                fprintf(stderr, "Error parsing pipeline.\n");
            }
        } else if (strchr(input, '>') || strchr(input, '<')) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("Error forking process for redirection");
                free(input);
                continue;
            }
            if (pid == 0) {
                handle_redirection(input);
                exit(EXIT_FAILURE);
            } else {
                waitpid(pid, NULL, 0);
            }
        } else {
            args = parse_input(input);
            if (args[0]) {
                if (is_builtin(args[0])) {
    handle_builtin(args);
} else if (is_custom_command(args[0])) {
    handle_custom_command(args);  // <-- Call your custom command dispatcher
} else {
    launch_external(args);
}

            }
            for (int i = 0; args[i]; i++) free(args[i]);
            free(args);
        }

        free(input);
    }

    printf("\nExiting shell...\n");
    return 0;
}
