#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "custom.h"

void greet_command() {
    printf("👋 Hello! Welcome to MiniShell. Type 'help' to see available commands.\n");
}

void clr_command() {
    // ANSI escape code to clear screen
    printf("\033[H\033[J");
}

void calculator_command() {
    char op;
    double a, b;

    printf("Enter an expression (e.g., 4 + 5): ");
    if (scanf("%lf %c %lf", &a, &op, &b) != 3) {
        printf("Invalid input format.\n");
        while (getchar() != '\n'); // clear input buffer
        return;
    }

    switch (op) {
        case '+': printf("Result: %.2lf\n", a + b); break;
        case '-': printf("Result: %.2lf\n", a - b); break;
        case '*': printf("Result: %.2lf\n", a * b); break;
        case '/':
            if (b == 0) printf("Error: Division by zero.\n");
            else printf("Result: %.2lf\n", a / b);
            break;
        default: printf("Unsupported operator: %c\n", op);
    }
    while (getchar() != '\n'); // clear input buffer
}

void quit_command() {
    printf("Exiting MiniShell...\n");
    exit(0);
}

void sysinfo_command() {
    struct utsname info;
    if (uname(&info) == 0) {
        printf("System:    %s\n", info.sysname);
        printf("Node Name: %s\n", info.nodename);
        printf("Release:   %s\n", info.release);
        printf("Version:   %s\n", info.version);
        printf("Machine:   %s\n", info.machine);
    } else {
        perror("sysinfo");
    }
}

void findfile_command(const char *filename) {
    char command[512];
    snprintf(command, sizeof(command), "find . -name \"%s\" 2>/dev/null", filename);
    system(command);
}

void createfile_command(const char *filename) {
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("createfile");
    } else {
        printf("File '%s' created successfully.\n", filename);
        close(fd);
    }
}

void help_command() {
    printf("MiniShell Help:\n");
    printf("  greet        - Prints a welcome message\n");
    printf("  clr          - Clears the screen\n");
    printf("  calculator   - Launches a basic calculator\n");
    printf("  quit         - Exits the shell\n");
    printf("  findfile     - Searches for a file\n");
    printf("  createfile   - Creates an empty file\n");
    printf("  sysinfo      - Displays system info\n");
    printf("  help         - Displays this help message\n");
}
int is_custom_command(const char *cmd) {
    return strcmp(cmd, "greet") == 0 ||
           strcmp(cmd, "clr") == 0 ||
           strcmp(cmd, "calculator") == 0 ||
           strcmp(cmd, "quit") == 0 ||
           strcmp(cmd, "sysinfo") == 0 ||
           strcmp(cmd, "findfile") == 0 ||
           strcmp(cmd, "createfile") == 0 ||
           strcmp(cmd, "help") == 0;
}

/* Call the appropriate custom command */
void handle_custom_command(char **args) {
    if (strcmp(args[0], "greet") == 0) {
        greet_command();
    } else if (strcmp(args[0], "clr") == 0) {
        clr_command();
    } else if (strcmp(args[0], "calculator") == 0) {
        calculator_command();
    } else if (strcmp(args[0], "quit") == 0) {
        quit_command();
    } else if (strcmp(args[0], "sysinfo") == 0) {
        sysinfo_command();
    } else if (strcmp(args[0], "findfile") == 0) {
        if (args[1])
            findfile_command(args[1]);
        else
            printf("Usage: findfile <filename>\n");
    } else if (strcmp(args[0], "createfile") == 0) {
        if (args[1])
            createfile_command(args[1]);
        else
            printf("Usage: createfile <filename>\n");
    } else if (strcmp(args[0], "help") == 0) {
        help_command();
    } else {
        printf("Unknown custom command: %s\n", args[0]);
    }
}