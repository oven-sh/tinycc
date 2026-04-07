/* ---------------------------------------------- */
/* chkstk86.s */

#ifdef __leading_underscore
# define _(s) _##s
#else
# define _(s) s
#endif

/* ---------------------------------------------- */
#if defined(__aarch64__)
/* ---------------------------------------------- */

.globl __chkstk
__chkstk:
    /* Windows ARM64 stack probing helper.
       arm64-gen.c passes the requested frame size in x15, scaled in 16-byte
       units. Probe one 4 KiB page at a time and leave SP unchanged; the caller
       subtracts SP after the probe returns. */
    mov     x16, sp
    lsl     x17, x15, 4
    cbz     x17, L_chkstk_done
L_chkstk_loop:
    subs    x0, x17, 4096
    bls     L_chkstk_tail
    sub     x16, x16, 4096
    ldr     xzr, [x16]
    sub     x17, x17, 4096
    b       L_chkstk_loop
L_chkstk_tail:
    sub     x16, x16, x17
    ldr     xzr, [x16]
L_chkstk_done:
    ret

.globl _(tinyc_getbp)
_(tinyc_getbp):
    mov     x0, x29
    ret

.globl _(mingw_getsp)
_(mingw_getsp):
    mov     x0, sp
    ret

.globl _(__mingw_setjmp)
_(__mingw_setjmp):
    /* _JUMP_BUFFER layout matches win32/include/setjmp.h for _ARM64_:
       0x00 Frame, 0x08 Reserved, 0x10-0x68 X19-X30, 0x70 Sp,
       0x78 Fpcr/Fpsr, 0x80-0xB8 D8-D15. */
    str     xzr, [x0]       /* Frame = 0 */
    stp     x19, x20, [x0, 16]
    stp     x21, x22, [x0, 32]
    stp     x23, x24, [x0, 48]
    stp     x25, x26, [x0, 64]
    stp     x27, x28, [x0, 80]
    stp     x29, x30, [x0, 96]
    mov     x2, sp
    str     x2, [x0, 112]   /* Sp */
    mrs     x2, FPCR
    str     w2, [x0, 120]   /* Fpcr */
    mrs     x2, FPSR
    str     w2, [x0, 124]   /* Fpsr */
    stp     d8, d9, [x0, 128]
    stp     d10, d11, [x0, 144]
    stp     d12, d13, [x0, 160]
    stp     d14, d15, [x0, 176]
    mov     x0, 0
    ret

.globl _(__mingw_longjmp)
_(__mingw_longjmp):
    ldp     x19, x20, [x0, 16]
    ldp     x21, x22, [x0, 32]
    ldp     x23, x24, [x0, 48]
    ldp     x25, x26, [x0, 64]
    ldp     x27, x28, [x0, 80]
    ldp     x29, x30, [x0, 96]
    ldr     x2, [x0, 112]   /* Sp */
    mov     sp, x2
    ldr     w2, [x0, 120]   /* Fpcr */
    msr     FPCR, x2
    ldr     w2, [x0, 124]   /* Fpsr */
    msr     FPSR, x2
    ldp     d8, d9, [x0, 128]
    ldp     d10, d11, [x0, 144]
    ldp     d12, d13, [x0, 160]
    ldp     d14, d15, [x0, 176]
    mov     x0, x1
    cbnz    x0, L_longjmp_done
    mov     x0, 1
L_longjmp_done:
    ret

/* ---------------------------------------------- */
#elif !defined(__x86_64__)
/* ---------------------------------------------- */

.globl _(__chkstk)
_(__chkstk):
    xchg    (%esp),%ebp     /* store ebp, get ret.addr */
    push    %ebp            /* push ret.addr */
    lea     4(%esp),%ebp    /* setup frame ptr */
    push    %ecx            /* save ecx */
    mov     %ebp,%ecx
P0:
    sub     $4096,%ecx
    test    %eax,(%ecx)
    sub     $4096,%eax
    cmp     $4096,%eax
    jge     P0
    sub     %eax,%ecx
    test    %eax,(%ecx)

    mov     %esp,%eax
    mov     %ecx,%esp
    mov     (%eax),%ecx     /* restore ecx */
    jmp     *4(%eax)

/* ---------------------------------------------- */
#else
/* ---------------------------------------------- */

.globl _(__chkstk)
_(__chkstk):
    xchg    (%rsp),%rbp     /* store ebp, get ret.addr */
    push    %rbp            /* push ret.addr */
    lea     8(%rsp),%rbp    /* setup frame ptr */
    push    %rcx            /* save ecx */
    mov     %rbp,%rcx
    movslq  %eax,%rax
P0:
    sub     $4096,%rcx
    test    %rax,(%rcx)
    sub     $4096,%rax
    cmp     $4096,%rax
    jge     P0
    sub     %rax,%rcx
    test    %rax,(%rcx)

    mov     %rsp,%rax
    mov     %rcx,%rsp
    mov     (%rax),%rcx     /* restore ecx */
    jmp     *8(%rax)

/* ---------------------------------------------- */
/* setjmp/longjmp support */

.globl _(tinyc_getbp)
_(tinyc_getbp):
    mov %rbp,%rax
    ret

/* ---------------------------------------------- */
#endif
/* ---------------------------------------------- */
