#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

#define MAX_ARGS 64
#define MAX_CMDS 10

// Helper: skip whitespace
static char *skip_spaces(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

// Helper: split command string into argv[], handling quotes
static char **split_args(char *cmd_str, int *argc_out) {
    char **argv = malloc(sizeof(char*) * MAX_ARGS);
    if (!argv) {
        perror("malloc");
        return NULL;
    }
    int argc = 0;
    char *s = cmd_str;

    while (*s) {
        s = skip_spaces(s);
        if (*s == '\0') break;

        char *start;
        char *arg;
        if (*s == '"' || *s == '\'') {
            char quote = *s++;
            start = s;
            while (*s && *s != quote) {
                if (*s == '\\' && s[1]) s += 2;
                else s++;
            }
            int len = s - start;
            arg = strndup(start, len);
            if (*s == quote) s++;
        } else {
            start = s;
            while (*s && !isspace((unsigned char)*s) && *s != '<' && *s != '>' && *s != '|')
                s++;
            int len = s - start;
            arg = strndup(start, len);
        }
        argv[argc++] = arg;
    }
    argv[argc] = NULL;
    if (argc_out) *argc_out = argc;
    return argv;
}

// Main parser function
int parse_pipeline(const char *input, Command **cmds_out, int *num_cmds_out) {
    char *input_copy = strdup(input);
    if (!input_copy) {
        perror("strdup");
        return -1;
    }
    char *segment;
    char *saveptr;
    int num_cmds = 0;
    Command *cmds = malloc(sizeof(Command) * MAX_CMDS);
    if (!cmds) {
        perror("malloc");
        free(input_copy);
        return -1;
    }

    segment = strtok_r(input_copy, "|", &saveptr);
    while (segment && num_cmds < MAX_CMDS) {
        char *cmd_str = strdup(segment);
        if (!cmd_str) {
            perror("strdup");
            free(input_copy);
            free(cmds);
            return -1;
        }
        Command *cmd = &cmds[num_cmds];
        cmd->input_file = NULL;
        cmd->output_file = NULL;
        cmd->append = 0;

        // Parse redirection tokens manually
        char *redir = cmd_str;
        char *args_buf = malloc(strlen(cmd_str) + 1);
        if (!args_buf) {
            perror("malloc");
            free(cmd_str);
            free(input_copy);
            free(cmds);
            return -1;
        }
        args_buf[0] = '\0';

        while (*redir) {
            if (*redir == '<') {
                *redir++ = '\0';
                redir = skip_spaces(redir);
                char *file = strtok(redir, " \t");
                cmd->input_file = strdup(file);
                redir += strlen(file);
            } else if (*redir == '>') {
                int append = 0;
                *redir++ = '\0';
                if (*redir == '>') {
                    append = 1;
                    *redir++ = '\0';
                }
                redir = skip_spaces(redir);
                char *file = strtok(redir, " \t");
                if (!file) {
                    fprintf(stderr, "minishell: syntax error: expected file after >\n");
                    free(args_buf);
                    free(cmd_str);
                    free(input_copy);
                    free(cmds);
                    return -1;
                }
                cmd->output_file = strdup(file);
                cmd->append = append;
                redir += strlen(file);
            } else {
                strncat(args_buf, redir, 1);
                redir++;
            }
        }

        int argc = 0;
        cmd->argv = split_args(args_buf, &argc);
        free(args_buf);
        free(cmd_str);

        num_cmds++;
        segment = strtok_r(NULL, "|", &saveptr);
    }

    free(input_copy);
    *cmds_out = cmds;
    *num_cmds_out = num_cmds;
    return 0;
}

// Free command list
void free_commands(Command *cmds, int num_cmds) {
    for (int i = 0; i < num_cmds; ++i) {
        if (cmds[i].argv) {
            for (int j = 0; cmds[i].argv[j]; ++j) {
                free(cmds[i].argv[j]);  // Free each argument string
            }
            free(cmds[i].argv);          // Free argv array
        }

        free(cmds[i].input_file);        // Only if you duplicated or malloc'd it
        free(cmds[i].output_file);       // Same here
    }
    free(cmds);                          // Finally free the array of Command structs
}
