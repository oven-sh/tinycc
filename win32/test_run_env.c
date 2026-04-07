#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    (void)argc;
    (void)argv;
    if (!envp || !envp[0]) {
        fputs("missing envp\n", stderr);
        return 1;
    }
    return 0;
}
