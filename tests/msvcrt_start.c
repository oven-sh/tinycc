/* ------------------------------------------------------------- */
/* minimal startup with runtime linker to msvcrt */

#define REDIR_ALL \
 REDIR(puts)\
 REDIR(printf)\
 REDIR(putchar)\
 REDIR(_vsnprintf)\
 REDIR(strtod)\
 REDIR(memset)\
 REDIR(strcpy)\
 REDIR(strlen)\
 REDIR(malloc)\
 REDIR(free)\
 REDIR(__set_app_type)\
 REDIR(exit)

#if defined __i386__ && !defined __TINYC__
# define __leading_underscore 1
#endif

#ifdef __leading_underscore
# define _(s) "_"#s
#else
# define _(s) #s
#endif

#define REDIR(s) void *s;
static struct { REDIR_ALL } all_ptrs;
#undef REDIR

#define REDIR(s) #s"\0"
static const char all_names[] = REDIR_ALL;
#undef REDIR

#if __aarch64__

  #if defined __TINYC__
  # define ALIGN ".align 8"
  #else
  # define ALIGN ".align 3"
  #endif

# define REDIR(s) \
    __asm__("\n"_(s)":"); \
    __asm__(".int 0x58000090"); /* ldr x16, [pc, #16] */ \
    __asm__(".int 0xf9400210"); /* ldr x16, [x16] */ \
    __asm__(".int 0xd61f0200"); /* br x16 */ \
    __asm__(".int 0xd503201f"); /* nop for alignment */ \
    __asm__(".quad all_ptrs + (. - all_jmps - 16) / 24 * 8"); \
    __asm__(".global "_(s));

    __asm__("\t.text\n\t"ALIGN"\nall_jmps:");
    REDIR_ALL

#else
# define REDIR(s) \
    __asm__("\n"_(s)":");\
    __asm__("jmp *%0"::"m"(all_ptrs.s));\
    __asm__(".global "_(s));

    static void all_jmps() { REDIR_ALL }
#endif
#undef REDIR

#ifdef __i386__
# define STDCALL __declspec(stdcall)
#else
# define STDCALL
#endif
STDCALL void *LoadLibraryA(const char *);
STDCALL void *GetProcAddress(void*, char*);
STDCALL void ExitProcess(int);
STDCALL int WriteFile(void*, const void*, unsigned, unsigned *, void*);
STDCALL void* GetStdHandle(unsigned);
STDCALL int FlushFileBuffers(void*);
#define STD_INPUT_HANDLE (-10)
#define STD_OUTPUT_HANDLE (-11)
#define STD_ERROR_HANDLE (-12)

void eput(const char *s)
{
    unsigned n_out;
    int n;

    for (n = 0; s[n]; )
        ++n;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), s, n, &n_out, 0);
}

void __rt_init()
{
    const char *s = all_names;
    void **p = (void**)&all_ptrs;
    void *dll = LoadLibraryA("msvcrt.dll");
    do {
        char buf[100], *d = buf;
        *p = (void*)GetProcAddress(dll, (char*)s);
        *d++ = '_'; do *d++ = *s; while (*s++);
        if (0 == *p)
            *p = (void*)GetProcAddress(dll, buf);
        if (0 == *p) {
            eput("CRT: RUNTIME RELOCATION ERROR: '");
            eput(buf+1);
            eput("'\n");
            ExitProcess(-1);
        }
        ++p;
    } while (*s);
}

void __main() {}
void __chkstk(unsigned n) {}
void _pei386_runtime_relocator(void) {}

int main(int argc, char **argv);
void exit(int);
void __set_app_type(int apptype);

#include <stdarg.h>
#define size_t __SIZE_TYPE__

int printf(const char *, ...);
int _vsnprintf(char *, size_t, const char *, va_list);

int vprintf(const char *format, va_list ap)
{
    char buf[1000];
    _vsnprintf(buf, sizeof buf, format, ap);
    return printf("%s", buf);
}

#ifdef __clang__
# define _start mainCRTStartup
#endif

void _start(void)
{
    __rt_init();
    __set_app_type(1);
    exit(main(1, 0));
}
