#define _POSIX_C_SOURCE 200809L
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern char **environ;

int is_allowed_program(const char *cmd) {
    const char *allowed_spawn[] = {
        "cp", "touch", "mkdir", "ls", "pwd", "cat",
        "grep", "chmod", "diff", NULL
    };
    for (int i = 0; allowed_spawn[i] != NULL; ++i) {
        if (strcmp(cmd, allowed_spawn[i]) == 0)
            return 1;
    }
    return 0;
}

int is_builtin(const char *cmd) {
    const char *builtins[] = {"cd", "exit", "help", NULL};
    for (int i = 0; builtins[i] != NULL; ++i) {
        if (strcmp(cmd, builtins[i]) == 0)
            return 1;
    }
    return 0;
}

void print_help(void) {
    printf("The allowed commands are:\n");
    printf("1: cp\n");
    printf("2: touch\n");
    printf("3: mkdir\n");
    printf("4: ls\n");
    printf("5: pwd\n");
    printf("6: cat\n");
    printf("7: grep\n");
    printf("8: chmod\n");
    printf("9: diff\n");
    printf("10: cd\n");
    printf("11: exit\n");
    printf("12: help\n");
    fflush(stdout);
}

int main(void) {
    const size_t LINE_MAX = 4096;
    char line[LINE_MAX];

    while (1) {
        /* printing prompt to stderr */
        fprintf(stderr, "rsh> ");
        fflush(stderr);

        if (fgets(line, sizeof(line), stdin) == NULL)
            return 0;

        /* tokenize input */
        char *saveptr;
        const char *delim = " \t\n";
        char *argv[21];
        int argc = 0;

        char *tok = strtok_r(line, delim, &saveptr);
        while (tok != NULL && argc < 20) {
            argv[argc++] = strdup(tok);
            tok = strtok_r(NULL, delim, &saveptr);
        }
        argv[argc] = NULL;

        if (argc == 0)
            continue;

        char *cmd = argv[0];

        /* builtins */
        if (strcmp(cmd, "exit") == 0) {
            for (int i = 0; i < argc; ++i) free(argv[i]);
            return 0;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
            for (int i = 0; i < argc; ++i) free(argv[i]);
            continue;
        } else if (strcmp(cmd, "cd") == 0) {
            if (argc > 2) {
                printf("-rsh: cd: too many arguments\n");
                fflush(stdout);
            } else if (argc == 1) {
                const char *home = getenv("HOME");
                if (home != NULL) {
                    if (chdir(home) != 0) {
                        printf("-rsh: cd: %s: %s\n", home, strerror(errno));
                        fflush(stdout);
                    }
                }
            } else {
                if (chdir(argv[1]) != 0) {
                    printf("-rsh: cd: %s: %s\n", argv[1], strerror(errno));
                    fflush(stdout);
                }
            }
            for (int i = 0; i < argc; ++i) free(argv[i]);
            continue;
        }

        /* allowed external commands */
        if (!is_allowed_program(cmd)) {
            printf("NOT ALLOWED!\n");
            fflush(stdout);
            for (int i = 0; i < argc; ++i) free(argv[i]);
            continue;
        }

        /* spawn child */
        pid_t child;
        int spawn_err = posix_spawnp(&child, cmd, NULL, NULL, argv, environ);
        if (spawn_err != 0) {
            printf("NOT ALLOWED!\n");
            fflush(stdout);
            for (int i = 0; i < argc; ++i) free(argv[i]);
            continue;
        }

        /* wait for child */
        int status;
        waitpid(child, &status, 0);

        for (int i = 0; i < argc; ++i) free(argv[i]);
    }

    return 0;
}
