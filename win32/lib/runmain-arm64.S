#ifdef __leading_underscore
# define _(s) _##s
#else
# define _(s) s
#endif

    .text
    .p2align 2

    .global _(__run_on_exit)
_(__run_on_exit):
    b _(__tcc_run_on_exit)

    .global _(on_exit)
_(on_exit):
    b _(__tcc_on_exit)

    .global _(atexit)
_(atexit):
    b _(__tcc_atexit)

    .global _(exit)
_(exit):
    b _(__tcc_exit)
