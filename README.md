#  Minishell

**Minishell** is a basic Unix-like shell written in C. It supports core shell features including basic command execution, input/output redirection, pipelines, and a simple prompt loop. This project is designed as a learning tool to understand how shells work under the hood.

---

##  Features

- Basic command execution (`ls`, `pwd`, `date`, etc.)
- Built-in commands: `cd`, `pwd`, `exit`
- Input redirection (`<`)
- Output redirection (`>` and `>>`)
- Command piping (`|`)
- Simple prompt loop with graceful termination

---

##  Installation

First, ensure you have the required libraries installed:

```bash
sudo apt update
sudo apt install libreadline-dev
```

---

##  Compilation

You can compile the project in two ways:

### 🔸 Using GCC directly:

```bash
gcc -Wall -Werror main.c builtins.c parser.c executor.c suggest.c  -o minishell -lreadline -lncurses
```

### 🔸 Using the Makefile:

```bash
make
```

This will produce an executable called `minishell`.

To clean up compiled objects and binaries:

```bash
make clean
```

---

## 🚀 Usage

Run the shell using:

```bash
./minishell
```

Example commands you can try:

```bash
ls -l | grep ".c" > files.txt
cat < files.txt
cd ..
pwd
exit
```

---

##  Project Structure

```
.
├── main.c          # Shell loop and prompt
├── builtins.c      # Handles built-in commands
├── parser.c        # Parses user input
├── executor.c      # Executes parsed commands
├── parser.h        # Header for parser
├── executor.h      # Header for executor
├── Makefile        # Build instructions
└── README.md       # Project documentation
```

---

##  Future Improvements

- [ ] Add custom shell commands
- [ ] GUI interface using ncurses/GTK
- [ ] Background process support (`&`)
- [ ] Signal handling (e.g., Ctrl+C)
- [ ] Command history and editing

## Contributions

Pull requests, suggestions, and bug reports are welcome! Feel free to fork this repo and build upon it.
