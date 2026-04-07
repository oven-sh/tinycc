#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "..\tcc.h"

typedef struct libtcc_api {
    TCCState *(*tcc_new)(void);
    void (*tcc_delete)(TCCState *s);
    int (*tcc_set_options)(TCCState *s, const char *str);
    void (*tcc_set_lib_path)(TCCState *s, const char *path);
    void (*tcc_set_error_func)(TCCState *s, void *opaque, TCCErrorFunc *error_func);
    int (*tcc_set_output_type)(TCCState *s, int output_type);
    int (*tcc_add_include_path)(TCCState *s, const char *pathname);
    int (*tcc_add_library_path)(TCCState *s, const char *pathname);
    int (*tcc_compile_string)(TCCState *s, const char *buf);
    int (*tcc_run)(TCCState *s, int argc, char **argv);
} libtcc_api;

static const char crash_program[] =
    "int main(void)\n"
    "{\n"
    "    *(volatile int *)0 = 1;\n"
    "    return 0;\n"
    "}\n";

static const char argv_program[] =
    "#include <string.h>\n"
    "int main(int argc, char **argv)\n"
    "{\n"
    "    if (argc != 2)\n"
    "        return 10 + argc;\n"
    "    if (strcmp(argv[0], \"beta\") || strcmp(argv[1], \"gamma\"))\n"
    "        return 20;\n"
    "    return 0;\n"
    "}\n";

static void handle_error(void *opaque, const char *msg)
{
    fprintf((FILE *)opaque, "%s\n", msg);
}

static FARPROC load_symbol(HMODULE dll, const char *name)
{
    FARPROC proc = GetProcAddress(dll, name);

    if (!proc) {
        fprintf(stderr, "missing libtcc symbol: %s\n", name);
        exit(1);
    }
    return proc;
}

static int run_program(const libtcc_api *api, const char *source, int run_arg_start)
{
    TCCState *s;
    char *argv[] = { "unused0", "unused1", "unused2", NULL };
    int ret;

    s = api->tcc_new();
    if (!s) {
        fprintf(stderr, "tcc_new failed\n");
        return 100;
    }

    api->tcc_set_error_func(s, stderr, handle_error);
    api->tcc_set_options(s, "-bt");
    api->tcc_set_lib_path(s, ".");
    api->tcc_add_include_path(s, ".\\include");
    api->tcc_add_include_path(s, ".\\win32\\include");
    api->tcc_add_library_path(s, ".\\win32\\lib");
    s->run_arg_start = run_arg_start;

    ret = api->tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if (!ret)
        ret = api->tcc_compile_string(s, source);
    if (!ret)
        ret = api->tcc_run(s, 3, argv);

    api->tcc_delete(s);
    return ret;
}

int main(int argc, char **argv)
{
    HMODULE dll;
    libtcc_api api;
    int ret;

    if (argc < 4) {
        fprintf(stderr, "usage: %s alpha beta gamma\n", argv[0]);
        return 1;
    }

    dll = LoadLibraryA("libtcc.dll");
    if (!dll) {
        fprintf(stderr, "failed to load libtcc.dll\n");
        return 1;
    }

    memset(&api, 0, sizeof(api));
    api.tcc_new = (void *)load_symbol(dll, "tcc_new");
    api.tcc_delete = (void *)load_symbol(dll, "tcc_delete");
    api.tcc_set_options = (void *)load_symbol(dll, "tcc_set_options");
    api.tcc_set_lib_path = (void *)load_symbol(dll, "tcc_set_lib_path");
    api.tcc_set_error_func = (void *)load_symbol(dll, "tcc_set_error_func");
    api.tcc_set_output_type = (void *)load_symbol(dll, "tcc_set_output_type");
    api.tcc_add_include_path = (void *)load_symbol(dll, "tcc_add_include_path");
    api.tcc_add_library_path = (void *)load_symbol(dll, "tcc_add_library_path");
    api.tcc_compile_string = (void *)load_symbol(dll, "tcc_compile_string");
    api.tcc_run = (void *)load_symbol(dll, "tcc_run");

    ret = run_program(&api, crash_program, 1);
    if (ret != 255) {
        fprintf(stderr, "crash program returned %d instead of 255\n", ret);
        FreeLibrary(dll);
        return 2;
    }

    ret = run_program(&api, argv_program, 2);
    if (ret != 0) {
        fprintf(stderr, "argv cleanup test returned %d\n", ret);
        FreeLibrary(dll);
        return 3;
    }

    puts("run argv cleanup ok");
    FreeLibrary(dll);
    return 0;
}
