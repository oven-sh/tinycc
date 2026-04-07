#include <stdio.h>
#include <stdlib.h>

#ifdef __leading_underscore
# define _(s) s
#else
# define _(s) _##s
#endif

extern void (*_(_fini_array_start)[]) (void);
extern void (*_(_fini_array_end)[]) (void);

typedef struct rt_frame {
    void *ip, *fp, *sp;
} rt_frame;

__attribute__((weak, noreturn)) void __rt_exit(rt_frame *, int);

static void *rt_exitfunc[32];
static void *rt_exitarg[32];
static int __rt_nr_exit;

static void run_dtors(void)
{
    int i = 0;

    while (&_(_fini_array_end)[i] != _(_fini_array_start))
        (*_(_fini_array_end)[--i])();
}

void __tcc_run_on_exit(int ret)
{
    int n = __rt_nr_exit;

    while (n)
        --n, ((void (*)(int, void *))rt_exitfunc[n])(ret, rt_exitarg[n]);
}

int __tcc_on_exit(void *function, void *arg)
{
    int n = __rt_nr_exit;

    if (n < 32) {
        rt_exitfunc[n] = function;
        rt_exitarg[n] = arg;
        __rt_nr_exit = n + 1;
        return 0;
    }
    return 1;
}

int __tcc_atexit(void (*function)(void))
{
    return __tcc_on_exit(function, 0);
}

void __attribute__((noreturn)) __tcc_exit(int code)
{
    rt_frame f;

    run_dtors();
    __tcc_run_on_exit(code);
    if (__rt_exit) {
        f.fp = 0;
        f.ip = __tcc_exit;
        f.sp = 0;
        __rt_exit(&f, code);
    }
    fflush(NULL);
    _exit(code);
}
