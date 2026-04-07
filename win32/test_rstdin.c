#include <stdio.h>

int main(void)
{
    char buf[64];

    if (!fgets(buf, sizeof buf, stdin))
        return 1;
    printf("%s", buf);
    return 0;
}
