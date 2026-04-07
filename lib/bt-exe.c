/* ------------------------------------------------------------- */
/* for linking rt_printline and the signal/exception handler
   from tccrun.c into executables. */

#define CONFIG_TCC_BACKTRACE_ONLY
#define ONE_SOURCE 1
#define pstrcpy tcc_pstrcpy
#include "../tccrun.c"

#ifndef _WIN32
# define __declspec(n)
#endif

#if defined(_WIN64) && defined(__aarch64__)
/* The bt-only Windows ARM64 build should not rely on importing this helper. */
LONG InterlockedExchange(LONG volatile *Target, LONG Value)
{
    LONG Old = *Target;
    *Target = Value;
    return Old;
}
#endif

#ifdef _WIN64
static void bt_init_pe_prog_base(rt_context *p)
{
    MEMORY_BASIC_INFORMATION mbi;
    addr_t imagebase;

    if (!p->prog_base)
        return;
    if (!VirtualQuery(p, &mbi, sizeof(mbi)) || !mbi.AllocationBase)
        return;
    imagebase = (addr_t)mbi.AllocationBase - p->prog_base;
    p->prog_base = (addr_t)mbi.AllocationBase - (imagebase & 0xffffffffu);
}
#endif

__declspec(dllexport)
void __bt_init(rt_context *p, int is_exe)
{
    __attribute__((weak)) int main();
    __attribute__((weak)) void __bound_init(void*, int);

    /* call __bound_init here due to redirection of sigaction */
    /* needed to add global symbols */
    if (p->bounds_start)
	__bound_init(p->bounds_start, -1);

#ifdef _WIN64
    bt_init_pe_prog_base(p);
#endif

    /* add to chain */
    rt_wait_sem();
    p->next = g_rc, g_rc = p;
    rt_post_sem();
    if (is_exe) {
        /* we are the executable (not a dll) */
        p->top_func = main;
        set_exception_handler();
    }
}

#ifdef _WIN32
static int bt_backtrace_msg(rt_frame *f, const char *fmt, const char *msg)
{
    rt_context *rc, *rc2;
    addr_t pc;
    char skip[40];
    int i, level, ret, n, one;
    const char *a;
    bt_info bi;
    addr_t (*getinfo)(rt_context*, addr_t, bt_info*);

    rt_backtrace_format(fmt, skip, &one);

    rt_wait_sem();
    rc = g_rc;
    getinfo = rt_printline, n = 6;
    if (rc) {
        if (rc->dwarf)
            getinfo = rt_printline_dwarf;
        if (rc->num_callers)
            n = rc->num_callers;
    }

    for (i = level = 0; level < n; i++) {
        ret = rt_get_caller_pc(&pc, f, i);
        if (ret == -1)
            break;
        memset(&bi, 0, sizeof bi);
        for (rc2 = rc; rc2; rc2 = rc2->next) {
            if (getinfo(rc2, pc, &bi))
                break;
            if (!!(a = rt_elfsym(rc2, pc, &bi.func_pc))) {
                pstrcpy(bi.func, sizeof bi.func, a);
                break;
            }
        }
        if (skip[0] && strstr(bi.file, skip))
            continue;
        if (bi.file[0]) {
            rt_printf("%s:%d", bi.file, bi.line);
        } else {
            rt_printf("0x%08llx", (long long)pc);
        }
        rt_printf(": %s %s", level ? "by" : "at", bi.func[0] ? bi.func : "???");
        if (level == 0) {
            rt_printf(": %s", msg);
            if (one)
                break;
        }
        rt_printf("\n");
        if (rc2
            && bi.func_pc
            && bi.func_pc == (addr_t)rc2->top_func)
            break;
        ++level;
    }
    rt_post_sem();
    return 0;
}

__declspec(dllexport)
int __bt_backtrace(rt_frame *f, const char *msg)
{
    return bt_backtrace_msg(f, msg, msg);
}
#endif

__declspec(dllexport)
void __bt_exit(rt_context *p)
{
    struct rt_context *rc, **pp;
    __attribute__((weak)) void __bound_exit_dll(void*);

    //fprintf(stderr, "__bt_exit %d %p\n", !!p->top_func, p);

    if (p->bounds_start)
	__bound_exit_dll(p->bounds_start);

    /* remove from chain */
    rt_wait_sem();
    for (pp = &g_rc; rc = *pp, rc; pp = &rc->next)
        if (rc == p) {
            *pp = rc->next;
            break;
        }
    rt_post_sem();
}

/* copy a string and truncate it. */
ST_FUNC char *pstrcpy(char *buf, size_t buf_size, const char *s)
{
    int l = strlen(s);
    if (l >= buf_size)
        l = buf_size - 1;
    memcpy(buf, s, l);
    buf[l] = 0;
    return buf;
}
