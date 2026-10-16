/* ------------------------------------------------------------------ */
/*
 *  LOONGARCH64 assembler token table for TCC  (phase-1 minimal)
 *
 *  Phase 1 of the LoongArch64 backend ships no real assembler
 *  (loongarch64-asm.c is a stub), so no instruction mnemonics are
 *  defined here.  Only the tokens the compiler core itself references
 *  are provided:
 *
 *    TOK_ASM_push / TOK_ASM_pop  -- used by tccpp.c's pragma_parse()
 *        for `#pragma pack(push)` / `#pragma pack(pop)`.
 *
 *  When a real LoongArch64 assembler is implemented (phase 2),
 *  extend this file with the full mnemonic table, modeled on
 *  riscv64-tok.h.
 */

/* pragma pack push/pop (tccpp.c pragma_parse) */
DEF_ASM(push)
DEF_ASM(pop)
