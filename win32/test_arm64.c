/* Simple ARM64 Windows test program - no inline asm */
#include <stdio.h>

int main(int argc, char **argv) {
    int x = 42;
    int y = x * 2;

    printf("Hello from ARM64 Windows!\n");
    printf("x = %d, y = %d\n", x, y);

    return 0;
}
