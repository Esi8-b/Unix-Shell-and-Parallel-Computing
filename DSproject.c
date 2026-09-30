
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LINE 80


char *ltrim(char *s) {
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

int main(void) {
    char *args[MAX_LINE / 2 + 1];
    char *args2[MAX_LINE / 2 + 1];
    char input[MAX_LINE];
    char last_command[MAX_LINE] = "";

    while (1) {
        printf("uinxsh> ");
        fflush(stdout);


        while (waitpid(-1, NULL, WNOHANG) > 0);

        if (fgets(input, MAX_LINE, stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';


        if (strcmp(input, "!!") == 0) {
            if (strlen(last_command) == 0) {
                printf("No commands in history.\n");
                continue;
            }
            strcpy(input, last_command);
            printf("%s\n", input);
        } else if (strlen(input) > 0) {
            strcpy(last_command, input);
        }

        int background = 0;
        if (strlen(input) > 0 && input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';
        }

        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';

            char *left  = ltrim(input);
            char *right = ltrim(pipe_pos + 1);

            int i = 0;
            char *token = strtok(left, " ");
            while (token) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            i = 0;
            token = strtok(right, " ");
            while (token) {
                args2[i++] = token;
                token = strtok(NULL, " ");
            }
            args2[i] = NULL;

            int fd[2];
            pipe(fd);

            pid_t p1 = fork();
            if (p1 == 0) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(args[0], args);
                perror("exec");
                exit(1);
            }

            pid_t p2 = fork();
            if (p2 == 0) {
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(args2[0], args2);
                perror("exec");
                exit(1);
            }

            close(fd[0]);
            close(fd[1]);

            if (!background) {
                waitpid(p1, NULL, 0);
                waitpid(p2, NULL, 0);
            } else {
                printf("[Running in background]\n");
            }
            continue;
        }


        int i = 0;
        char *token = strtok(input, " ");
        while (token) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL)
            continue;


        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        if (strcmp(args[0], "pwd") == 0) {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            printf("Command not found: %s\n", args[0]);
            exit(1);
        } else {
            if (!background)
                waitpid(pid, NULL, 0);
            else
                printf("[Running in background]\n");
        }
    }
    return 0;
}
