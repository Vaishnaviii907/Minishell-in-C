// #include <dirent.h>
// #include <ctype.h>
// #include <limits.h>
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include "command.h"

// #define MAX_SUGGESTIONS 3

// typedef struct {
//     const char *cmd;
//     int score;
// } Suggestion;

// int levenshtein_distance(const char *s1, const char *s2);
// void to_lower(char *dst, const char *src);
// void load_path_commands(char ***cmds_out, int *count_out);
// int compare_suggestions(const void *a, const void *b);

// int is_builtin(const char *cmd) {
//     return (strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "pwd") == 0);
// }

// void handle_builtin(char **args) {
//     if (strcmp(args[0], "cd") == 0) {
//         if (args[1]) {
//             if (chdir(args[1]) != 0) {
//                 perror("cd");
//             }
//         } else {
//             fprintf(stderr, "cd: missing argument\n");
//         }
//     } else if (strcmp(args[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd))) {
//             printf("%s\n", cwd);
//         } else {
//             perror("pwd");
//         }
//     } else if (strcmp(args[0], "exit") == 0) {
//         printf("Exiting shell...\n");
//         exit(0);
//     }
// }


// /* Suggest up to 3 closest commands */
// void suggest_commands(const char *input) {
//     char **all_cmds = NULL;
//     int cmd_count = 0;
//     load_path_commands(&all_cmds, &cmd_count);

//     Suggestion suggestions[MAX_SUGGESTIONS];
//     for (int i = 0; i < MAX_SUGGESTIONS; i++) {
//         suggestions[i].cmd = NULL;
//         suggestions[i].score = INT_MAX;
//     }

//     char input_lower[256];
//     to_lower(input_lower, input);

//     for (int i = 0; i < cmd_count; i++) {
//         char cmd_lower[256];
//         to_lower(cmd_lower, all_cmds[i]);

//         int dist = levenshtein_distance(input_lower, cmd_lower);
//         for (int j = 0; j < MAX_SUGGESTIONS; j++) {
//             if (dist < suggestions[j].score) {
//                 for (int k = MAX_SUGGESTIONS - 1; k > j; k--)
//                     suggestions[k] = suggestions[k - 1];
//                 suggestions[j].cmd = all_cmds[i];
//                 suggestions[j].score = dist;
//                 break;
//             }
//         }
//     }

//     if (suggestions[0].score <= 3) {
//         printf("\033[33mDid you mean:\n");
//         for (int i = 0; i < MAX_SUGGESTIONS && suggestions[i].cmd; i++) {
//             printf("  • %s\n", suggestions[i].cmd);
//         }
//         printf("\033[0m");
//     }

//     for (int i = 0; i < cmd_count; i++)
//         free(all_cmds[i]);
//     free(all_cmds);
// }

// /* Collect all available commands from $PATH */
// void load_path_commands(char ***cmds_out, int *count_out) {
//     char *path = getenv("PATH");
//     char *dirs = strdup(path);
//     char *token = strtok(dirs, ":");

//     int capacity = 128, count = 0;
//     char **cmds = malloc(capacity * sizeof(char*));

//     while (token) {
//         DIR *dir = opendir(token);
//         if (dir) {
//             struct dirent *entry;
//             while ((entry = readdir(dir)) != NULL) {
//                 if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
//                     if (count >= capacity) {
//                         capacity *= 2;
//                         cmds = realloc(cmds, capacity * sizeof(char*));
//                     }
//                     cmds[count++] = strdup(entry->d_name);
//                 }
//             }
//             closedir(dir);
//         }
//         token = strtok(NULL, ":");
//     }

//     free(dirs);
//     *cmds_out = cmds;
//     *count_out = count;
// }

// /* Case-insensitive helper */
// void to_lower(char *dst, const char *src) {
//     while (*src) {
//         *dst++ = tolower((unsigned char)*src++);
//     }
//     *dst = '\0';
// }

// /* Levenshtein distance */
// int levenshtein_distance(const char *s1, const char *s2) {
//     size_t len1 = strlen(s1), len2 = strlen(s2);
//     int matrix[len1 + 1][len2 + 1];

//     for (size_t i = 0; i <= len1; i++) matrix[i][0] = i;
//     for (size_t j = 0; j <= len2; j++) matrix[0][j] = j;

//     for (size_t i = 1; i <= len1; i++) {
//         for (size_t j = 1; j <= len2; j++) {
//             int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
//             matrix[i][j] = fmin(
//                 fmin(matrix[i - 1][j] + 1, matrix[i][j - 1] + 1),
//                 matrix[i - 1][j - 1] + cost);
//         }
//     }

//     return matrix[len1][len2];
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>    // <-- for fmin
#include "command.h"

#define PATH_MAX 4096  // example value on most Linux systems

#define MAX_SUGGESTIONS 3

typedef struct {
    const char *cmd;
    int score;
} Suggestion;

/* Built-in command check */
int is_builtin(const char *cmd) {
    return strcmp(cmd, "cd") == 0 ||
           strcmp(cmd, "pwd") == 0 ||
           strcmp(cmd, "exit") == 0;
}

/* Handle built-in command logic */
void handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        } else {
            fprintf(stderr, "cd: missing argument\n");
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
    } else if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }
}

/* Convert string to lowercase */
static void to_lower(char *dst, const char *src) {
    while (*src) {
        *dst++ = tolower((unsigned char)*src++);
    }
    *dst = '\0';
}

/* Compute Levenshtein distance (edit distance) */
static int levenshtein_distance(const char *s1, const char *s2) {
    size_t len1 = strlen(s1), len2 = strlen(s2);
    int matrix[len1 + 1][len2 + 1];

    for (size_t i = 0; i <= len1; i++) matrix[i][0] = i;
    for (size_t j = 0; j <= len2; j++) matrix[0][j] = j;

    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            matrix[i][j] = fmin(
                fmin(matrix[i - 1][j] + 1, matrix[i][j - 1] + 1),
                matrix[i - 1][j - 1] + cost
            );
        }
    }

    return matrix[len1][len2];
}

/* Suggest commands from PATH based on Levenshtein distance */
void suggest_commands(const char *input) {
    char *path = getenv("PATH");
    if (!path) return;

    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");

    Suggestion suggestions[MAX_SUGGESTIONS] = {0};
    for (int i = 0; i < MAX_SUGGESTIONS; i++) {
        suggestions[i].score = INT_MAX;
    }

    char input_lower[256];
    to_lower(input_lower, input);

    while (dir) {
        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *entry;
            while ((entry = readdir(dp))) {
                if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                    char cmd_lower[256];
                    to_lower(cmd_lower, entry->d_name);
                    int dist = levenshtein_distance(input_lower, cmd_lower);

                    for (int i = 0; i < MAX_SUGGESTIONS; i++) {
                        if (dist < suggestions[i].score) {
                            for (int j = MAX_SUGGESTIONS - 1; j > i; j--) {
                                suggestions[j] = suggestions[j - 1];
                            }
                            suggestions[i].cmd = strdup(entry->d_name);
                            suggestions[i].score = dist;
                            break;
                        }
                    }
                }
            }
            closedir(dp);
        }
        dir = strtok(NULL, ":");
    }

    if (suggestions[0].score <= 3) {
        printf("\033[33mDid you mean:\n");
        for (int i = 0; i < MAX_SUGGESTIONS && suggestions[i].cmd; i++) {
            printf("  • %s\n", suggestions[i].cmd);
        }
        printf("\033[0m");
    }

    for (int i = 0; i < MAX_SUGGESTIONS; i++) {
        if (suggestions[i].cmd)
            free((char *)suggestions[i].cmd);
    }
    free(path_copy);
}
