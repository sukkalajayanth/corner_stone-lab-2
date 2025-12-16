#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program>\n", argv[0]);
        return 1;
    }

    printf("Minimal debugger skeleton\n");
    printf("Target program: %s\n", argv[1]);
    printf("Debugger framework initialized.\n");

    return 0;
}
