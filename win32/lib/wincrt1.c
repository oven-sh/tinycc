//+---------------------------------------------------------------------------

// _UNICODE for tchar.h, UNICODE for API
#include <tchar.h>

#include <windows.h>
#include <excpt.h>
#include <stdlib.h>
#define __UNKNOWN_APP    0
#define __CONSOLE_APP    1
#define __GUI_APP        2
void __set_app_type(int);
void _controlfp(unsigned a, unsigned b);

#ifdef _UNICODE
#define __tgetmainargs __wgetmainargs
#define _twinstart _wwinstart
#define _runtwinmain _runwwinmain
#define get_tenviron _get_wenviron
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
#else
#define __tgetmainargs __getmainargs
#define _twinstart _winstart
#define _runtwinmain _runwinmain
#define get_tenviron _get_environ
#endif

typedef struct { int newmode; } _startupinfo;
int __cdecl __tgetmainargs(int *pargc, _TCHAR ***pargv, _TCHAR ***penv, int globb, _startupinfo*);
int __cdecl __getmainargs(int *pargc, char ***pargv, char ***penv, int globb, _startupinfo*);
int __cdecl __wgetmainargs(int *pargc, wchar_t ***pargv, wchar_t ***penv, int globb, _startupinfo*);
int __cdecl get_tenviron(_TCHAR ***penv);
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);
__attribute__((weak)) int __cdecl __rt_get_run_argstart(void);

#include "crtinit.c"

typedef struct run_targv_state {
    int saved_argc;
    _TCHAR **saved_argv;
    int active;
    struct run_targv_state *prev;
} run_targv_state;

static __declspec(thread) run_targv_state *run_targv_tls;

static void restore_run_targv(run_targv_state *state)
{
    if (!state || !state->active)
        return;
    __argc = state->saved_argc;
    __targv = state->saved_argv;
    state->active = 0;
    if (run_targv_tls == state)
        run_targv_tls = state->prev;
}

void __tcc_cleanup_run_targv(void)
{
    restore_run_targv(run_targv_tls);
}

static int select_run_arg_start_t(int base, const _TCHAR *arg0, int full_argc, _TCHAR **full_argv)
{
    int i;

    if (base < 0 || base > full_argc)
        return -1;
    if (!arg0)
        return base;
    for (i = base; i < full_argc; ++i) {
        if (full_argv[i] && 0 == _tcscmp(full_argv[i], arg0))
            return i;
    }
    return base;
}

static int find_run_arg_slice(int globb, int *pstart, int *prun_argc)
{
    int literal_argc, full_argc, base, start, ret = 0;
    _TCHAR **literal_argv = NULL, **full_argv = NULL;
    _startupinfo start_info = {0};

    if (!__rt_get_run_argstart)
        return 0;
    base = __rt_get_run_argstart();
    if (base < 0)
        return 0;
    if (__tgetmainargs(&literal_argc, &literal_argv, NULL, 0, &start_info))
        return 0;
    if (base >= literal_argc)
        return 0;
    if (__tgetmainargs(&full_argc, &full_argv, NULL, globb, &start_info))
        return 0;
    start = select_run_arg_start_t(base, literal_argv[base], full_argc, full_argv);
    if (start < 0 || start > full_argc)
        return 0;
    *pstart = start;
    *prun_argc = full_argc - start;
    ret = 1;
    return ret;
}

static int go_winmain(TCHAR *arg1)
{
    STARTUPINFO si;
    _TCHAR **env = NULL;
    _TCHAR *szCmd, *p;
    int fShow;
    int retval;

    GetStartupInfo(&si);
    if (si.dwFlags & STARTF_USESHOWWINDOW)
        fShow = si.wShowWindow;
    else
        fShow = SW_SHOWDEFAULT;

    szCmd = NULL, p = GetCommandLine();
    if (arg1)
        szCmd = _tcsstr(p, arg1);
    if (NULL == szCmd)
        szCmd = _tcsdup(__T(""));
    else if (szCmd > p && szCmd[-1] == __T('"'))
        --szCmd;
#if defined __i386__ || defined __x86_64__
    _controlfp(0x10000, 0x30000);
#endif
    get_tenviron(&env);
    run_ctors(__argc, __targv, env);
    retval = _tWinMain(GetModuleHandle(NULL), NULL, szCmd, fShow);
    run_dtors();
    return retval;
}

static LONG WINAPI catch_sig(EXCEPTION_POINTERS *ex)
{
  return _XcptFilter(ex->ExceptionRecord->ExceptionCode, ex);
}

int _twinstart(void)
{
    _startupinfo start_info_con = {0};
    SetUnhandledExceptionFilter(catch_sig);
    __set_app_type(__GUI_APP);
    __tgetmainargs(&__argc, &__targv, NULL, 0, &start_info_con);
    exit(go_winmain(__argc > 1 ? __targv[1] : NULL));
}

int _runtwinmain(int argc, /* as tcc passed in */ char **argv)
{
    run_targv_state argv_state = { __argc, __targv, 0, NULL };
    int ret;
    int run_arg_start = -1;
#ifdef UNICODE
    {
        int full_argc;
        _TCHAR **full_argv = NULL;
        _startupinfo start_info = {0};

        if (find_run_arg_slice(0, &run_arg_start, &__argc)
            && !__tgetmainargs(&full_argc, &full_argv, NULL, 0, &start_info)
            && run_arg_start <= full_argc)
            __targv = full_argv + run_arg_start;
        else {
            __tgetmainargs(&__argc, &__targv, NULL, 0, &start_info);
            if (argc < __argc)
                __targv += __argc - argc, __argc = argc;
        }
    }
#else
    {
        int full_argc;
        _TCHAR **full_argv = NULL;
        _startupinfo start_info = {0};

        if (find_run_arg_slice(0, &run_arg_start, &__argc)
            && !__tgetmainargs(&full_argc, &full_argv, NULL, 0, &start_info)
            && run_arg_start <= full_argc)
            __targv = full_argv + run_arg_start;
        else
            __argc = argc, __targv = argv;
    }
#endif
    argv_state.prev = run_targv_tls;
    run_targv_tls = &argv_state;
    argv_state.active = 1;
    ret = go_winmain(__argc > 1 ? __targv[1] : NULL);
    __tcc_cleanup_run_targv();
    return ret;
}
