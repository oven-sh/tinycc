#include <stdio.h>

int main(void)
{
    int x = 42;
    asm("mov x0, #0");
    printf("x = %d\n", x);
    return 0;
}
