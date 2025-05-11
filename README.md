<
 ## for readline
sudo apt update
sudo apt install libreadline-dev

# Minishell
A simple Unix shell written in C.

## Features
- Basic command execution (like `ls`, `pwd`, `date`)
- Simple prompt loop
- Graceful exit with `exit`

## to compile the shell
gcc -Wall -Werror main.c builtins.c parser.c executor.c  -o minishell -lreadline -lncurses

