#include <stdio.h>

static void plain_inline_asm(void)
{
    asm("nop");
}

static int inline_asm_goto(void)
{
    asm goto("b %l[target]" : : : : target);
    return 1;
target:
    return 0;
}

#if defined(TEST_OPERANDS)
int main(void)
{
    int input = 1;
    int output = 0;

    asm("add %w0, %w1, #1" : "=r"(output) : "r"(input));
    printf("%d\n", output);
    return output != 2;
}
#elif defined(TEST_CLOBBERS)
int main(void)
{
    asm volatile("nop" : : : "x0");
    return 0;
}
#elif defined(TEST_GOTO)
int main(void)
{
    int rc = inline_asm_goto();

    printf("%s\n", rc ? "asm goto wrong" : "asm goto ok");
    return rc;
}
#else
int main(void)
{
    int rc;

    plain_inline_asm();
    rc = inline_asm_goto();
    printf("%s\n", rc ? "inline asm wrong" : "inline asm ok");
    return rc;
}
#endif
