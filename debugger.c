#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string.h>

/*
 * Core debugger: command loop with cont and step
 */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program>\n", argv[0]);
        return 1;
    }

    pid_t child = fork();

    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else {
        int status;
        waitpid(child, &status, 0);
        printf("Debugger attached\n");

        char cmd[64];

        while (1) {
            printf("dbg> ");
            if (!fgets(cmd, sizeof(cmd), stdin))
                break;

            if (strncmp(cmd, "cont", 4) == 0) {
                ptrace(PTRACE_CONT, child, NULL, NULL);
                waitpid(child, &status, 0);
                printf("Process continued\n");
            }
            else if (strncmp(cmd, "step", 4) == 0) {
                ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
                waitpid(child, &status, 0);
                printf("Single step done\n");
            }
            else if (strncmp(cmd, "quit", 4) == 0) {
                printf("Exiting debugger\n");
                break;
            }
            else {
                printf("Commands: cont, step, quit\n");
            }
        }
    }

    return 0;
}
