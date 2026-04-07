#include <stdio.h>
#include <assert.h>

int main(void)
{
    int x = 42;
    int y;
    asm("ldr %0, [%1]" : "=r"(y) : "r"(&x));
    printf("y = %d\n", y);
    assert(y == 42);
    printf("PASSED\n");
    return 0;
}
