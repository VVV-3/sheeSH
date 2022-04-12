#ifndef SHEESH_H
#define SHEESH_H

// Libraries needed.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>

// Basic config variables used in the shell.
#define SHELL_NAME "sheesh"
#define MAX_NAME_SIZE 100
#define MAX_PATH_SIZE 100
#define MAX_INPUT_SIZE 100
#define MAX_NO_WORDS_IN_COMMAND 100
#define MAX_CHILD_PROCESS_COUNT 100

// I/O streams file descriptors.
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2

// Default file permissions.
#define PERMS 0644

// Struct used to store child process details.
struct process_data
{
    pid_t pid;
    char pname[MAX_INPUT_SIZE + 5];
};

#endif