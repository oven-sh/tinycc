#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "libtcc.h"

typedef struct libtcc_api {
    TCCState *(*tcc_new)(void);
    void (*tcc_delete)(TCCState *s);
    void (*tcc_set_lib_path)(TCCState *s, const char *path);
    void (*tcc_set_error_func)(TCCState *s, void *opaque, TCCErrorFunc *error_func);
    int (*tcc_set_output_type)(TCCState *s, int output_type);
    int (*tcc_add_include_path)(TCCState *s, const char *pathname);
    int (*tcc_add_library_path)(TCCState *s, const char *pathname);
    int (*tcc_compile_string)(TCCState *s, const char *buf);
    int (*tcc_run)(TCCState *s, int argc, char **argv);
} libtcc_api;

typedef struct test_context {
    libtcc_api api;
} test_context;

extern int arm64_call_with_dregs(const double *expected, double *actual,
    int (*fn)(void *opaque), void *opaque);

static const char run_program[] =
    "void exit(int);\n"
    "int main(int argc, char **argv)\n"
    "{\n"
    "    if (argc != 2)\n"
    "        return 11;\n"
    "    if (argv[1][0] != 'x' || argv[1][1] != '\\0')\n"
    "        return 12;\n"
    "    exit(42);\n"
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

static int run_once(void *opaque)
{
    test_context *ctx = (test_context *)opaque;
    TCCState *s;
    char *argv[] = { "arm64_libtcc_context", "x" };
    int ret;

    s = ctx->api.tcc_new();
    if (!s) {
        fprintf(stderr, "tcc_new failed\n");
        return 1;
    }

    ctx->api.tcc_set_error_func(s, stderr, handle_error);
    ctx->api.tcc_set_lib_path(s, ".");
    ctx->api.tcc_add_include_path(s, ".\\include");
    ctx->api.tcc_add_library_path(s, ".\\lib");

    ret = ctx->api.tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if (!ret)
        ret = ctx->api.tcc_compile_string(s, run_program);
    if (!ret)
        ret = ctx->api.tcc_run(s, 2, argv);

    ctx->api.tcc_delete(s);
    return ret;
}

int main(void)
{
    static const double expected[8] = {
        1.25, -2.5, 3.75, -4.875,
        5.5, -6.625, 7.75, -8.875
    };
    double actual[8];
    HMODULE dll;
    test_context ctx;
    int i;

    memset(&ctx, 0, sizeof(ctx));
    dll = LoadLibraryA("libtcc.dll");
    if (!dll) {
        fprintf(stderr, "failed to load libtcc.dll\n");
        return 1;
    }

    ctx.api.tcc_new = (void *)load_symbol(dll, "tcc_new");
    ctx.api.tcc_delete = (void *)load_symbol(dll, "tcc_delete");
    ctx.api.tcc_set_lib_path = (void *)load_symbol(dll, "tcc_set_lib_path");
    ctx.api.tcc_set_error_func = (void *)load_symbol(dll, "tcc_set_error_func");
    ctx.api.tcc_set_output_type = (void *)load_symbol(dll, "tcc_set_output_type");
    ctx.api.tcc_add_include_path = (void *)load_symbol(dll, "tcc_add_include_path");
    ctx.api.tcc_add_library_path = (void *)load_symbol(dll, "tcc_add_library_path");
    ctx.api.tcc_compile_string = (void *)load_symbol(dll, "tcc_compile_string");
    ctx.api.tcc_run = (void *)load_symbol(dll, "tcc_run");

    for (i = 0; i < 32; ++i) {
        memset(actual, 0, sizeof(actual));
        if (arm64_call_with_dregs(expected, actual, run_once, &ctx) != 42) {
            fprintf(stderr, "tcc_run did not return 42 on iteration %d\n", i);
            FreeLibrary(dll);
            return 1;
        }
        if (memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(stderr, "nonvolatile FP registers were not restored on iteration %d\n", i);
            FreeLibrary(dll);
            return 1;
        }
    }

    puts("arm64 libtcc context ok");
    FreeLibrary(dll);
    return 0;
}
