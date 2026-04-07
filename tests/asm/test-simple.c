#include <stdio.h>

int main(void)
{
    int x;
    asm("mov %0, #42" : "=r"(x));
    printf("x = %d\n", x);
    return 0;
}
