/*
 *  LoongArch64 code generator for TCC
 *
 *  Translated from riscv64-gen.c.  LoongArch instruction encoding:
 *    3R:    rd[4:0] | rj[9:5] | rk[14:10]
 *    2RI12: rd[4:0] | rj[9:5] | imm12[21:10]
 *    shift: rd[4:0] | rj[9:5] | ui5/6[15:10]
 *    branch:rd[4:0] | rj[9:5] | offs16[25:10]  (offs = byte offset >> 2)
 *    jirl:  rd[4:0] | rj[9:5] | offs16[25:10]
 *    lu12i.d / pcalau12i: rd[4:0] | si20[24:5]
 *
 *  Register mapping (LP64D):
 *    TCC int regs 0..7   -> r4..r11 (a0-a7, argument regs)
 *    TCC float regs 8..15 -> f0..f7 (fa0-fa7)
 *    TREG_RA -> r1, TREG_SP -> r3
 */

#ifdef TARGET_DEFS_ONLY

// Number of registers available to allocator:
#define NB_REGS 19 // r4-r11 aka a0-a7, f0-f7 aka fa0-fa7, xxx, ra, sp
/* tccasm.c handles directives and labels; loongarch64-asm.c has no
   instruction encoder yet, so mnemonics and inline asm are rejected there */
#define CONFIG_TCC_ASM

#define TREG_R(x) (x) // x = 0..7
#define TREG_F(x) (x + 8) // x = 0..7

// Register classes sorted from more general to more precise:
#define RC_INT (1 << 0)
#define RC_FLOAT (1 << 1)
#define RC_R(x) (1 << (2 + (x))) // x = 0..7
#define RC_F(x) (1 << (10 + (x))) // x = 0..7

#define RC_IRET (RC_R(0)) // int return register class
#define RC_IRE2 (RC_R(1)) // int 2nd return register class
#define RC_FRET (RC_F(0)) // float return register class

#define REG_IRET (TREG_R(0)) // int return register number
#define REG_IRE2 (TREG_R(1)) // int 2nd return register number
#define REG_FRET (TREG_F(0)) // float return register number

#define PTR_SIZE 8

#define LDOUBLE_SIZE 16
#define LDOUBLE_ALIGN 16

#define MAX_ALIGN 16

/* define if return values need to be extended explicitely
   at caller side (for interfacing with non-TCC compilers) */
#define PROMOTE_RET

#else
#define USING_GLOBALS
#include "tcc.h"
#include <assert.h>


#define XLEN 8

#define TREG_RA 17
#define TREG_SP 18

/* ---- instruction encodings (verified from gcc 15 objdump) ---- */
/* 3R */
#define O_ADD_D  0x00108000u
#define O_SUB_D  0x00118000u
#define O_AND    0x00148000u
#define O_OR     0x00150000u
#define O_XOR    0x00158000u
#define O_SLL_D  0x00188000u
#define O_SRL_D  0x00190000u
#define O_SRA_D  0x00198000u
#define O_MUL_D  0x001d8000u
#define O_DIV_D  0x00220000u
#define O_MOD_D  0x00228000u
#define O_DIV_DU 0x00230000u
#define O_MOD_DU 0x00238000u
#define O_ADD_W  0x00100000u
#define O_SUB_W  0x00110000u
#define O_SLL_W  0x00170000u
#define O_SRL_W  0x00178000u
#define O_SRA_W  0x00180000u
#define O_MUL_W  0x001c0000u
#define O_DIV_W  0x00200000u
#define O_MOD_W  0x00208000u
#define O_DIV_WU 0x00210000u
#define O_MOD_WU 0x00218000u
#define O_SLT    0x00120000u
#define O_SLTU   0x00128000u
#define O_FADD_D 0x01010000u
#define O_FSUB_D 0x01030000u
#define O_FMUL_D 0x01050000u
#define O_FDIV_D 0x01070000u
#define O_FADD_S 0x01008000u
#define O_FSUB_S 0x01028000u
#define O_FMUL_S 0x01048000u
#define O_FDIV_S 0x01068000u
#define O_FCMP_D 0x0c200000u  /* fcmp.cond.d cd, fj, fk: | cond<<15 */
#define O_FCMP_S 0x0c100000u
/* 2RI12 */
#define O_ADDI_D 0x02c00000u
#define O_ADDI_W 0x02800000u
#define O_ANDI   0x03400000u
#define O_ORI    0x03800000u
#define O_XORI   0x03c00000u
#define O_SLTI   0x02000000u
#define O_SLTUI  0x02400000u
#define O_LD_B   0x28000000u
#define O_LD_H   0x28400000u
#define O_LD_W   0x28800000u
#define O_LD_D   0x28c00000u
#define O_LD_BU  0x2a000000u
#define O_LD_HU  0x2a400000u
#define O_ST_B   0x29000000u   /* binutils 权威: st 在 0x29x00000 区间 */
#define O_ST_H   0x29400000u
#define O_ST_W   0x29800000u
#define O_ST_D   0x29c00000u
#define O_FLD_S  0x2b000000u
#define O_FLD_D  0x2b800000u
#define O_FST_S  0x2b400000u
#define O_FST_D  0x2bc00000u
#define O_LDPTR_W 0x24000000u
#define O_LDPTR_D 0x26000000u
#define O_STPTR_W 0x25000000u
#define O_STPTR_D 0x27000000u
/* shift */
#define O_SLLI_W 0x00408000u
#define O_SLLI_D 0x00410000u
#define O_SRLI_W 0x00448000u
#define O_SRAI_W 0x00488000u
#define O_SRLI_D 0x00450000u
#define O_SRAI_D 0x00490000u
/* branch / jump */
#define O_B      0x50000000u
#define O_BL     0x54000000u
#define O_BEQ    0x58000000u
#define O_BNE    0x5c000000u
#define O_BLT    0x60000000u
#define O_BGE    0x64000000u
#define O_BLTU   0x68000000u
#define O_BGEU   0x6c000000u
#define O_JIRL   0x4c000000u
#define O_BEQZ   0x40000000u   /* beqz rj, si21: op[31:26]=010000 (原误写 0x58000000=beq) */
#define O_BNEZ   0x44000000u
/* load-address / misc */
#define O_LU12I_D 0x14000000u  /* 实为 lu12i.w: rd = sext32(si20 << 12) */
#define O_LU32I_D 0x16000000u  /* lu32i.d: rd[51:32] = si20 */
#define O_LU52I_D 0x03000000u  /* lu52i.d: rd[63:52] = si12 (2RI12 格式) */
#define O_PCALAU12I 0x1a000000u /* pcalau12i: rd = PC + sext(si20<<12), 配 R_LARCH_PCALA_HI20/LO12
                                   (原误写 0x16000000=lu32i.d; 0x1c000000 是 pcaddu12i 配 PCADD_HI20) */
#define O_NOP    0x03400000u
/* float moves / conversions —— 模板以 binutils loongarch-opc.c 为准,
   低 10 位(fd/rj 字段位)必须为 0(原值从具体反汇编抄来, 字段未清零) */
#define O_MOVGR2FR_D 0x0114a800u  /* movgr2fr.d fd, rj:  fd[4:0] rj[9:5] */
#define O_MOVGR2FR_W 0x0114a400u  /* movgr2fr.w fd, rj */
#define O_MOVFR2GR_D 0x0114b800u  /* movfr2gr.d rd, fj:  rd[4:0] fj[9:5] */
#define O_MOVFR2GR_S 0x0114b400u  /* movfr2gr.s rd, fj */
#define O_MOVCF2GR   0x0114dc00u  /* movcf2gr rd, cj:    rd[4:0] cj[9:5]
                                    (原误用 0x0114d400 = movcf2fr 写浮点寄存器) */
#define O_FMOV_S     0x01149400u  /* fmov.s fd, fj:      fd[4:0] fj[9:5] */
#define O_FMOV_D     0x01149800u  /* fmov.d fd, fj:      fd[4:0] fj[9:5] */
#define O_FCMP_SLT_D 0x0c218000u  /* fcmp.slt.d cd, fj, fk: cd[4:0] fj[9:5] fk[14:10]
                                    (原 0x0c218020 带 fj=1 脏字段) */
#define O_FCMP_SLE_D 0x0c238000u  /* fcmp.sle.d (cond=7) */
#define O_FCMP_SEQ_D 0x0c228000u  /* fcmp.seq.d (cond=5) */
#define O_FFINT_D_L  0x011d2800u  /* ffint.d.l fd, fj:   fd[4:0] fj[9:5] */
#define O_FFINT_S_L  0x011d1800u  /* ffint.s.l fd, fj */
#define O_FCMP_CLT_D 0x0c210000u  /* fcmp.clt.d cd, fj, fk */
#define O_BCNEZ      0x48000100u  /* bcnez cj, offs21: cj[7:5] offs[15:0]@[25:10] offs[20:16]@[4:0] */
#define O_FTINTRZ_L_D 0x011aa800u /* ftintrz.l.d fd, fj */
#define O_FCVT_S_D   0x01191800u  /* fcvt.s.d fd, fj */
#define O_FCVT_D_S   0x01192400u  /* fcvt.d.s fd, fj */

/* The LP64D subset of GCC's predefined macros (gcc -dM -E on loongarch64).
   glibc's <gnu/stubs.h> picks stubs-lp64s.h on mere *definedness* of
   __loongarch_soft_float, so that macro must not exist at all. */
ST_DATA const char * const target_machine_defs =
    "__loongarch__ 1\0"
    "__loongarch64 1\0"
    "__loongarch_grlen 64\0"
    "__loongarch_frlen 64\0"
    "__loongarch_lp64 1\0"
    "__loongarch_hard_float 1\0"
    "__loongarch_double_float 1\0"
    "_ABILP64 3\0"
    "_LOONGARCH_SIM _ABILP64\0"
    "_LOONGARCH_SZINT 32\0"
    "_LOONGARCH_SZLONG 64\0"
    "_LOONGARCH_SZPTR 64\0"
    ;

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_R(0),
  RC_INT | RC_R(1),
  RC_INT | RC_R(2),
  RC_INT | RC_R(3),
  RC_INT | RC_R(4),
  RC_INT | RC_R(5),
  RC_INT | RC_R(6),
  RC_INT | RC_R(7),
  RC_FLOAT | RC_F(0),
  RC_FLOAT | RC_F(1),
  RC_FLOAT | RC_F(2),
  RC_FLOAT | RC_F(3),
  RC_FLOAT | RC_F(4),
  RC_FLOAT | RC_F(5),
  RC_FLOAT | RC_F(6),
  RC_FLOAT | RC_F(7),
  0,
  1 << TREG_RA,
  1 << TREG_SP
};

#if defined(CONFIG_TCC_BCHECK)
static addr_t func_bound_offset;
static unsigned long func_bound_ind;
ST_DATA int func_bound_add_epilog;
#endif

static int ireg(int r)
{
    if (r == TREG_RA)
      return 1; // ra
    if (r == TREG_SP)
      return 3; // sp
    assert(r >= 0 && r < 8);
    return r + 4;  // tccrX --> aX == r(4+X)
}

static int is_ireg(int r)
{
    return (unsigned)r < 8 || r == TREG_RA || r == TREG_SP;
}

static int freg(int r)
{
    assert(r >= 8 && r < 16);
    return r - 8;  // tccfX --> faX == fX
}

static int is_freg(int r)
{
    return r >= 8 && r < 16;
}

/* emit 32-bit instruction */
ST_FUNC void o(unsigned int c)
{
    int ind1 = ind + 4;
    if (nocode_wanted)
        return;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    write32le(cur_text_section->data + ind, c);
    ind = ind1;
}

/* 3R: rd[4:0] rj[9:5] rk[14:10] */
static void o3r(uint32_t base, int rd, int rj, int rk)
{
    o(base | (rk << 10) | (rj << 5) | rd);
}

/* 2RI12: rd[4:0] rj[9:5] imm12[21:10] */
static void o2ri(uint32_t base, int rd, int rj, int imm12)
{
    o(base | ((imm12 & 0xfff) << 10) | (rj << 5) | rd);
}

/* shift: rd[4:0] rj[9:5] ui5/6[15:10] */
static void oshift(uint32_t base, int rd, int rj, int ui)
{
    o(base | ((ui & 0x3f) << 10) | (rj << 5) | rd);
}

/* branch: rd[4:0] rj[9:5] offs16[25:10], offs = byte offset >> 2 */
static void obranch(uint32_t base, int rj, int rd, int offs)
{
    o(base | ((offs & 0xffff) << 10) | (rj << 5) | rd);
}

/* jirl rd, rj, offs */
static void ojirl(int rd, int rj, int offs)
{
    o(O_JIRL | ((offs & 0xffff) << 10) | (rj << 5) | rd);
}

/* lu12i.d / pcalau12i: rd[4:0] si20[24:5] */
static void oli20(uint32_t base, int rd, int si20)
{
    o(base | ((si20 & 0xfffff) << 5) | rd);
}

/* move: or rd, rj, r0 */
static void omove(int rd, int rj)
{
    o3r(O_OR, rd, rj, 0);
}

/* 12-bit signed check */
static int imm12_ok(int64_t v)
{
    return v >= -2048 && v <= 2047;
}

/* ---- register save/restore for calls: LoongArch stores args to stack
   frame in prolog, we keep the same scheme as riscv64 (s0 = fp) ---- */

// Patch all branches in list pointed to by t to branch to a:
ST_FUNC void gsym_addr(int t_, int a_)
{
    uint32_t t = t_;
    uint32_t a = a_;
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t next = read32le(ptr);
        uint32_t r = a - t;   /* byte offset */
        int offs;
        /* b 的 si26 支持 ±128MB; 原检查按 ±128KB 写死会误报 */
        if ((r + (1 << 27)) & ~(uint32_t)((1U << 28) - 4))
          tcc_error("out-of-range branch chain");
        offs = (int)(r >> 2);
        if (offs == 1) {
            write32le(ptr, O_NOP);   /* fall through -> nop */
        } else {
            /* b 的 si26: bits[25:10]=si26[15:0], bits[9:0]=si26[25:16]
               (原代码丢了高 10 位: 负偏移(循环回跳)变成 +256KB 乱跳) */
            write32le(ptr, O_B | ((offs & 0xffff) << 10) | ((offs >> 16) & 0x3ff));
        }
        t = next;
    }
}
static void load_large_constant(int rr, int fc, int hi32);

/* Load the address of sv (symbol / local) into a register, return reg.
   new_fc receives the residual 12-bit offset (usually 0). */
static int load_symofs(int r, SValue *sv, int forstore, int *new_fc)
{
    int rr;
    int fc = sv->c.i, v = sv->r & VT_VALMASK;
    if (sv->r & VT_SYM) {
        assert(v == VT_CONST);
        rr = is_ireg(r) ? ireg(r) : 12;  /* r12 = t0 (LoongArch 的 r5 是 a1 参数寄存器, 不能当 scratch) */
        if (sv->sym->type.t & VT_TLS) {
            /* TLS local-exec: lu12i.w rr, %le_hi20(sym+A) ;
               ori rr, rr, %le_lo12(sym+A) ; add.d rr, rr, $tp */
            greloca(cur_text_section, sv->sym, ind, R_LARCH_TLS_LE_HI20, sv->c.i);
            oli20(O_LU12I_D, rr, 0);
            greloca(cur_text_section, sv->sym, ind, R_LARCH_TLS_LE_LO12, sv->c.i);
            o2ri(O_ORI, rr, rr, 0);
            o3r(O_ADD_D, rr, rr, 2);  /* $tp = $r2 */
            *new_fc = 0;
        } else if (sv->sym->type.t & VT_STATIC) {
            /* pcalau12i rr, %pc_hi20(sym+A) ; addi.d rr, rr, %pc_lo12(sym+A)
               (page-aligned pcalau12i; both relocs name the same symbol) */
            greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_HI20, sv->c.i);
            oli20(O_PCALAU12I, rr, 0);
            greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_LO12, sv->c.i);
            o2ri(O_ADDI_D, rr, rr, 0);
            *new_fc = 0;
        } else {
            /* Any other symbol may live outside +-2GB of this code: a
               libc symbol, or one given by tcc_add_symbol() when the code
               runs from memory. Load its address from the GOT:
               pcalau12i rr, %got_pc_hi20(sym) ; ld.d rr, rr, %got_pc_lo12(sym)
               and apply the addend afterwards. */
            greloca(cur_text_section, sv->sym, ind, R_LARCH_GOT_PC_HI20, 0);
            oli20(O_PCALAU12I, rr, 0);
            greloca(cur_text_section, sv->sym, ind, R_LARCH_GOT_PC_LO12, 0);
            o2ri(O_LD_D, rr, rr, 0);
            if (imm12_ok(fc)) {
                *new_fc = fc;
            } else {
                load_large_constant(13, fc, fc < 0 ? -1 : 0);  /* t1 */
                o3r(O_ADD_D, rr, rr, 13);
                *new_fc = 0;
            }
        }
        return rr;
    } else if (v == VT_LOCAL || v == VT_LLOCAL) {
        rr = 22; /* fp */
        if (fc != sv->c.i)
          tcc_error("unimp: store(giant local off) (0x%lx)", (long)sv->c.i);
        if (!imm12_ok(fc)) {
            rr = is_ireg(r) ? ireg(r) : 12;  /* r12 = t0 */
            oli20(O_LU12I_D, rr, (int)((fc + 0x800) >> 12));
            o2ri(O_ADDI_D, rr, rr, fc);
            o3r(O_ADD_D, rr, rr, 22);
            *new_fc = 0;
        }
    } else
      tcc_error("uhh");
    return rr;
}

/* Load 64-bit constant into register rr (lu12i.w + addi.d [+ lu32i.d + lu52i.d]) */
static void load_large_constant(int rr, int fc, int hi32)
{
    int v;
    v = (int)(((uint32_t)fc + 0x800) >> 12);
    oli20(O_LU12I_D, rr, v);
    /* 低位必须用 addi.d 而非 ori: lu12i.w 按 (fc+0x800)>>12 进位,
       低 12 位需要符号补偿; ori 是逻辑或, fc 的 bit11=1 时产生错误值 */
    o2ri(O_ADDI_D, rr, rr, fc);
    /* lu12i.w 把 32 位结果符号扩展到 64 位: 当目标值高 32 位应为 0 但
       bit31=1 (fc<0, 如无符号 32 位常量)或 +0x800 进位使 bit31 变 1
       (v & 0x80000, 如 0x7fffff00)时, 必须再用 lu32i.d/lu52i.d 清零高位,
       否则得到 0xffffffff_8xxxxxxx 之类的错误值 */
    if (hi32 || fc < 0 || (v & 0x80000)) {
        oli20(O_LU32I_D, rr, hi32);          /* rd[51:32] = hi32 低 20 位 */
        o2ri(O_LU52I_D, rr, rr, hi32 >> 20); /* rd[63:52] = hi32 高 12 位 */
    }
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r;
    int v = fr & VT_VALMASK;
    int rr = is_ireg(r) ? ireg(r) : freg(r);
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size;
    if (fr & VT_LVAL) {
        uint32_t opcode;
        int br;
        size = type_size(&sv->type, &align);
        assert(!is_freg(r) || bt == VT_FLOAT || bt == VT_DOUBLE);
        if (bt == VT_PTR || bt == VT_FUNC)
          size = PTR_SIZE;
        if (is_freg(r))
            opcode = (size == 4) ? O_FLD_S : O_FLD_D;
        else
            opcode = size == 1 ? O_LD_B : size == 2 ? O_LD_H
                   : size == 4 ? O_LD_W : O_LD_D;
        /* unsigned char/short are zero-extended; 32-bit values (unsigned
           too) stay sign-extended in registers, as the psABI and tccgen's
           RISC-V-style 32-bit handling both expect */
        if (size < 4 && !is_float(sv->type.t) && (sv->type.t & VT_UNSIGNED))
            opcode = size == 1 ? O_LD_BU : O_LD_HU;
        if (v == VT_LOCAL || (fr & VT_SYM)) {
            br = load_symofs(r, sv, 0, &fc);
        } else if (v < VT_CONST) {
            br = ireg(v);
            fc = 0;
        } else if (v == VT_LLOCAL) {
            br = load_symofs(r, sv, 0, &fc);
            o2ri(O_LD_D, rr, br, fc);
            br = rr;
            fc = 0;
        } else if (v == VT_CONST) {
            int64_t si = sv->c.i;
            br = rr;
            load_large_constant(rr, (int)si, (int)((uint64_t)si >> 32));
            fc = 0;
        } else {
            tcc_error("unimp: load(non-local lval)");
        }
        o2ri(opcode, rr, br, fc);
    } else if (v == VT_CONST) {
        /* CValue.i 是 uint64_t: 完整 64 位常量必须按 64 位判断,
           否则 0x8000000000000000 之类被 int 截断成 0, 误判为
           "12 位可表示" 直接 addi.d 0, 高位全丢 (与 gen_opil 同理) */
        int64_t cv = (int64_t)sv->c.i;
        int rb = 0;
        /* 32-bit values live sign-extended in registers, unsigned int
           too, so 0x80000000u must become 0xffffffff80000000 to compare
           equal to a computed unsigned int (riscv64 loads it with addiw) */
        if (bt != VT_LLONG && bt != VT_PTR && bt != VT_FUNC && !(fr & VT_SYM)
            && !is_float(sv->type.t))
            cv = (int32_t)cv;
        if (is_float(sv->type.t) && bt != VT_LDOUBLE) {
            /* load float/double constant: move bit pattern from int reg */
            uint64_t val = sv->c.i;
            load_large_constant(13, (int)val, (int)((uint64_t)val >> 32));
            o(O_MOVGR2FR_D | (13 << 5) | rr);  /* movgr2fr.d rr(fd), t1(rj) */
            return;
        }
        assert(is_ireg(r) || bt == VT_LDOUBLE);
        if (fr & VT_SYM) {
            rb = load_symofs(r, sv, 0, &fc);
        } else if (!imm12_ok(cv)) {
            load_large_constant(rr, (int)cv, (int)((uint64_t)cv >> 32));
            fc = 0;
            rb = rr;
        } else {
            rb = 0;  /* zero */
        }
        o2ri(O_ADDI_D, rr, rb, fc);
    } else if (v == VT_LOCAL) {
        int br = load_symofs(r, sv, 0, &fc);
        assert(is_ireg(r));
        o2ri(O_ADDI_D, rr, br, fc);
    } else if (v < VT_CONST) { /* reg-reg */
        if (is_freg(r) && is_freg(v)) {
            o(O_FMOV_D | (freg(v) << 5) | rr);  /* fmov.d rr, freg(v)
                (原用 fadd.d rr, fj, f0 —— f0 是 fa0 参数寄存器, 不是 0.0!) */
        } else if (is_ireg(r) && is_ireg(v)) {
            omove(rr, ireg(v));
        } else {
            if (is_ireg(r))  /* float -> int: movfr2gr.d rd=rr, fj=freg(v) */
                o(O_MOVFR2GR_D | ((is_freg(v) ? freg(v) : 0) << 5) | rr);
            else  /* int -> float: movgr2fr.d fd=rr, rj=ireg(v) */
                o(O_MOVGR2FR_D | ((is_ireg(v) ? ireg(v) : 0) << 5) | rr);
        }
    } else if (v == VT_CMP) {
        int op = vtop->cmp_op;
        int a = vtop->cmp_r & 0xff;
        int b = (vtop->cmp_r >> 8) & 0xff;
        int inv = 0;
        switch (op) {
            case TOK_ULT:
            case TOK_UGE:
            case TOK_ULE:
            case TOK_UGT:
            case TOK_LT:
            case TOK_GE:
            case TOK_LE:
            case TOK_GT:
                if (op & 1) { inv = 1; op--; }
                if ((op & 7) == 6) { int t = a; a = b; b = t; inv ^= 1; }
                /* TOK_LT..TOK_GT (signed) sort after TOK_ULT..TOK_UGT */
                o3r((op > TOK_UGT) ? O_SLT : O_SLTU, rr, a, b);
                if (inv)
                    o2ri(O_XORI, rr, rr, 1);
                break;
            case TOK_NE:
            case TOK_EQ:
                if (rr != a || b)
                    o3r(O_SUB_D, rr, a, b);
                if (op == TOK_NE)
                    o3r(O_SLTU, rr, 0, rr);
                else
                    o2ri(O_SLTUI, rr, rr, 1);
                break;
        }
    } else if ((v & ~1) == VT_JMP) {
        int t = v & 1;
        assert(is_ireg(r));
        o2ri(O_ADDI_D, rr, 0, t);
        gjmp_addr(ind + 8);
        gsym(fc);
        o2ri(O_ADDI_D, rr, 0, t ^ 1);
    } else
      tcc_error("unimp: load(non-const)");
}

ST_FUNC void store(int r, SValue *sv)
{
    int fr = sv->r & VT_VALMASK;
    int rr = is_ireg(r) ? ireg(r) : freg(r), ptrreg;
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size = type_size(&sv->type, &align);
    uint32_t opcode;
    assert(!is_float(bt) || is_freg(r) || bt == VT_LDOUBLE);
    if (bt == VT_LDOUBLE)
      size = align = 8;
    if (bt == VT_STRUCT)
      tcc_error("unimp: store(struct)");
    if (size > 8)
      tcc_error("unimp: large sized store");
    assert(sv->r & VT_LVAL);
    if (is_freg(r))
        opcode = (size == 4) ? O_FST_S : O_FST_D;
    else
        opcode = size == 1 ? O_ST_B : size == 2 ? O_ST_H
               : size == 4 ? O_ST_W : O_ST_D;
    if (fr == VT_LOCAL || (sv->r & VT_SYM)) {
        ptrreg = load_symofs(-1, sv, 1, &fc);
    } else if (fr < VT_CONST) {
        ptrreg = ireg(fr);
        fc = 0;
    } else if (fr == VT_CONST) {
        int64_t si = sv->c.i;
        ptrreg = 12;  /* t0 */
        load_large_constant(ptrreg, (int)si, (int)((uint64_t)si >> 32));
        fc = 0;
    } else
      tcc_error("implement me: %s(!local)", __FUNCTION__);
    o2ri(opcode, rr, ptrreg, fc);
}

static void gcall_or_jmp(int docall)
{
    /* rd 必须写 tr(docall 时 = ra): jirl rd, rj, 0 语义是 rd=PC+4 再跳 rj。
       原代码 ojirl(0, ...) 用 r0 作 rd, 返回地址丢失 -> 所有函数调用返回即乱飞。
       尾跳(docall=0)时 tr=r12(t0), 写入无害。注意 LoongArch 的 r5=a1 是
       参数寄存器, 不能像 riscv(x5=t0)那样当 scratch。 */
    int tr = docall ? 1 : 12; /* ra or t0 */
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
        ((vtop->r & VT_SYM) && vtop->c.i == (int)vtop->c.i)) {
        /* constant symbolic case -> bl/b sym with R_LARCH_B26: the linker
           routes it through a PLT entry when the target is outside this
           image (shared library, tcc_add_symbol() address) */
        greloca(cur_text_section, vtop->sym, ind, R_LARCH_B26, (int)vtop->c.i);
        o(docall ? O_BL : O_B);
    } else if (vtop->r < VT_CONST) {
        int r = ireg(vtop->r);
        ojirl(tr, r, 0);   /* jirl tr, r, 0 */
    } else {
        int r = TREG_RA;
        load(r, vtop);
        r = ireg(r);
        ojirl(tr, r, 0);
    }
}

#if defined(CONFIG_TCC_BCHECK)

static void gen_bounds_call(int v)
{
    Sym *sym = external_helper_sym(v);
    greloca(cur_text_section, sym, ind, R_LARCH_B26, 0);
    o(O_BL);
}

/* Bound checking, the riscv64-gen.c scheme: the prolog reserves room
   that gen_bounds_epilog patches into a __bound_local_new call when the
   function registered local objects, and the epilog frees them with
   __bound_local_delete around the return-value registers. */
static void gen_bounds_prolog(void)
{
    /* leave some room for bound checking code */
    func_bound_offset = lbounds_section->data_offset;
    func_bound_ind = ind;
    func_bound_add_epilog = 0;
    o(O_NOP);  /* pcalau12i a0, %got_pc_hi20(lbound section pointer) */
    o(O_NOP);  /* ld.d a0, a0, %got_pc_lo12(...) */
    o(O_NOP);  /* bl __bound_local_new */
}

static void gen_bounds_load_table(Sym *sym_data)
{
    greloca(cur_text_section, sym_data, ind, R_LARCH_GOT_PC_HI20, 0);
    oli20(O_PCALAU12I, 4, 0);                  /* pcalau12i a0, 0 */
    greloca(cur_text_section, sym_data, ind, R_LARCH_GOT_PC_LO12, 0);
    o2ri(O_LD_D, 4, 4, 0);                     /* ld.d a0, a0, 0 */
}

static void gen_bounds_epilog(void)
{
    addr_t saved_ind;
    addr_t *bounds_ptr;
    Sym *sym_data;

    int offset_modified = func_bound_offset != lbounds_section->data_offset;

    if (!offset_modified && !func_bound_add_epilog)
        return;

    /* add end of table info */
    bounds_ptr = section_ptr_add(lbounds_section, sizeof(addr_t));
    *bounds_ptr = 0;

    sym_data = get_sym_ref(&char_pointer_type, lbounds_section,
                           func_bound_offset, PTR_SIZE);

    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = ind;
        ind = func_bound_ind;
        gen_bounds_load_table(sym_data);
        gen_bounds_call(TOK___bound_local_new);
        ind = saved_ind;
    }

    /* generate bound check local freeing, keeping the return value
       (a0/a1, fa0/fa1) alive across the call */
    o2ri(O_ADDI_D, 3, 3, -32);
    o2ri(O_ST_D, 4, 3, 0);
    o2ri(O_ST_D, 5, 3, 8);
    o2ri(O_FST_D, 0, 3, 16);
    o2ri(O_FST_D, 1, 3, 24);
    gen_bounds_load_table(sym_data);
    gen_bounds_call(TOK___bound_local_delete);
    o2ri(O_LD_D, 4, 3, 0);
    o2ri(O_LD_D, 5, 3, 8);
    o2ri(O_FLD_D, 0, 3, 16);
    o2ri(O_FLD_D, 1, 3, 24);
    o2ri(O_ADDI_D, 3, 3, 32);
}

#endif

/* ---- prolog/epilog helpers ---- */
static int func_sub_sp_offset, num_va_regs, func_va_list_ofs;

/* ---- argument classification (from riscv64-gen.c) ---- */
static void reg_pass_rec(CType *type, int *rc, int *fieldofs, int ofs)
{
    if ((type->t & VT_BTYPE) == VT_STRUCT) {
        Sym *f;
        if (type->ref->type.t == VT_UNION)
          rc[0] = -1;
        else for (f = type->ref->next; f; f = f->next)
          reg_pass_rec(&f->type, rc, fieldofs, ofs + f->c);
    } else if (type->t & VT_ARRAY) {
        if (type->ref->c < 0 || type->ref->c > 2)
          rc[0] = -1;
        else {
            int a, sz = type_size(&type->ref->type, &a);
            reg_pass_rec(&type->ref->type, rc, fieldofs, ofs);
            if (rc[0] > 2 || (rc[0] == 2 && type->ref->c > 1))
              rc[0] = -1;
            else if (type->ref->c == 2 && rc[0] && rc[1] == RC_FLOAT) {
              rc[++rc[0]] = RC_FLOAT;
              fieldofs[rc[0]] = ((ofs + sz) << 4)
                                | (type->ref->type.t & VT_BTYPE);
            } else if (type->ref->c == 2)
              rc[0] = -1;
        }
    } else if (rc[0] == 2 || rc[0] < 0 || (type->t & VT_BTYPE) == VT_LDOUBLE)
      rc[0] = -1;
    else if (!rc[0] || rc[1] == RC_FLOAT || is_float(type->t)) {
      rc[++rc[0]] = is_float(type->t) ? RC_FLOAT : RC_INT;
      fieldofs[rc[0]] = (ofs << 4) | ((type->t & VT_BTYPE) == VT_PTR ? VT_LLONG : type->t & VT_BTYPE);
    } else
      rc[0] = -1;
}

static void reg_pass(CType *type, int *prc, int *fieldofs, int named)
{
    prc[0] = 0;
    reg_pass_rec(type, prc, fieldofs, 0);
    if (prc[0] <= 0 || !named) {
        int align, size = type_size(type, &align);
        prc[0] = (size + 7) >> 3;
        prc[1] = prc[2] = RC_INT;
        fieldofs[1] = (0 << 4) | (size <= 1 ? VT_BYTE : size <= 2 ? VT_SHORT : size <= 4 ? VT_INT : VT_LLONG);
        fieldofs[2] = (8 << 4) | (size <= 9 ? VT_BYTE : size <= 10 ? VT_SHORT : size <= 12 ? VT_INT : VT_LLONG);
    }
}

/* LoongArch LP64D argument passing (psABI v2, "Parameter passing"):
   integer args in a0..a7 ($r4..$r11), FP args in fa0..fa7 ($f0..$f7).
   An argument that reg_pass classifies onto FARs keeps them only if every
   FAR it needs, and the GAR of a float+int struct, is still free;
   otherwise it is passed by the integer calling convention: GARs, then
   the stack.  This applies to scalar floats too, so the 9th double goes
   in a GAR, not on the stack.  GPR_USED/FPR_USED count registers taken. */
static void reg_pass_avail(CType *type, int *prc, int *fieldofs, int named,
                           int gpr_used, int fpr_used)
{
    int nf, ni;
    reg_pass(type, prc, fieldofs, named);
    nf = (prc[1] == RC_FLOAT) + (prc[0] == 2 && prc[2] == RC_FLOAT);
    ni = prc[0] - nf;
    if (nf && (fpr_used + nf > 8 || gpr_used + ni > 8))
        reg_pass(type, prc, fieldofs, 0);
}

ST_FUNC void gfunc_call(int nb_args)
{
    int i, align, size, areg[2];
    int *info = tcc_malloc((nb_args + 1) * sizeof (int));
    int stack_adj = 0, tempspace = 0, stack_add, ofs, splitofs = 0;
    int old = (vtop[-nb_args].type.ref->f.func_type == FUNC_OLD);
    SValue *sv;
    Sym *sa;

#ifdef CONFIG_TCC_BCHECK
    int bc_save = tcc_state->do_bounds_check;
    if (tcc_state->do_bounds_check)
        gbound_args(nb_args);
#endif

    areg[0] = 0; /* int arg regs */
    areg[1] = 8; /* float arg regs */
    sa = vtop[-nb_args].type.ref->next;
    for (i = 0; i < nb_args; i++) {
        int nregs, byref = 0, tempofs;
        int prc[3], fieldofs[3];
        sv = &vtop[1 + i - nb_args];
        sv->type.t &= ~VT_ARRAY;
        size = type_size(&sv->type, &align);
        if (size > 16) {
            if (align < XLEN)
              align = XLEN;
            tempspace = (tempspace + align - 1) & -align;
            tempofs = tempspace;
            tempspace += size;
            size = align = 8;
            byref = 64 | (tempofs << 7);
        }
        reg_pass_avail(&sv->type, prc, fieldofs, old || sa != 0,
                       areg[0], areg[1] - 8);
        if (!old && !sa && align == 2*XLEN && size <= 2*XLEN)
          areg[0] = (areg[0] + 1) & ~1;
        nregs = prc[0];
        if (size == 0)
            info[i] = 0;
        else if (prc[1] == RC_INT && areg[0] >= 8) {
            info[i] = 32;
            if (align < XLEN)
              align = XLEN;
            stack_adj += (size + align - 1) & -align;
            if (!old && !sa)
              areg[0] = 8, areg[1] = 16;
        } else {
            info[i] = areg[prc[1] - 1]++;
            if (!byref)
              info[i] |= (fieldofs[1] & VT_BTYPE) << 12;
            assert(!(fieldofs[1] >> 4));
            if (nregs == 2) {
                if (prc[2] == RC_FLOAT || areg[0] < 8)
                  info[i] |= (1 + areg[prc[2] - 1]++) << 7;
                else {
                    info[i] |= 16;
                    stack_adj += 8;
                }
                if (!byref) {
                    assert((fieldofs[2] >> 4) < 2048);
                    info[i] |= fieldofs[2] << (12 + 4);
                }
            }
        }
        info[i] |= byref;
        if (sa)
          sa = sa->next;
    }
    stack_adj = (stack_adj + 15) & -16;
    tempspace = (tempspace + 15) & -16;
    stack_add = stack_adj + tempspace;

    if (stack_add) {
        if (!imm12_ok(-stack_add)) {
            int r5 = 12; /* t0 */
            load_large_constant(r5, -stack_add, (int)((uint64_t)(-stack_add) >> 32));
            o3r(O_ADD_D, 3, 3, r5);
        }
        else
            o2ri(O_ADDI_D, 3, 3, -stack_add);   /* addi.d sp, sp, -adj */
        for (i = ofs = 0; i < nb_args; i++) {
            if (info[i] & (64 | 32)) {
                vrotb(nb_args - i);
                size = type_size(&vtop->type, &align);
                if (info[i] & 64) {
                    vset(&char_pointer_type, TREG_SP, 0);
                    vpushi(stack_adj + (info[i] >> 7));
                    gen_op('+');
                    vpushv(vtop);
                    vrott(3);
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vpop();
                    size = align = 8;
                }
                if (info[i] & 32) {
                    if (align < XLEN)
                      align = XLEN;
                    vset(&char_pointer_type, TREG_SP, 0);
                    ofs = (ofs + align - 1) & -align;
                    vpushi(ofs);
                    gen_op('+');
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vtop->r = vtop->r2 = VT_CONST;
                    ofs += size;
                }
                vrott(nb_args - i);
            } else if (info[i] & 16) {
                assert(!splitofs);
                splitofs = ofs;
                ofs += 8;
            }
        }
    }
    for (i = 0; i < nb_args; i++) {
        int ii = info[nb_args - 1 - i], r = ii, r2 = r;
        if (!(r & 32)) {
            CType origtype;
            int loadt;
            r &= 15;
            r2 = r2 & 64 ? 0 : (r2 >> 7) & 31;
            assert(r2 <= 16);
            vrotb(i+1);
            origtype = vtop->type;
            size = type_size(&vtop->type, &align);
            if (size == 0)
                goto done;
            loadt = vtop->type.t & VT_BTYPE;
            if (loadt == VT_STRUCT) {
                loadt = (ii >> 12) & VT_BTYPE;
            }
            if (info[nb_args - 1 - i] & 16) {
                assert(!r2);
                r2 = 1 + TREG_RA;
            }
            if (loadt == VT_LDOUBLE) {
                assert(r2);
                r2--;
            } else if (r2) {
                test_lvalue();
                vpushv(vtop);
            }
            vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
            /* RC_F(k) is $f(k) = fa(k): already the ABI argument register */
            gv(r < 8 ? RC_R(r) : RC_F(r - 8));
            vtop->type = origtype;

            if (r2 && loadt != VT_LDOUBLE) {
                r2--;
                assert(r2 < 16 || r2 == TREG_RA);
                vswap();
                gaddrof();
                vtop->type = char_pointer_type;
                vpushi(ii >> 20);
#ifdef CONFIG_TCC_BCHECK
                if ((origtype.t & VT_BTYPE) == VT_STRUCT)
                    tcc_state->do_bounds_check = 0;
#endif
                gen_op('+');
#ifdef CONFIG_TCC_BCHECK
                tcc_state->do_bounds_check = bc_save;
#endif
                indir();
                vtop->type = origtype;
                loadt = vtop->type.t & VT_BTYPE;
                if (loadt == VT_STRUCT) {
                    loadt = (ii >> 16) & VT_BTYPE;
                }
                save_reg_upstack(r2, 1);
                vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
                load(r2, vtop);
                assert(r2 < VT_CONST);
                vtop--;
                vtop->r2 = r2;
            }
            if (info[nb_args - 1 - i] & 16) {
                o2ri(O_ST_D, ireg(vtop->r2), 3, splitofs);  /* st.d t0, ofs(sp) */
                vtop->r2 = VT_CONST;
            } else if (loadt == VT_LDOUBLE && vtop->r2 != r2) {
                assert(vtop->r2 <= 7 && r2 <= 7);
                omove(ireg(r2), ireg(vtop->r2));
                vtop->r2 = r2;
            }
done:
            vrott(i+1);
        }
    }
    vrotb(nb_args + 1);
    save_regs(nb_args + 1);
    gcall_or_jmp(1);
    vtop -= nb_args + 1;
    if (stack_add) {
        if (!imm12_ok(stack_add)) {
            int r5 = 12; /* t0 */
            load_large_constant(r5, stack_add, (int)((uint64_t)stack_add >> 32));
            o3r(O_ADD_D, 3, 3, r5);
        }
        else
            o2ri(O_ADDI_D, 3, 3, stack_add);
    }
    tcc_free(info);
}

ST_FUNC void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int i, addr, align, size;
    int param_addr = 0;
    int areg[2];
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    loc = -16; // for ra and fp
    func_sub_sp_offset = ind;
    ind += 5 * 4;

    areg[0] = 0, areg[1] = 0;
    addr = 0;
    /* if the function returns by reference, then add an
       implicit pointer parameter */
    size = type_size(&func_vt, &align);
    if (size > 2 * XLEN) {
        loc -= 8;
        func_vc = loc;
        o2ri(O_ST_D, 4 + areg[0]++, 22, loc); // st.d a0, loc(fp)
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        int byref = 0;
        int regcount;
        int prc[3], fieldofs[3];
        type = &sym->type;
        size = type_size(type, &align);
        if (size > 2 * XLEN) {
            type = &char_pointer_type;
            size = align = byref = 8;
        }
        reg_pass_avail(type, prc, fieldofs, 1, areg[0], areg[1]);
        regcount = prc[0];
        if (prc[1] == RC_INT && areg[0] >= 8) {
            if (align < XLEN)
              align = XLEN;
            addr = (addr + align - 1) & -align;
            param_addr = addr;
            addr += size;
        } else {
            loc -= regcount * 8;
            param_addr = loc;
            for (i = 0; i < regcount; i++) {
                /* each part lands at its member's offset: for a struct
                   classified FP+int (e.g. {float f; int i;}) the integer
                   member is at offset 4, not 8 (riscv64-gen.c has the same
                   'XXX'); all-integer aggregates give offsets 0 and 8 */
                int fofs = loc + (fieldofs[i+1] >> 4);
                int bt = fieldofs[i+1] & VT_BTYPE;
                uint32_t st = bt == VT_BYTE || bt == VT_BOOL ? O_ST_B
                            : bt == VT_SHORT ? O_ST_H : bt == VT_INT ? O_ST_W : O_ST_D;
                if (areg[prc[1+i] - 1] >= 8) {
                    assert(i == 1 && regcount == 2 && !(addr & 7));
                    o2ri(O_LD_D, 12, 22, addr);  // ld.d t0, addr(fp)
                    addr += 8;
                    o2ri(st, 12, 22, fofs);      // st.[bhwd] t0, member(fp)
                } else if (prc[1+i] == RC_FLOAT) {
                    /* fa(k) is $f(k), not $f(12+k) */
                    o2ri((size / regcount) == 4 ? O_FST_S : O_FST_D,
                         areg[1]++, 22, fofs);
                } else {
                    o2ri(st, 4 + areg[0]++, 22, fofs); // st.[bhwd] aX, member(fp)
                }
            }
        }
        gfunc_set_param(sym, param_addr, byref);
    }
    func_va_list_ofs = addr;
    num_va_regs = 0;
    if (func_var) {
        for (; areg[0] < 8; areg[0]++) {
            num_va_regs++;
            o2ri(O_ST_D, 4 + areg[0], 22, -8 + num_va_regs * 8); // st.d aX, loc(fp)
        }
    }
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_prolog();
#endif
}

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *ret_align, int *regsize)
{
    int align, size = type_size(vt, &align), nregs;
    int prc[3], fieldofs[3];
    *ret_align = 1;
    *regsize = 8;
    if (size > 16)
      return 0;
    reg_pass(vt, prc, fieldofs, 1);
    nregs = prc[0];
    if (nregs == 2 && prc[1] != prc[2])
      return -1;
    if (prc[1] == RC_FLOAT) {
        *regsize = size / nregs;
    }
    ret->t = fieldofs[1] & VT_BTYPE;
    ret->ref = NULL;
    return nregs;
}

ST_FUNC void arch_transfer_ret_regs(int aftercall)
{
    int prc[3], fieldofs[3];
    reg_pass(&vtop->type, prc, fieldofs, 1);
    assert(prc[0] == 2 && prc[1] != prc[2] && !(fieldofs[1] >> 4));
    assert(vtop->r == (VT_LOCAL | VT_LVAL));
    vpushv(vtop);
    vtop->type.t = fieldofs[1] & VT_BTYPE;
    (aftercall ? store : load)(prc[1] == RC_INT ? REG_IRET : REG_FRET, vtop);
    vtop->c.i += fieldofs[2] >> 4;
    vtop->type.t = fieldofs[2] & VT_BTYPE;
    (aftercall ? store : load)(prc[2] == RC_INT ? REG_IRET : REG_FRET, vtop);
    vtop--;
}

ST_DATA int func_frame_size, func_va_size;

ST_FUNC void gfunc_epilog(void)
{
    int v, saved_ind, d, large_ofs_ind;

#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_epilog();
#endif

    loc = (loc - num_va_regs * 8);
    d = v = (-loc + 15) & -16;

    /* epilog. The saved ra/fp must stay at or above sp until reloaded:
       LoongArch has no red zone, and a signal delivered between raising
       sp and the loads would overwrite them with its frame.
       tccdbg.c's LoongArch FDE describes exactly this sequence. */
    o2ri(O_ADDI_D, 3, 22, -16);                  // addi.d sp, fp, -16
    o2ri(O_LD_D, 1, 3, 8);                       // ld.d ra, sp, 8
    o2ri(O_LD_D, 22, 3, 0);                      // ld.d fp, sp, 0
    o2ri(O_ADDI_D, 3, 3, 16 + num_va_regs * 8);  // addi.d sp, sp, 16+va
    ojirl(0, 1, 0);                              // jr ra

    large_ofs_ind = ind;
    if (v >= (1 << 11)) {
        d = 16;
        o2ri(O_ADDI_D, 22, 3, d - num_va_regs * 8);  // addi.d fp, sp, d
        if (!imm12_ok(v - 16)) {
            int r5 = 12;  /* t0 */
            load_large_constant(r5, v - 16, (int)((uint64_t)(v - 16) >> 32));
            o3r(O_SUB_D, 3, 3, r5);   // sub sp, sp, t0
        } else {
            o2ri(O_ADDI_D, 3, 3, -(v - 16));
        }
        gjmp_addr(func_sub_sp_offset + 5*4);
    }
    saved_ind = ind;
    func_frame_size = d;
    func_va_size = num_va_regs * 8;

    ind = func_sub_sp_offset;
    o2ri(O_ADDI_D, 3, 3, -d);            // addi.d sp, sp, -d
    o2ri(O_ST_D, 1, 3, d - 8 - num_va_regs * 8);   // st.d ra, d-8(sp)
    o2ri(O_ST_D, 22, 3, d - 16 - num_va_regs * 8); // st.d fp, d-16(sp)
    if (v < (1 << 11))
      o2ri(O_ADDI_D, 22, 3, d - num_va_regs * 8);  // addi.d fp, sp, d
    else
      gjmp_addr(large_ofs_ind);
    if ((ind - func_sub_sp_offset) != 5*4)
      o(O_NOP);
    ind = saved_ind;
}

ST_FUNC void gen_va_start(void)
{
    vtop--;
    vset(&char_pointer_type, VT_LOCAL, func_va_list_ofs);
}

ST_FUNC void gen_fill_nops(int bytes)
{
    if ((bytes & 3))
      tcc_error("alignment of code section not multiple of 4");
    while (bytes > 0) {
        o(O_NOP);
        bytes -= 4;
    }
}

/* Generate forward branch to label (branch chain: word stores next) */
ST_FUNC int gjmp(int t)
{
    if (nocode_wanted)
      return t;
    o(t);
    return ind - 4;
}

/* Generate branch to known address */
ST_FUNC void gjmp_addr(int a)
{
    uint32_t r = a - ind;   /* byte offset */
    int offs;
    if ((r + (1 << 27)) & ~(uint32_t)((1U << 28) - 4)) {
        /* out of range (±128MB): 基本不会触发; 真到了也只能长跳 */
        o2ri(O_ADDI_D, 12, 3, 0);  /* placeholder (r12 = t0) */
        ojirl(0, 12, 0);
    } else {
        offs = (int)(r >> 2);
        /* 同 gsym_addr: si26 高 10 位(bits[9:0])必须写符号位 */
        o(O_B | ((offs & 0xffff) << 10) | ((offs >> 16) & 0x3ff));
    }
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int tmp;
    int a = vtop->cmp_r & 0xff;
    int b = (vtop->cmp_r >> 8) & 0xff;
    switch (op) {
        case TOK_ULT: op = O_BLTU; break;
        case TOK_UGE: op = O_BGEU; break;
        case TOK_ULE: op = O_BGEU; tmp = a; a = b; b = tmp; break;
        case TOK_UGT: op = O_BLTU; tmp = a; a = b; b = tmp; break;
        case TOK_LT:  op = O_BLT; break;
        case TOK_GE:  op = O_BGE; break;
        case TOK_LE:  op = O_BGE; tmp = a; a = b; b = tmp; break;
        case TOK_GT:  op = O_BLT; tmp = a; a = b; b = tmp; break;
        case TOK_NE:  op = O_BNE; break;
        case TOK_EQ:  op = O_BEQ; break;
    }
    /* 条件分支必须发"反条件"来跳过链式跳转(对照 riscv64 的 op ^ 1):
       上面的映射得到的是"真条件", LoongArch 分支操作码 bit26 即真假对
       (BEQ=0x58/BNE=0x5c, BLT=0x60/BGE=0x64, BLTU=0x68/BGEU=0x6c),
       ^0x04000000 取反。原代码漏了取反 -> 所有 if/while/for 条件全部颠倒 */
    o((op ^ 0x04000000u) | (2 << 10) | (a << 5) | b);  /* bINV a,b,+8 (skip the j) */
    return gjmp(t);
}

ST_FUNC int gjmp_append(int n, int t)
{
    void *p;
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(p = cur_text_section->data + n1)))
            n1 = n2;
        write32le(p, t);
        t = n;
    }
    return t;
}

static void gen_opil(int op, int ll)
{
    int a, b, d;
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
        /* CValue.i 是 uint64_t: 必须按 64 位读取, 否则高位非零常量
           (如 SLAB_MAGIC = 1ULL<<63 = 0x8000000000000000) 被 int 截断
           成 0, 误入 ori/andi 立即数路径, 生成 "ori $a1,$a1,0" 丢掉
           整个高位 (t05 slab b->size 只写成 sc 的教训).
           riscv64 版靠 "fc == vtop->c.i" 的 64 位比较兜底, 这里直接
           用 int64_t 读. */
        int64_t fc = (int64_t)vtop->c.i;
        int is_log = (op == '&' || op == '|' || op == '^');
        /* ANDI/ORI/XORI 的 imm12 是【零扩展】(逻辑运算), 负立即数会得到
           0x00000xxx 而非 0xFFFF...xxx —— 如 va_arg 的 "ap & ~3" (& -4)
           若用 andi 0xffc 会把 64 位地址截成 12 位小整数!
           逻辑运算只接受 0..4095; 算术/比较(ADDI/SLTI 符号扩展)接受 ±2048 */
        if ((is_log && (uint64_t)fc < 0x1000) || (!is_log && imm12_ok(fc))) {
            vswap();
            gv(RC_INT);
            a = ireg(vtop[0].r);
            --vtop;
            d = get_reg(RC_INT);
            ++vtop;
            vswap();
            switch (op) {
                case '-':
                    if (fc == -2048)
                      goto too_big;    /* addi.d 不能表示 -(-2048), 走非立即数路径 */
                    fc = -fc;
                case '+':
                    o2ri(ll ? O_ADDI_D : O_ADDI_W, ireg(d), a, (int)fc);
                    --vtop;
                    vtop[0].r = d;
                    return;
                case TOK_LE:
                    if (fc >= 2047)     /* ++fc 后需 < 2^11 */
                      goto too_big;
                    ++fc;
                case TOK_LT:
                    o2ri(O_SLTI, ireg(d), a, (int)fc);
                    goto cmp_done;
                case TOK_ULE:
                    if (fc >= 2047 || fc == -1)
                      goto too_big;
                    ++fc;
                case TOK_ULT:
                    o2ri(O_SLTUI, ireg(d), a, (int)fc);
                    goto cmp_done;
                case '^': o2ri(O_XORI, ireg(d), a, (int)fc); goto done2;
                case '|': o2ri(O_ORI,  ireg(d), a, (int)fc); goto done2;
                case '&': o2ri(O_ANDI, ireg(d), a, (int)fc); goto done2;
                case TOK_SHL: oshift(ll ? O_SLLI_D : O_SLLI_W, ireg(d), a, fc & (ll ? 63 : 31)); goto done2;
                case TOK_SHR: oshift(ll ? O_SRLI_D : O_SRLI_W, ireg(d), a, fc & (ll ? 63 : 31)); goto done2;
                case TOK_SAR: oshift(ll ? O_SRAI_D : O_SRAI_W, ireg(d), a, fc & (ll ? 63 : 31)); goto done2;

                case TOK_UGE: /* -> TOK_ULT */
                case TOK_UGT: /* -> TOK_ULE */
                case TOK_GE:  /* -> TOK_LT */
                case TOK_GT:  /* -> TOK_LE */
                    gen_opil(op - 1, ll);
                    vtop->cmp_op ^= 1;
                    return;

                case TOK_NE:
                case TOK_EQ:
                    if (fc)
                        gen_opil('-', ll), a = ireg(vtop++->r);
                    --vtop;
                    vset_VT_CMP(op);
                    vtop->cmp_r = a | 0 << 8;
                    return;

                default:
                    goto too_big;
            }
        cmp_done:
            /* slti/sltui 产生 0/1, 结果即 (d != 0) */
            --vtop;
            vset_VT_CMP(TOK_NE);
            vtop->cmp_r = ireg(d) | 0 << 8;
            return;
        done2:
            --vtop;
            vtop[0].r = d;
            return;
        too_big:
            ;  /* 落入非立即数路径 */
        }
    }
    gv2(RC_INT, RC_INT);
    a = ireg(vtop[-1].r);
    b = ireg(vtop[0].r);
    vtop -= 2;
    d = get_reg(RC_INT);
    vtop++;
    vtop[0].r = d;
    d = ireg(d);
    switch (op) {
    default:
        if (op >= TOK_ULT && op <= TOK_GT) {
            vset_VT_CMP(op);
            vtop->cmp_r = a | b << 8;
            break;
        }
        tcc_error("implement me: %s(%s)", __FUNCTION__, get_tok_str(op, NULL));
        break;

    /* 32-bit operations use the .w forms: they read the low 32 bits and
       sign-extend the result, the register convention for int and
       unsigned int alike (riscv64-gen.c does the same with its *w ops) */
    case '+': o3r(ll ? O_ADD_D : O_ADD_W, d, a, b); break;
    case '-': o3r(ll ? O_SUB_D : O_SUB_W, d, a, b); break;
    case TOK_SAR: o3r(ll ? O_SRA_D : O_SRA_W, d, a, b); break;
    case TOK_SHR: o3r(ll ? O_SRL_D : O_SRL_W, d, a, b); break;
    case TOK_SHL: o3r(ll ? O_SLL_D : O_SLL_W, d, a, b); break;
    case '*': o3r(ll ? O_MUL_D : O_MUL_W, d, a, b); break;
    case '/':
    case TOK_PDIV: o3r(ll ? O_DIV_D : O_DIV_W, d, a, b); break;
    case '&': o3r(O_AND, d, a, b); break;
    case '^': o3r(O_XOR, d, a, b); break;
    case '|': o3r(O_OR, d, a, b); break;
    case '%': o3r(ll ? O_MOD_D : O_MOD_W, d, a, b); break;
    case TOK_UMOD: o3r(ll ? O_MOD_DU : O_MOD_WU, d, a, b); break;
    case TOK_UDIV: o3r(ll ? O_DIV_DU : O_DIV_WU, d, a, b); break;
    }
}

ST_FUNC void gen_opi(int op)
{
    gen_opil(op, 0);
}

ST_FUNC void gen_opl(int op)
{
    gen_opil(op, 1);
}

ST_FUNC void gen_opf(int op)
{
    int rs1, rs2, rd, dbl, invert;
    if (vtop[0].type.t == VT_LDOUBLE) {
        CType type = vtop[0].type;
        int func = 0;
        int cond = -1;
        switch (op) {
        case '*': func = TOK___multf3; break;
        case '+': func = TOK___addtf3; break;
        case '-': func = TOK___subtf3; break;
        case '/': func = TOK___divtf3; break;
        case TOK_EQ: func = TOK___eqtf2; cond = 1; break;
        case TOK_NE: func = TOK___netf2; cond = 0; break;
        case TOK_LT: func = TOK___lttf2; cond = 10; break;
        case TOK_GE: func = TOK___getf2; cond = 11; break;
        case TOK_LE: func = TOK___letf2; cond = 12; break;
        case TOK_GT: func = TOK___gttf2; cond = 13; break;
        default: assert(0); break;
        }
        vpush_helper_func(func);
        vrott(3);
        gfunc_call(2);
        vpushi(0);
        vtop->r = REG_IRET;
        vtop->r2 = cond < 0 ? TREG_R(1) : VT_CONST;
        if (cond < 0)
            vtop->type = type;
        else {
            vpushi(0);
            gen_opil(op, 1);
        }
        return;
    }

    gv2(RC_FLOAT, RC_FLOAT);
    assert(vtop->type.t == VT_DOUBLE || vtop->type.t == VT_FLOAT);
    dbl = vtop->type.t == VT_DOUBLE;
    rs1 = freg(vtop[-1].r);
    rs2 = freg(vtop->r);
    vtop--;
    invert = 0;
    switch(op) {
    default:
        assert(0);
    case '+':
        op = dbl ? O_FADD_D : O_FADD_S;
    arithop:
        rd = get_reg(RC_FLOAT);
        vtop->r = rd;
        rd = freg(rd);
        o3r(op, rd, rs1, rs2);
        break;
    case '-':
        op = dbl ? O_FSUB_D : O_FSUB_S; goto arithop;
    case '*':
        op = dbl ? O_FMUL_D : O_FMUL_S; goto arithop;
    case '/':
        op = dbl ? O_FDIV_D : O_FDIV_S; goto arithop;
    case TOK_EQ:
    case TOK_NE:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE: {
        /* fcmp.cond.{s,d} fcc0, fj, fk ; movcf2gr rd, fcc0 [; xori rd, 1].
           a > b and a >= b are b < a and b <= a, so that they are false
           when either operand is a NaN; only != is a negated compare.
           cond: CEQ = 4 (quiet, as for C ==), SLT = 3, SLE = 7. */
        int cond, t;
        rd = get_reg(RC_INT);
        vtop->r = rd;
        rd = ireg(rd);
        switch (op) {
        case TOK_EQ: cond = 4; break;
        case TOK_NE: cond = 4; invert = 1; break;
        case TOK_LT: cond = 3; break;
        case TOK_LE: cond = 7; break;
        case TOK_GT: cond = 3; t = rs1; rs1 = rs2; rs2 = t; break;
        default:     cond = 7; t = rs1; rs1 = rs2; rs2 = t; break; /* GE */
        }
        o((dbl ? O_FCMP_D : O_FCMP_S) | (cond << 15) | (rs2 << 10) | (rs1 << 5));
        o(O_MOVCF2GR | (0 << 5) | rd);
        if (invert)
            o2ri(O_XORI, rd, rd, 1);
        vset_VT_CMP(TOK_NE);
        vtop->cmp_r = rd | (0 << 8);
        break;
    }
    }
}

ST_FUNC void gen_cvt_csti(int t)
{
    int r = ireg(gv(RC_INT));
    if ((t & VT_BTYPE) == VT_SHORT) {
        if (t & VT_UNSIGNED) {
            oshift(O_SLLI_D, r, r, 48);
            oshift(O_SRLI_D, r, r, 48);
        } else {
            oshift(O_SLLI_D, r, r, 48);
            oshift(O_SRAI_D, r, r, 48);
        }
    } else {
        if (t & VT_UNSIGNED) {
            o2ri(O_ANDI, r, r, 0xff);
        } else {
            oshift(O_SLLI_D, r, r, 56);
            oshift(O_SRAI_D, r, r, 56);
        }
    }
}

ST_FUNC void gen_cvt_sxtw(void)
{
    int r = ireg(gv(RC_INT));
    o2ri(O_ADDI_W, r, r, 0);  /* addi.w r, r, 0 sign-extends 32->64 */
}

ST_FUNC void gen_cvt_itof(int t)
{
    int rr = ireg(gv(RC_INT)), dr;
    int u = vtop->type.t & VT_UNSIGNED;
    int l = (vtop->type.t & VT_BTYPE) == VT_LLONG;
    if (t == VT_LDOUBLE) {
        int func = l ?
          (u ? TOK___floatunditf : TOK___floatditf) :
          (u ? TOK___floatunsitf : TOK___floatsitf);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_IRET;
        vtop->r2 = TREG_R(1);
    } else {
        vtop--;
        dr = get_reg(RC_FLOAT);
        vtop++;
        vtop->r = dr;
        dr = freg(dr);
        uint32_t ffint = t == VT_FLOAT ? O_FFINT_S_L : O_FFINT_D_L;
        if (u && !l) {
            /* unsigned int: 先零扩展高 32 位 —— 必须在 movgr2fr 之前
               (原来放在 mov 之后, 等于没做, 负解释被 ffint 转换) */
            oshift(O_SLLI_D, rr, rr, 32);
            oshift(O_SRLI_D, rr, rr, 32);
        }
        /* ffint.{s,d}.l rounds once, straight from the 64-bit integer
           (going through double first rounds twice for float) */
        o(O_MOVGR2FR_D | (rr << 5) | dr);
        o(ffint | (dr << 5) | dr);
        if (u && l) {
            /* unsigned long long >= 2^63 read as negative: convert
               (x >> 1) | (x & 1) instead and double it (round-to-odd
               keeps the single rounding exact), as gcc does */
            obranch(O_BGE, rr, 0, 7);                    /* bgez rr, done (+7 insns) */
            oshift(O_SRLI_D, 12, rr, 1);                 /* t0 = rr >> 1 */
            o2ri(O_ANDI, 13, rr, 1);                     /* t1 = rr & 1 */
            o3r(O_OR, 12, 12, 13);
            o(O_MOVGR2FR_D | (12 << 5) | dr);
            o(ffint | (dr << 5) | dr);
            o3r(t == VT_FLOAT ? O_FADD_S : O_FADD_D, dr, dr, dr);
        }
    }
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    int ft = vtop->type.t & VT_BTYPE;
    int l = (t & VT_BTYPE) == VT_LLONG;
    int u = t & VT_UNSIGNED;
    if (ft == VT_LDOUBLE) {
        int func = l ?
          (u ? TOK___fixunstfdi : TOK___fixtfdi) :
          (u ? TOK___fixunstfsi : TOK___fixtfsi);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_IRET;
    } else {
        int rr = freg(gv(RC_FLOAT)), dr;
        vtop--;
        dr = get_reg(RC_INT);
        vtop++;
        vtop->r = dr;
        dr = ireg(dr);
        if (ft == VT_FLOAT)
            o(O_FCVT_D_S | (rr << 5) | rr);  /* fcvt.d.s fd, fj (exact) */
        if (u && l) {
            /* unsigned long long: values >= 2^63 do not fit ftintrz.l.d;
               convert x - 2^63 and set bit 63. $f8/$f9 (ft0/ft1) are
               never allocated by tcc. */
            o2ri(O_LU52I_D, 12, 0, 0x43e);               /* t0 = 2^63 as double */
            o(O_MOVGR2FR_D | (12 << 5) | 8);             /* $f8 = 2^63 */
            o(O_FCMP_CLT_D | (8 << 10) | (rr << 5));     /* fcc0 = x < 2^63 */
            o(O_BCNEZ | (7 << 10));                      /* bcnez fcc0, small */
            o3r(O_FSUB_D, 9, rr, 8);
            o(O_FTINTRZ_L_D | (9 << 5) | 9);
            o(O_MOVFR2GR_D | (9 << 5) | dr);
            o2ri(O_LU52I_D, 12, 0, -2048);               /* t0 = 1 << 63 */
            o3r(O_XOR, dr, dr, 12);
            gjmp_addr(ind + 12);                         /* b done */
        }
        o(O_FTINTRZ_L_D | (rr << 5) | 9);    /* small: ftintrz.l.d $f9, fj */
        o(O_MOVFR2GR_D | (9 << 5) | dr);     /* movfr2gr.d rd=dr, fj=$f9 */
        if (!l)  /* a 32-bit result, unsigned int too, lives sign-extended */
            o2ri(O_ADDI_W, dr, dr, 0);
    }
}

ST_FUNC void gen_cvt_ftof(int dt)
{
    int st = vtop->type.t & VT_BTYPE, rs, rd;
    dt &= VT_BTYPE;
    if (st == dt)
      return;
    if (dt == VT_LDOUBLE || st == VT_LDOUBLE) {
        int func = (dt == VT_LDOUBLE) ?
            (st == VT_FLOAT ? TOK___extendsftf2 : TOK___extenddftf2) :
            (dt == VT_FLOAT ? TOK___trunctfsf2 : TOK___trunctfdf2);
        save_regs(1);
        if (dt == VT_LDOUBLE) {
            gv(RC_F(0));  /* $f0 = fa0, the helper's argument register */
        }
        else {
            gv(RC_R(0));
            assert(vtop->r2 < 7);
            if (vtop->r2 != 1 + vtop->r) {
                omove(ireg(vtop->r) + 1, ireg(vtop->r2));
                vtop->r2 = 1 + vtop->r;
            }
        }
        vpush_helper_func(func);
        gcall_or_jmp(1);
        vtop -= 2;
        vpushi(0);
        vtop->type.t = dt;
        if (dt == VT_LDOUBLE)
          vtop->r = REG_IRET, vtop->r2 = REG_IRET+1;
        else
          vtop->r = REG_FRET;
    } else {
        assert (dt == VT_FLOAT || dt == VT_DOUBLE);
        assert (st == VT_FLOAT || st == VT_DOUBLE);
        rs = gv(RC_FLOAT);
        rd = get_reg(RC_FLOAT);
        if (dt == VT_DOUBLE)
            o(O_FCVT_D_S | (freg(rs) << 5) | freg(rd));  /* fcvt.d.s fd, fj */
        else
            o(O_FCVT_S_D | (freg(rs) << 5) | freg(rd));  /* fcvt.s.d fd, fj */
        vtop->r = rd;  /* tcc register number, not the hardware $f index */
    }
}

ST_FUNC void ggoto(void)
{
    gcall_or_jmp(0);
    vtop--;
}

ST_FUNC void gen_vla_sp_save(int addr)
{
    if (!imm12_ok(addr)) {
        load_large_constant(12, addr, (int)((uint64_t)addr >> 32));
        o3r(O_ADD_D, 12, 12, 22);
        o2ri(O_ST_D, 3, 12, 0);
    } else
        o2ri(O_ST_D, 3, 22, addr);  /* st.d sp, addr(fp) */
}

ST_FUNC void gen_vla_sp_restore(int addr)
{
    if (!imm12_ok(addr)) {
        load_large_constant(12, addr, (int)((uint64_t)addr >> 32));
        o3r(O_ADD_D, 12, 12, 22);
        o2ri(O_LD_D, 3, 12, 0);  /* ld.d sp, 0(t0) */
    } else
        o2ri(O_LD_D, 3, 22, addr);  /* ld.d sp, addr(fp) */
}

ST_FUNC void gen_vla_alloc(CType *type, int align)
{
    int rr;
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        vpushv(vtop);
#endif
    rr = ireg(gv(RC_INT));
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        o2ri(O_ADDI_D, rr, rr, 16);
    else
#endif
    o2ri(O_ADDI_D, rr, rr, 15);
    /* 对齐到 16: 不能用 andi -16(零扩展会得到 0xFF0), 用移位清低 4 位 */
    oshift(O_SRLI_D, rr, rr, 4);
    oshift(O_SLLI_D, rr, rr, 4);
    o3r(O_SUB_D, 3, 3, rr);
    vpop();
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check) {
        /* __bound_new_region(sp, size): the size is the copy pushed above */
        vpushi(0);
        vtop->r = TREG_R(0);
        omove(4, 3);  /* move a0, sp */
        vswap();
        vpush_helper_func(TOK___bound_new_region);
        vrott(3);
        gfunc_call(2);
        func_bound_add_epilog = 1;
    }
#endif
}

ST_FUNC void gen_clear_cache(void)
{
    /* ibar 0: what gcc emits for __builtin___clear_cache on LoongArch
       (0x002ae000 written here before is not ibar) */
    o(0x38728000u);
}

/* increment tcov counter (LoongArch: pcalau12i + addi.d + ld.d + addi.d + st.d) */
ST_FUNC void gen_increment_tcov (SValue *sv)
{
    int r1, r2;

    vpushv(sv);
    vtop->r = r1 = get_reg(RC_INT);
    r2 = get_reg(RC_INT);
    r1 = ireg(r1);
    r2 = ireg(r2);
    /* 读: pcalau12i + addi.d(r1 = &sym), PCALA_LO12 引用同一符号 */
    greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_HI20, 0);
    oli20(O_PCALAU12I, r1, 0);
    greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_LO12, 0);
    o2ri(O_ADDI_D, r1, r1, 0);
    o2ri(O_LD_D, r2, r1, 0);            /* ld.d r2, r1, 0 */
    o2ri(O_ADDI_D, r2, r2, 1);          /* addi.d r2, r2, 1 */
    /* 写: 重新取地址再 st.d */
    greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_HI20, 0);
    oli20(O_PCALAU12I, r1, 0);
    greloca(cur_text_section, sv->sym, ind, R_LARCH_PCALA_LO12, 0);
    o2ri(O_ADDI_D, r1, r1, 0);
    o2ri(O_ST_D, r2, r1, 0);            /* st.d r2, r1, 0 */
    vpop();
}
#endif /* !TARGET_DEFS_ONLY */
