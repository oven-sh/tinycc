/*
 *  TCC runtime library for riscv64.
 *
 *  Copyright (c) 2026
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.  This file is offered as-is,
 * without any warranty.
 */

/* ------------------------------------------------------------- */
/* __clear_cache is used in tccrun.c.  It is a built-in
   intrinsic with gcc.  However tcc in order to compile
   itself needs this function */

void __clear_cache(void *beg, void *end)
{
    __riscv64_clear_cache(beg, end);
}
