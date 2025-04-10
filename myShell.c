#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("minishell> ");
        fgets(input, MAX_INPUT, stdin);

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Exit shell
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Fork a child process
        pid_t pid = fork();

        if (pid == 0) {
            // In child
            char *args[] = {input, NULL};
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            // In parent
            wait(NULL);
        } else {
            perror("fork failed");
        }
    }

    return 0;
}
