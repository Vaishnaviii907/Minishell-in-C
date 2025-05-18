 
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

    const char *color_reset = "\033[0m";
    const char *color_blue = "\033[34m";
    const char *color_green = "\033[32m";

    char *prompt = malloc(strlen(cwd) + 30);
    if (!prompt) {
        perror("Error allocating memory for prompt");
        return NULL;
    }

    snprintf(prompt, strlen(cwd) + 30, "%s%s %s> %s", color_blue, cwd, color_green, color_reset);
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
char **parse_input(char *input) {
    int bufsize = 64, pos = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *start, *tok;

    if (!tokens) {
        perror("Error allocating memory for tokens");
        return NULL;
    }

    while (*input) {
        while (*input && isspace((unsigned char)*input)) input++;
        if (!*input) break;

        if (*input == '"' || *input == '\'') {
            char quote = *input++;
            start = input;
            while (*input && *input != quote) {
                if (*input == '\\' && *(input + 1)) input += 2;
                else input++;
            }
            int len = input - start;
            tok = strndup(start, len);
            if (*input == quote) input++;
        } else {
            start = input;
            while (*input && !isspace((unsigned char)*input)) {
                if (*input == '\\' && *(input + 1)) input += 2;
                else input++;
            }
            int len = input - start;
            tok = malloc(len + 1);
            if (!tok) {
                perror("Error allocating memory for token");
                return NULL;
            }
            char *dst = tok, *src = start;
            while (src < input) {
                if (*src == '\\' && (src + 1) < input) {
                    *dst++ = *(src + 1);
                    src += 2;
                } else {
                    *dst++ = *src++;
                }
            }
            *dst = '\0';
        }

        tokens[pos++] = tok;
        if (pos >= bufsize) {
            bufsize *= 2;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                perror("Error reallocating memory for tokens");
                return NULL;
            }
        }
    }
    tokens[pos] = NULL;
    return tokens;
}

/* External command launcher with suggestion */
void launch_external(char **args) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "\033[1;31mCommand not found:\033[0m %s\n", args[0]);
        suggest_commands(args[0]);  // call external suggestion from command.c
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

/* Redirection handler */
void handle_redirection(char *input) {
    int in_fd = 0, out_fd = 0;
    char *args[100];
    char *output_file = NULL;
    char *input_file = NULL;
    int append = 0;

    char *token = strtok(input, " ");
    int i = 0;

    while (token != NULL) {
        if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            output_file = strtok(NULL, " ");
            append = (strcmp(token, ">>") == 0);
        } else if (strcmp(token, "<") == 0) {
            input_file = strtok(NULL, " ");
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }

    args[i] = NULL;

    if (input_file) {
        in_fd = open(input_file, O_RDONLY);
        if (in_fd == -1) {
            perror("Error opening input file for redirection");
            return;
        }
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("Error redirecting input");
            close(in_fd);
            return;
        }
        close(in_fd);
    }

    if (output_file) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        out_fd = open(output_file, flags, 0644);
        if (out_fd == -1) {
            perror("Error opening output file for redirection");
            return;
        }
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("Error redirecting output");
            close(out_fd);
            return;
        }
        close(out_fd);
    }

    execvp(args[0], args);
    perror("Error executing command after redirection");
}

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
