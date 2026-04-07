#include <stdio.h>
#include <wchar.h>

int wmain(int argc, wchar_t **argv, wchar_t **envp)
{
    (void)argc;
    (void)argv;
    if (!envp || !envp[0]) {
        fputws(L"missing envp\n", stderr);
        return 1;
    }
    return 0;
}
