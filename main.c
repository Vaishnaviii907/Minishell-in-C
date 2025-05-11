#main.c
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

#include "parser.h"
#include "executor.h"

#define MAX_LINE 1024

/* Built-in command interface */
int is_builtin(const char *cmd);
void handle_builtin(char **args);

/* Colored prompt with current working directory */
char *get_input(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("minishell");
        return NULL;
    }

    const char *color_reset = "\033[0m";
    const char *color_blue = "\033[34m";
    const char *color_green = "\033[32m";

    char *prompt = malloc(strlen(cwd) + 30);
    if (!prompt) return NULL;

    snprintf(prompt, strlen(cwd) + 30, "%s%s %s> %s", color_blue, cwd, color_green, color_reset);
    char *buf = readline(prompt);
    free(prompt);

    if (!buf) return NULL;
    if (strlen(buf) > 0) {
        add_history(buf);  // Add command to history
    }
    return buf;
}

/* Parse input into tokens (quote & backslash aware) */
char **parse_input(char *input) {
    int bufsize = 64, pos = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *start, *tok;

    if (!tokens) return NULL;

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
        }
    }
    tokens[pos] = NULL;
    return tokens;
}

/* External command launcher */
void launch_external(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("minishell");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("minishell");
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
            perror("minishell");
            return;
        }
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }

    if (output_file) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        out_fd = open(output_file, flags, 0644);
        if (out_fd == -1) {
            perror("minishell");
            return;
        }
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    execvp(args[0], args);
    perror("minishell");
}

/* Main shell loop */
int main(void) {
    char *input;
    char **args;


    while (1) {
        input = get_input();
        if (!input) break;
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
                fprintf(stderr, "Failed to parse pipeline\n");
            }
        } else if (strchr(input, '>') || strchr(input, '<')) {
            pid_t pid = fork();
            if (pid == 0) {
                handle_redirection(input);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
            } else {
                perror("fork");
            }
        } else {
            // Process normal input (no redirection or piping)
            args = parse_input(input);
            if (args[0]) {
                if (is_builtin(args[0])) {
                    handle_builtin(args);
                } else {
                    launch_external(args);
                }
            }

            for (int i = 0; args[i]; i++)
                free(args[i]);
            free(args);
        }

        free(input);
    }
    printf("\n");
    return 0;
}
