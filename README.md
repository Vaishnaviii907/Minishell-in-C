# Minishell

A simple Unix shell written in C.

## Features
- Basic command execution (like `ls`, `pwd`, `date`)
- Simple prompt loop
- Graceful exit with `exit`

## Usage
Compile the code:
```bash
gcc myShell.c -o minishell

## to compile the new brach
gcc -Wall -g main.c parser.c executor.c builtins.c -o minishell -lreadline
