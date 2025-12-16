#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <signal.h>


typedef struct {
    long addr;
    long orig_data;
    int enabled;
} breakpoint_t;

breakpoint_t bp = {0};

void print_status(int status) {
    if (WIFEXITED(status)) {
        printf("[STATUS] Process exited with code %d\n",
               WEXITSTATUS(status));
    } 
    else if (WIFSTOPPED(status)) {
        printf("[STATUS] Process stopped by signal %d",
               WSTOPSIG(status));
        if (WSTOPSIG(status) == SIGTRAP)
            printf(" (SIGTRAP)");
        printf("\n");
    }
}

void print_registers(pid_t pid) {
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);

    printf("RIP = 0x%llx\n", regs.rip);
    printf("RSP = 0x%llx  RBP = 0x%llx\n", regs.rsp, regs.rbp);
    printf("RAX = 0x%llx  RBX = 0x%llx  RCX = 0x%llx\n",
           regs.rax, regs.rbx, regs.rcx);
}

void set_breakpoint(pid_t pid, long addr) {
    if (bp.enabled) {
        printf("A breakpoint is already set. Clear it first.\n");
        return;
    }

    long data = ptrace(PTRACE_PEEKTEXT, pid, (void*)addr, NULL);
    bp.addr = addr;
    bp.orig_data = data;
    bp.enabled = 1;

    long data_with_int3 = (data & ~0xff) | 0xcc;
    ptrace(PTRACE_POKETEXT, pid, (void*)addr, (void*)data_with_int3);

    printf("[+] Breakpoint set at 0x%lx\n", addr);
}

void clear_breakpoint(pid_t pid) {
    if (!bp.enabled) {
        printf("No active breakpoint to clear\n");
        return;
    }

    ptrace(PTRACE_POKETEXT, pid,
           (void*)bp.addr, (void*)bp.orig_data);

    bp.enabled = 0;
    printf("[+] Breakpoint cleared\n");
}

void handle_breakpoint(pid_t pid) {
    struct user_regs_struct regs;

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);

    /* RIP points past INT3 → fix it */
    regs.rip -= 1;
    ptrace(PTRACE_SETREGS, pid, NULL, &regs);

    /* Restore original instruction */
    ptrace(PTRACE_POKETEXT, pid,
           (void*)bp.addr, (void*)bp.orig_data);

    bp.enabled = 0;

    printf("[*] Breakpoint hit\n");
    print_registers(pid);
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
    } 
    else {
        int status;
        waitpid(child, &status, 0);
        print_status(status);

        char cmd[64];

        while (1) {
            printf("dbg> ");
            if (!fgets(cmd, sizeof(cmd), stdin))
                break;

            if (strncmp(cmd, "break", 5) == 0) {
                long addr;
                if (sscanf(cmd + 6, "%lx", &addr) != 1) {
                    printf("Usage: break <hex_address>\n");
                } else {
                    set_breakpoint(child, addr);
                }
            }
            else if (strncmp(cmd, "clear", 5) == 0) {
                clear_breakpoint(child);
            }
            else if (strncmp(cmd, "cont", 4) == 0) {
                ptrace(PTRACE_CONT, child, NULL, NULL);
                waitpid(child, &status, 0);
                print_status(status);

                if (bp.enabled &&
                    WIFSTOPPED(status) &&
                    WSTOPSIG(status) == SIGTRAP)
                    handle_breakpoint(child);
            }
            else if (strncmp(cmd, "step", 4) == 0) {
                ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
                waitpid(child, &status, 0);
                print_status(status);
                print_registers(child);
            }
            else if (strncmp(cmd, "regs", 4) == 0) {
                print_registers(child);
            }
            else if (strncmp(cmd, "quit", 4) == 0) {
                break;
            }
            else {
                printf("Commands: break <addr>, clear, cont, step, regs, quit\n");
            }
        }
    }

    return 0;
}
