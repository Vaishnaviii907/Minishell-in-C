#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include "parser.h"
#include "suggest.h"

void execute_pipeline(Command *cmds, int num_cmds) {
    int i;
    int in_fd = -1;
    int pipe_fd[2];
    int prev_fd = -1;

    for (i = 0; i < num_cmds; i++) {
        // Set up pipe for this command (except last)
        if (i < num_cmds - 1) {
            if (pipe(pipe_fd) < 0) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            // CHILD PROCESS

            // Input redirection from file
            if (cmds[i].input_file) {
                in_fd = open(cmds[i].input_file, O_RDONLY);
                if (in_fd < 0) {
                    perror("open input");
                    exit(1);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Output redirection to file
            if (cmds[i].output_file) {
                int flags = O_WRONLY | O_CREAT;
                flags |= cmds[i].append ? O_APPEND : O_TRUNC;
                int out_fd = open(cmds[i].output_file, flags, 0644);
                if (out_fd < 0) {
                    perror("open output");
                    exit(1);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            // If there's a previous pipe, read from it
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            // If there's a next pipe, write to it
            if (i < num_cmds - 1) {
                close(pipe_fd[0]);               // Close read end
                dup2(pipe_fd[1], STDOUT_FILENO); // Replace stdout
                close(pipe_fd[1]);
            }

            // Execute the command
            execvp(cmds[i].argv[0], cmds[i].argv);
            perror("execvp");
            exit(1);  // Ensure child process exits if execvp fails
        } else if (pid < 0) {
            perror("fork");
            return;
        }

        // PARENT PROCESS
        if (prev_fd != -1)
            close(prev_fd);
        if (i < num_cmds - 1) {
            close(pipe_fd[1]);      // Close write end in parent
            prev_fd = pipe_fd[0];   // Next child reads from this
        }
    }

    // Wait for all children to finish
    for (i = 0; i < num_cmds; i++) {
        wait(NULL);  // Wait for any child process
    }
}
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
void launch_external(char **args) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        if (!getenv("MINISHELL_GUI")) {
            fprintf(stderr, "\033[1;31mCommand not found:\033[0m %s\n", args[0]);
        } else {
            fprintf(stderr, "Command not found: %s\n", args[0]);
        }
        suggest_commands(args[0]);  // call external suggestion from command.c
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}
