# unix-shell

A small Unix shell written from scratch in C, built on top of `fork`, `execvp`, `pipe`, and `dup2`. It implements a REPL with built-in commands, I/O redirection, multi-stage piping, and basic signal handling 
```
+------------+------------+------------+------------+------------+------------+
tron_cole>> 
+------------+------------+------------+------------+------------+------------+
```

## Features

- **REPL loop**: prompts for input, tokenizes it, and dispatches to built-ins, piped commands, or external programs
- **Built-in commands**: `cd [dir]`, `pwd`, `exit [code]`, `help`
- **External command execution**: runs any program on `$PATH` via `fork` + `execvp`
- **I/O redirection**: `<` (input), `>` (output, truncate), `>>` (output, append)
- **Piping**: chains multiple commands with `|` (up to 8 pipe stages)
- **Signal handling**:
  - `SIGINT` (Ctrl+C) is caught and reprints the prompt instead of killing the shell
  - `SIGTSTP` (Ctrl+Z) is ignored
  - `SIGCHLD` is handled to reap child processes cleanly

## Build

Requires `gcc` and `make`.

```bash
make
```

This compiles `shell.c` into a `myshell` binary using `-Wall -Wextra -Wpedantic -std=c99`.

## Run

```bash
make run
```

or directly:

```bash
./myshell
```

## Usage

```
tron_cole>> pwd
/home/user/unix-shell

tron_cole>> ls -la > out.txt
tron_cole>> cat out.txt

tron_cole>> cat out.txt | grep myshell | wc -l

tron_cole>> cd ..
tron_cole>> help
tron_cole -- built-in commands:
  cd [dir] -> change directory
  pwd      -> print working directory
  exit [n] -> exit with code n
  help     -> show this message

tron_cole>> exit 0
```

## Project structure

```
.
├── shell.c    # core implementation: REPL, parsing, execution, piping, signals
├── shell.h    # declarations, constants (MAX_INPUT, MAX_ARGS, MAX_PIPES), color codes
├── Makefile   # build/run/clean targets
└── README.md
```

## How it works

1. **`read_line`** reads a line of input from stdin into a fixed buffer.
2. **`tokenize`** splits the line on whitespace using `strtok` into an argument vector.
3. The shell checks whether the first token is a built-in (`is_builtin`); if so, it runs it in-process via `run_builtin` (built-ins like `cd` must run in the parent process to affect the shell's own state).
4. Otherwise, it counts pipe symbols (`count_pipes`). If there are none, it forks and execs a single command via `execute`, which also handles `<`, `>`, and `>>` redirection by opening the target file and `dup2`-ing it onto stdin/stdout before `execvp`.
5. If there are pipes, `execute_piped` splits the token stream into per command argument lists, creates the necessary `pipe()` file descriptors, forks one child per command, and wires each child's stdin/stdout to the appropriate pipe ends before `execvp`.
6. The parent waits on all child PIDs with `waitpid` before returning to the prompt.

## Limitations / ideas for extension

- No support for background jobs (`&`)
- No quoting or escaping (arguments are split purely on whitespace)
- No environment variable expansion (`$VAR`)
- No command history / line editing
- Fixed-size buffers (`MAX_INPUT`, `MAX_ARGS`, `MAX_PIPES`) rather than dynamic growth

## License

No license specified.
