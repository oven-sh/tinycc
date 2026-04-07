#include <stdio.h>

static volatile int crash_value;

__attribute__((noinline))
static void crash4(double a0, double a1, double a2, double a3,
                   double a4, double a5, double a6, double a7,
                   int i0, int i1, int i2, int i3,
                   int i4, int i5, int i6, int i7)
{
    volatile char frame[8192];

    frame[0] = (char)(a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 +
                      i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7);
    crash_value = frame[0];
    *(volatile int *)0 = crash_value;
}

__attribute__((noinline))
static void crash3(void)
{
    puts("crash3");
    fflush(stdout);
    crash4(1.0, 2.0, 3.0, 4.0,
           5.0, 6.0, 7.0, 8.0,
           9, 10, 11, 12, 13, 14, 15, 16);
}

__attribute__((noinline))
static void crash2(void)
{
    puts("crash2");
    fflush(stdout);
    crash3();
}

__attribute__((noinline))
static void crash1(void)
{
    puts("crash1");
    fflush(stdout);
    crash2();
}

int main(void)
{
    puts("main");
    fflush(stdout);
    crash1();
    return 0;
}
