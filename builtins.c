#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define MAX_PATH 1024

/* Return 1 if cmd is one of our built‑ins; 0 otherwise */
int is_builtin(const char *cmd) {
    return strcmp(cmd, "cd") == 0 ||
           strcmp(cmd, "exit") == 0 ||
           strcmp(cmd, "echo") == 0 ||
           strcmp(cmd, "pwd") == 0;
}

/* cd command */
static void handle_cd(char **args) {
    char cwd[MAX_PATH];
    const char *dir = args[1] ? args[1] : getenv("HOME");

    if (strcmp(dir, "-") == 0) {
        // "cd -" to go back to the previous directory
        if (getenv("OLDPWD") != NULL) {
            chdir(getenv("OLDPWD"));
            printf("%s\n", getenv("OLDPWD"));
        } else {
            fprintf(stderr, "minishell: OLDPWD not set\n");
        }
    } else {
        // Save the current directory as OLDPWD before changing to the new directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("OLDPWD", cwd, 1);  // Update OLDPWD environment variable
        }
        if (chdir(dir) != 0) {
            perror("minishell");
        }
    }
}

/* echo command */
static void handle_echo(char **args) {
    int i = 1;
    int newline = 1;
    if (args[1] && strcmp(args[1], "-n") == 0) {
        newline = 0;
        i = 2;
    }
    for (; args[i]; i++) {
        printf("%s", args[i]);
        if (args[i+1]) printf(" ");
    }
    if (newline) printf("\n");
}

/* pwd command */
static void handle_pwd(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s\n", cwd);
    } else {
        perror("minishell");
    }
}

/* exit command */
static void handle_exit(char **args) {
    int status = 0;
    if (args[1]) status = atoi(args[1]);
    exit(status);
}

/* Dispatch to the correct built‑in */
void handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        handle_cd(args);
    } else if (strcmp(args[0], "echo") == 0) {
        handle_echo(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        handle_pwd();
    } else if (strcmp(args[0], "exit") == 0) {
        handle_exit(args);
    }
}
