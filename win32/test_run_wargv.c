#include <stdio.h>
#include <wchar.h>

int _dowildcard = 1;

int wmain(int argc, wchar_t **argv)
{
    int i;

    printf("argc=%d\n", argc);
    for (i = 1; i < argc; ++i)
        printf("arg%d=<%ls>\n", i, argv[i]);
    return 0;
}
