// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include "parser.h"
#include "executor.h"


#define MAX_LINE 1024

/* Built‑in command interface */
int  is_builtin(const char *cmd);
void handle_builtin(char **args);

/* Read one line of input */
char *get_input(void) {
    char *buf = malloc(MAX_LINE);
    if (!buf) return NULL;
    printf("minishell> ");
    if (!fgets(buf, MAX_LINE, stdin)) {
        free(buf);
        return NULL;    // EOF (Ctrl+D)
    }
    buf[strcspn(buf, "\n")] = '\0';  // strip newline
    return buf;
}

/* Split input into tokens, honoring quotes and backslashes */
char **parse_input(char *input) {
    int   bufsize = 64, pos = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *start, *tok;
    
    if (!tokens) return NULL;
    while (*input) {
        /* Skip whitespace */
        while (*input && isspace((unsigned char)*input))
            input++;
        if (!*input) break;

        /* Quoted token? */
        if (*input == '"' || *input == '\'') {
            char quote = *input++;
            start = input;
            while (*input && *input != quote) {
                if (*input == '\\' && *(input+1)) input += 2;
                else input++;
            }
            int len = input - start;
            tok = strndup(start, len);
            if (*input == quote) input++;
        } else {
            /* Unquoted: read until next space, handling backslashes */
            start = input;
            while (*input && !isspace((unsigned char)*input)) {
                if (*input == '\\' && *(input+1)) input += 2;
                else input++;
            }
            int len = input - start;
            tok = malloc(len + 1);
            char *dst = tok, *src = start;
            while (src < input) {
                if (*src == '\\' && (src+1) < input) {
                    *dst++ = *(src+1);
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

/* Fork & exec an external command */
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
// main.c
void execute_pipeline(Command *cmds, int num_cmds);

/* Handle input/output redirection */
void handle_redirection(char *input) {
    int in_fd = 0, out_fd = 0;
    char *args[100]; // Array to store command arguments
    char *output_file = NULL;
    char *input_file = NULL;
    int append = 0;
    
    // Split input by spaces
    char *token = strtok(input, " ");
    int i = 0;
    
    while (token != NULL) {
        // Check for output redirection
        if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            output_file = strtok(NULL, " ");
            append = (strcmp(token, ">>") == 0); // Handle append mode
        }
        // Check for input redirection
        else if (strcmp(token, "<") == 0) {
            input_file = strtok(NULL, " ");
        }
        else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    
    args[i] = NULL;

    // Handle input redirection
    if (input_file) {
        in_fd = open(input_file, O_RDONLY);
        if (in_fd == -1) {
            perror("minishell");
            return;
        }
        dup2(in_fd, STDIN_FILENO);  // Redirect stdin to the file
        close(in_fd);
    }

    // Handle output redirection
    if (output_file) {
        int flags = O_WRONLY | O_CREAT | O_TRUNC;
        if (append) {
            flags = O_WRONLY | O_CREAT | O_APPEND;
        }
        out_fd = open(output_file, flags, 0644);
        if (out_fd == -1) {
            perror("minishell");
            return;
        }
        dup2(out_fd, STDOUT_FILENO);  // Redirect stdout to the file
        close(out_fd);
    }

    // Execute the command
    execvp(args[0], args);
    perror("minishell");
}

int main(void) {
    char *input;
    char **args;

    while (1) {
        input = get_input();
        if (!input) break;              // EOF
        if (strlen(input) == 0) {       // empty line
            free(input);
            continue;
        }

       // Check for pipes first
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
            exit(EXIT_FAILURE);  // only if exec fails
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    } else {
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
