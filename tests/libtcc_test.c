/*
 * Simple Test program for libtcc
 *
 * libtcc can be useful to use tcc as a "backend" for a code generator.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libtcc.h"

void handle_error(void *opaque, const char *msg)
{
    fprintf(opaque, "%s\n", msg);
}

/* this function is called by the generated code */
int add(int a, int b)
{
    return a + b;
}

/* this strinc is referenced by the generated code */
const char hello[] = "Hello World!";

char my_program[] =
"#include <tcclib.h>\n" /* include the "Simple libc header for TCC" */
"#include \"my_header.h\"\n" /* served from memory by open_file() */
"extern int add(int a, int b);\n"
"#ifdef _WIN32\n" /* dynamically linked data needs 'dllimport' */
" __attribute__((dllimport))\n"
"#endif\n"
"extern const char hello[];\n"
"int fib(int n)\n"
"{\n"
"    if (n <= 2)\n"
"        return 1;\n"
"    else\n"
"        return fib(n-1) + fib(n-2);\n"
"}\n"
"\n"
"int foo(int n)\n"
"{\n"
"    printf(\"%s\\n\", hello);\n"
"    printf(\"fib(%d) = %d\\n\", n, fib(n));\n"
"    printf(\"add(%d, %d) = %d\\n\", n, 2 * n, add(n, 2 * n));\n"
"    printf(\"twice(%d) = %d\\n\", n, twice(n));\n"
"    return 0;\n"
"}\n";

/* these two files only exist in memory; open_file() hands them to tcc */
const char my_header[] =
"extern int twice(int n);\n";

const char my_twice[] =
"int twice(int n)\n"
"{\n"
"    return 2 * n;\n"
"}\n";

int open_file(void *opaque, const char *filename, const char **buf, unsigned long *len)
{
    if (!strcmp(filename, "my_header.h")) {
        *buf = my_header;
        *len = strlen(my_header);
        return 0;
    }
    if (!strcmp(filename, "virtual/twice.c")) {
        *buf = my_twice;
        *len = strlen(my_twice);
        return 0;
    }
    /* anything else (tcclib.h, ...) comes from the file system */
    return -1;
}

int main(int argc, char **argv)
{
    TCCState *s;
    int i;
    int (*func)(int);

    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* set custom error/warning printer */
    tcc_set_error_func(s, stderr, handle_error);

    /* serve some source files from memory */
    tcc_set_open_func(s, NULL, open_file);

    /* if tcclib.h and libtcc1.a are not installed, where can we find them */
    for (i = 1; i < argc; ++i) {
        char *a = argv[i];
        if (a[0] == '-') {
            if (a[1] == 'B')
                tcc_set_lib_path(s, a+2);
            else if (a[1] == 'I')
                tcc_add_include_path(s, a+2);
            else if (a[1] == 'L')
                tcc_add_library_path(s, a+2);
        }
    }

    /* MUST BE CALLED before any compilation */
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    if (tcc_compile_string(s, my_program) == -1)
        return 1;

    /* this file does not exist on disk, open_file() provides it */
    if (tcc_add_file(s, "virtual/twice.c") == -1)
        return 1;

    /* as a test, we add symbols that the compiled program can use.
       You may also open a dll with tcc_add_dll() and use symbols from that */
    tcc_add_symbol(s, "add", add);
    tcc_add_symbol(s, "hello", hello);

    /* relocate the code */
    if (tcc_relocate(s) < 0)
        return 1;

    /* get entry symbol */
    func = tcc_get_symbol(s, "foo");
    if (!func)
        return 1;

    /* run the code */
    func(32);

    /* delete the state */
    tcc_delete(s);

    return 0;
}
