    .text
    .p2align 2
    .global arm64_call_with_dregs

arm64_call_with_dregs:
    .seh_proc arm64_call_with_dregs
    sub sp, sp, 80
    .seh_stackalloc 80
    str x30, [sp, 0]
    .seh_save_reg x30, 0
    str x19, [sp, 8]
    .seh_save_reg x19, 8
    str d8, [sp, 16]
    .seh_save_freg d8, 16
    str d9, [sp, 24]
    .seh_save_freg d9, 24
    str d10, [sp, 32]
    .seh_save_freg d10, 32
    str d11, [sp, 40]
    .seh_save_freg d11, 40
    str d12, [sp, 48]
    .seh_save_freg d12, 48
    str d13, [sp, 56]
    .seh_save_freg d13, 56
    str d14, [sp, 64]
    .seh_save_freg d14, 64
    str d15, [sp, 72]
    .seh_save_freg d15, 72
    .seh_endprologue
    mov x19, x1

    ldr d8, [x0, 0]
    ldr d9, [x0, 8]
    ldr d10, [x0, 16]
    ldr d11, [x0, 24]
    ldr d12, [x0, 32]
    ldr d13, [x0, 40]
    ldr d14, [x0, 48]
    ldr d15, [x0, 56]

    mov x0, x3
    blr x2

    str d8, [x19, 0]
    str d9, [x19, 8]
    str d10, [x19, 16]
    str d11, [x19, 24]
    str d12, [x19, 32]
    str d13, [x19, 40]
    str d14, [x19, 48]
    str d15, [x19, 56]

    ldr d8, [sp, 16]
    ldr d9, [sp, 24]
    ldr d10, [sp, 32]
    ldr d11, [sp, 40]
    ldr d12, [sp, 48]
    ldr d13, [sp, 56]
    ldr d14, [sp, 64]
    ldr d15, [sp, 72]
    ldr x19, [sp, 8]
    ldr x30, [sp, 0]
    add sp, sp, 80
    ret
    .seh_endproc
