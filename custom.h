#ifndef CUSTOM_COMMANDS_H
#define CUSTOM_COMMANDS_H

// Dispatcher functions for checking and handling custom commands
int is_custom_command(const char *cmd);
void handle_custom_command(char **args);

// Individual custom command declarations
void clr_command();
void help_command();
void calculator_command();
void greet_command();
void quit_command();
void sysinfo_command();
void findfile_command(const char *filename);
void createfile_command(const char *filename);

#endif // CUSTOM_COMMANDS_H
