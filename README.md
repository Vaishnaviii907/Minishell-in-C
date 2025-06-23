# Minishell

**Minishell** is a modern, educational Unix-like shell written in C, featuring both a classic terminal interface and a beautiful GTK-based GUI. It supports core shell features, custom commands, and a visually appealing, user-friendly experience.

---

## Features

- Basic command execution (`ls`, `pwd`, `date`, etc.)
- Built-in commands: `cd`, `pwd`, `exit`, `echo`
- Input redirection (`<`)
- Output redirection (`>` and `>>`)
- Command piping (`|`)
- Custom commands: `greet`, `clr`, `calculator`, `quit`, `sysinfo`, `findfile`, `createfile`, `help`
- Command suggestions for mistyped commands
- Modern GTK GUI with:
  - Terminal-like prompt, input, and output
  - Command history navigation (up/down)
  - Colored prompt, output, and errors
  - Custom header bar with minimize, maximize, and close buttons
  - Dark theme, monospaced font, and vibrant colors
  - Prompt is un-erasable and always visible
  - GUI-specific features (e.g., calculator dialog)

---

## Installation

First, ensure you have the required libraries installed:

```bash
sudo apt update
sudo apt install libreadline-dev libgtk-3-dev
```

---

## Compilation

You can compile the project in two ways:

### 🔸 Terminal version:

```bash
gcc -Wall -Werror main.c builtins.c parser.c executor.c suggest.c custom.c  -o minishell -lreadline -lncurses
```

### 🔸 GUI version (GTK):

```bash
gcc -o gui gui.c shell_interface.c shell_core.c builtins.c custom.c executor.c parser.c suggest.c `pkg-config --cflags --libs gtk+-3.0`
```

### 🔸 Using the Makefile:

```bash
make
```

This will produce both `minishell` (terminal) and `gui` (GTK GUI) executables.

To clean up compiled objects and binaries:

```bash
make clean
```

---

## 🚀 Usage

### Terminal version:
```bash
./minishell
```

### GUI version:
```bash
./gui
```

**GUI Features:**
- Modern, resizable window with custom header bar
- Prompt and output styled like a real terminal
- Command history (up/down arrows)
- Colored prompt, output, and errors
- Un-erasable prompt and protected input area
- Minimize, maximize, and close buttons
- Custom commands (e.g., calculator opens a dialog)

**Example commands:**
```bash
ls -l | grep ".c" > files.txt
cat < files.txt
cd ..
pwd
greet
sysinfo
findfile gui.c
createfile test.txt
calculator
exit
```

---

## Project Structure

```
.
├── main.c            # Shell loop and prompt (terminal)
├── gui.c             # GTK GUI implementation
├── builtins.c        # Handles built-in commands
├── custom.c          # Handles custom commands
├── parser.c          # Parses user input
├── executor.c        # Executes parsed commands
├── shell_core.c      # Core shell logic for GUI
├── shell_interface.c # GUI/terminal interface bridge
├── *.h               # Header files
├── Makefile          # Build instructions
└── README.md         # Project documentation
```

---

## Screenshots

<!-- Add your screenshots here -->

---

## Future Improvements

- [x] Add custom shell commands
- [x] GUI interface using GTK
- [ ] Background process support (`&`)
- [ ] Signal handling (e.g., Ctrl+C)
- [x] Command history and editing
- [ ] Tab completion
- [ ] More advanced error handling

## Contributions

Pull requests, suggestions, and bug reports are welcome! Feel free to fork this repo and build upon it.
