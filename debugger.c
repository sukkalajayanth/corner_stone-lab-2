#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/*
 * Core debugger: process creation and ptrace attach
 */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program>\n", argv[0]);
        return 1;
    }

    pid_t child = fork();

    if (child == 0) {
        // Child process (debuggee)
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else {
        // Parent process (debugger)
        int status;
        waitpid(child, &status, 0);
        printf("Debugger attached to process %d\n", child);
    }

    return 0;
}
