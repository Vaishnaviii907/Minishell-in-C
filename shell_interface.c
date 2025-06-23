#include "shell_interface.h"
#include "shell_core.h"
   #include <sys/wait.h>
   #include "executor.h"
   #include "parser.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int minishell_execute(const char *cmd, char *output, size_t out_size) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        snprintf(output, out_size, "Error creating pipe\n");
        return -1;
    }

    int stdout_save = dup(STDOUT_FILENO);
    int stderr_save = dup(STDERR_FILENO);

    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    // Call your shell logic for a single command
    minishell_run_command(cmd);

    fflush(stdout);
    fflush(stderr);

    dup2(stdout_save, STDOUT_FILENO);
    dup2(stderr_save, STDERR_FILENO);
    close(stdout_save);
    close(stderr_save);

    ssize_t n = read(pipefd[0], output, out_size - 1);
    if (n >= 0) output[n] = '\0';
    else output[0] = '\0';
    close(pipefd[0]);

    return 0;
}
