#include <stddef.h>
#include <stdio.h>

struct pe_field_align_min {
    char c;
    double d __attribute__((aligned(1)));
};

int main(void)
{
    printf("size=%u align=%u off=%u\n",
        (unsigned)sizeof(struct pe_field_align_min),
        (unsigned)__alignof__(struct pe_field_align_min),
        (unsigned)offsetof(struct pe_field_align_min, d));
    return 0;
}
