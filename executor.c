#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "parser.h"

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

            // If there’s a previous pipe, read from it
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            // If there’s a next pipe, write to it
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
