#include <stdio.h>

int _dowildcard = 1;

int main(int argc, char **argv)
{
    int i;

    printf("argc=%d\n", argc);
    for (i = 1; i < argc; ++i)
        printf("arg%d=<%s>\n", i, argv[i]);
    return 0;
}
