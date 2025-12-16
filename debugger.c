#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/*
 * Core debugger: single-step execution
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

        printf("Single stepping one instruction...\n");
        ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
        waitpid(child, &status, 0);

        printf("Single step completed\n");
    }

    return 0;
}
