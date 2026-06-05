#ifndef SHELL_H
#define SHELL_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>
#include<signal.h>
#include<errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PIPES 8

#define COLOR_GREEN "\x1b[32m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RED "\x1b[31m"
#define COLOR_RESET "\x1b[0m"
// core loop
void run_shell(void);

// input
char *read_line(void);

// parsing
char **tokenize(char *line);
int count_pipes(char **tokens);

//execution
void execute(char **args);
void execute_piped(char **tokens, int n_pipes);

// builtin
int is_builtin(char *cmd);
void run_builtin(char **args);

// signals
void setup_signals(void);

#endif
