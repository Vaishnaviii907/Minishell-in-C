#ifndef EXECUTOR_H
#define EXECUTOR_H

typedef struct {
    char **argv;             // ← change back to match rest of the code
    char *input_file;
    char *output_file;
    int append;
} Command;


void execute_pipeline(Command *cmds, int num_cmds);

#endif

