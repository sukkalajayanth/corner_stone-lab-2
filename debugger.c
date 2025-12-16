#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string.h>
typedef struct {
    long addr;
    long orig_data;
    int enabled;
} breakpoint_t;

breakpoint_t bp = {0};

void set_breakpoint(pid_t pid, long addr) {
    long data = ptrace(PTRACE_PEEKTEXT, pid, (void*)addr, NULL);
    bp.addr = addr;
    bp.orig_data = data;
    bp.enabled = 1;

    long data_with_int3 = (data & ~0xff) | 0xcc;
    ptrace(PTRACE_POKETEXT, pid, (void*)addr, (void*)data_with_int3);

    printf("[+] Breakpoint set at 0x%lx\n", addr);
}

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

            if (strncmp(cmd, "break", 5) == 0) {
                long addr;
                sscanf(cmd + 6, "%lx", &addr);
                set_breakpoint(child, addr);
            }
            else if (strncmp(cmd, "cont", 4) == 0) {
                ptrace(PTRACE_CONT, child, NULL, NULL);
                waitpid(child, &status, 0);
            }
            else if (strncmp(cmd, "step", 4) == 0) {
                ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
                waitpid(child, &status, 0);
            }
            else if (strncmp(cmd, "quit", 4) == 0) {
                break;
            }
            else {
                printf("Commands: break <addr>, cont, step, quit\n");
            }
        }
    }

    return 0;
}
