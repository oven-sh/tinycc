/* ------------------------------------------------------------- */
/* stubs for calling bcheck functions from a dll. */

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#define REDIR_ALL \
  REDIR(__bt_init) \
  REDIR(__bt_exit) \
  REDIR(__bt_backtrace) \
  REDIR(tcc_backtrace) \
  \
  REDIR(__bound_ptr_add) \
  REDIR(__bound_ptr_indir1) \
  REDIR(__bound_ptr_indir2) \
  REDIR(__bound_ptr_indir4) \
  REDIR(__bound_ptr_indir8) \
  REDIR(__bound_ptr_indir12) \
  REDIR(__bound_ptr_indir16) \
  REDIR(__bound_local_new) \
  REDIR(__bound_local_delete) \
  REDIR(__bound_new_region) \
  \
  REDIR(__bound_free) \
  REDIR(__bound_malloc) \
  REDIR(__bound_realloc) \
  REDIR(__bound_memcpy) \
  REDIR(__bound_memcmp) \
  REDIR(__bound_memmove) \
  REDIR(__bound_memset) \
  REDIR(__bound_strlen) \
  REDIR(__bound_strcpy) \
  REDIR(__bound_strncpy) \
  REDIR(__bound_strcmp) \
  REDIR(__bound_strncmp) \
  REDIR(__bound_strcat) \
  REDIR(__bound_strchr) \
  REDIR(__bound_strdup)

#ifdef __leading_underscore
#define _(s) "_"#s
#else
#define _(s) #s
#endif

#define REDIR(s) void *s;
static struct { REDIR_ALL } all_ptrs;
#undef REDIR
#define REDIR(s) #s"\0"
static const char all_names[] = REDIR_ALL;
#undef REDIR
#if defined(__aarch64__)
typedef struct rt_context rt_context;
typedef struct rt_frame {
    void *ip, *fp, *sp;
} rt_frame;
#ifndef FASTCALL
#define FASTCALL
#endif
#define REDIR_WRAP(ret, name, decl_args, type_args, call_args) \
    ret name decl_args                                         \
    {                                                          \
        typedef ret (*fn_t) type_args;                         \
        return ((fn_t)all_ptrs.name) call_args;                \
    }

static void all_jmps(void) {
    /* ARM64 uses C wrappers instead of instruction trampolines. */
}

REDIR_WRAP(void, __bt_init, (rt_context *p, int is_exe),
           (rt_context *, int), (p, is_exe))
REDIR_WRAP(void, __bt_exit, (rt_context *p),
           (rt_context *), (p))
REDIR_WRAP(int, __bt_backtrace, (rt_frame *f, const char *msg),
           (rt_frame *, const char *), (f, msg))

#define REDIR_PTR_INDIR(name) \
    REDIR_WRAP(void *, name, (void *p, size_t offset), \
               (void *, size_t), (p, offset))

REDIR_PTR_INDIR(__bound_ptr_add)
REDIR_PTR_INDIR(__bound_ptr_indir1)
REDIR_PTR_INDIR(__bound_ptr_indir2)
REDIR_PTR_INDIR(__bound_ptr_indir4)
REDIR_PTR_INDIR(__bound_ptr_indir8)
REDIR_PTR_INDIR(__bound_ptr_indir12)
REDIR_PTR_INDIR(__bound_ptr_indir16)

REDIR_WRAP(void FASTCALL, __bound_local_new, (void *p1),
           (void *), (p1))
REDIR_WRAP(void FASTCALL, __bound_local_delete, (void *p1),
           (void *), (p1))
REDIR_WRAP(void, __bound_new_region, (void *p, size_t size),
           (void *, size_t), (p, size))

REDIR_WRAP(void, __bound_free, (void *ptr, const void *caller),
           (void *, const void *), (ptr, caller))
REDIR_WRAP(void *, __bound_malloc, (size_t size, const void *caller),
           (size_t, const void *), (size, caller))
REDIR_WRAP(void *, __bound_realloc, (void *ptr, size_t size, const void *caller),
           (void *, size_t, const void *), (ptr, size, caller))
REDIR_WRAP(void *, __bound_memcpy, (void *dst, const void *src, size_t size),
           (void *, const void *, size_t), (dst, src, size))
REDIR_WRAP(int, __bound_memcmp, (const void *s1, const void *s2, size_t size),
           (const void *, const void *, size_t), (s1, s2, size))
REDIR_WRAP(void *, __bound_memmove, (void *dst, const void *src, size_t size),
           (void *, const void *, size_t), (dst, src, size))
REDIR_WRAP(void *, __bound_memset, (void *dst, int c, size_t size),
           (void *, int, size_t), (dst, c, size))
REDIR_WRAP(int, __bound_strlen, (const char *s),
           (const char *), (s))
REDIR_WRAP(char *, __bound_strcpy, (char *dst, const char *src),
           (char *, const char *), (dst, src))
REDIR_WRAP(char *, __bound_strncpy, (char *dst, const char *src, size_t n),
           (char *, const char *, size_t), (dst, src, n))
REDIR_WRAP(int, __bound_strcmp, (const char *s1, const char *s2),
           (const char *, const char *), (s1, s2))
REDIR_WRAP(int, __bound_strncmp, (const char *s1, const char *s2, size_t n),
           (const char *, const char *, size_t), (s1, s2, n))
REDIR_WRAP(char *, __bound_strcat, (char *dest, const char *src),
           (char *, const char *), (dest, src))
REDIR_WRAP(char *, __bound_strchr, (const char *string, int ch),
           (const char *, int), (string, ch))
REDIR_WRAP(char *, __bound_strdup, (const char *s),
           (const char *), (s))

int tcc_backtrace(const char *fmt, ...)
{
    char buf[1024];
    rt_frame f;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    f.ip = __builtin_return_address(0);
    f.fp = __builtin_frame_address(1);
    f.sp = __builtin_frame_address(0);
    return __bt_backtrace(&f, buf);
}

#undef REDIR_PTR_INDIR
#undef REDIR_WRAP
#else
#define REDIR(s) __asm__(".global " _(s) ";" _(s) ": jmp *%0" : : "m" (all_ptrs.s) );
static void all_jmps() { REDIR_ALL }
#endif
#undef REDIR

void __bt_init_dll(int bcheck)
{
    const char *s = all_names;
    void **p = (void**)&all_ptrs;
    do {
        *p = (void*)GetProcAddress(GetModuleHandle(NULL), (char*)s);
        if (NULL == *p) {
            char buf[100];
            sprintf(buf,
                "Error: function '%s()' not found in executable. "
                "(Need -bt or -b for linking the exe.)", s);
            if (GetStdHandle(STD_ERROR_HANDLE))
                fprintf(stderr, "TCC/BCHECK: %s\n", buf), fflush(stderr);
            else
                MessageBox(NULL, buf, "TCC/BCHECK", MB_ICONERROR);
            ExitProcess(1);
        }
        s = strchr(s,'\0') + 1, ++p;
    } while (*s && (bcheck || p < &all_ptrs.__bound_ptr_add));
}
