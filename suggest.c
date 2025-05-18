#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include "suggest.h"

#define PATH_MAX_LEN 4096
#define MAX_CMD_LEN 256
#define MAX_SUGGESTIONS 3
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    const char *cmd;
    int score;
} Suggestion;

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
            matrix[i][j] = MIN(
                MIN(matrix[i - 1][j] + 1, matrix[i][j - 1] + 1),
                matrix[i - 1][j - 1] + cost
            );
        }
    }

    return matrix[len1][len2];
}

/* Suggest up to MAX_SUGGESTIONS commands based on user input */
void suggest_commands(const char *input) {
    char *path = getenv("PATH");
    if (!path) return;

    char *path_copy = strdup(path);
    if (!path_copy) return;

    Suggestion suggestions[MAX_SUGGESTIONS] = {0};
    for (int i = 0; i < MAX_SUGGESTIONS; i++) {
        suggestions[i].score = INT_MAX;
    }

    char input_lower[MAX_CMD_LEN];
    to_lower(input_lower, input);

    char *dir = strtok(path_copy, ":");
    while (dir) {
        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *entry;
            while ((entry = readdir(dp))) {
                if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                    char cmd_lower[MAX_CMD_LEN];
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
