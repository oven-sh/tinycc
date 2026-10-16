/* LoongArch64 ELF linker / relocations for TCC */
#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_LOONGARCH

#define R_DATA_32  R_LARCH_32
#define R_DATA_PTR R_LARCH_64
#define R_JMP_SLOT R_LARCH_JUMP_SLOT
#define R_GLOB_DAT R_LARCH_64
#define R_COPY     R_LARCH_COPY
#define R_RELATIVE R_LARCH_RELATIVE

#define R_NUM      R_LARCH_NUM

#define ELF_START_ADDR 0x00010000
#define ELF_PAGE_SIZE 0x10000

#define PCRELATIVE_DLLPLT 1
#define RELOCATE_DLLPLT 1

#else /* !TARGET_DEFS_ONLY */

//#define DEBUG_RELOC
#include "tcc.h"


/* Returns 1 for a code relocation, 0 for a data relocation. For unknown
   relocations, returns -1. */
ST_FUNC int code_reloc (int reloc_type)
{
    switch (reloc_type) {

    case R_LARCH_B16:
    case R_LARCH_B21:
    case R_LARCH_B26:
    case R_LARCH_CALL36:
    case R_LARCH_RELAX:    /* relax 标记, 在代码段, relocate() 忽略 */
    case R_LARCH_MARK_LA:  /* GNU as marks la.abs sequences; no-op */
    case R_LARCH_MARK_PCREL:
    case R_LARCH_DELETE:
    case R_LARCH_CFA:
    case R_LARCH_ALIGN:
    case R_LARCH_PCREL20_S2:  /* pcaddi(松弛候选), 按代码重定位处理 */
        return 1;

    case R_LARCH_PCALA_HI20:
    case R_LARCH_PCALA_LO12:
    case R_LARCH_ABS_HI20:
    case R_LARCH_ABS_LO12:
    case R_LARCH_ABS64_LO20:
    case R_LARCH_ABS64_HI12:
    case R_LARCH_PCALA64_LO20:
    case R_LARCH_PCALA64_HI12:
    case R_LARCH_GOT_HI20:
    case R_LARCH_GOT_LO12:
    case R_LARCH_GOT_PC_HI20:
    case R_LARCH_GOT_PC_LO12:
    case R_LARCH_GOT64_PC_LO20:
    case R_LARCH_GOT64_PC_HI12:
    case R_LARCH_TLS_LE_HI20:
    case R_LARCH_TLS_LE_LO12:
    case R_LARCH_TLS_LE64_LO20:
    case R_LARCH_TLS_LE64_HI12:
    case R_LARCH_TLS_LE_HI20_R:
    case R_LARCH_TLS_LE_LO12_R:
    case R_LARCH_TLS_LE_ADD_R:
    case R_LARCH_ADD6:
    case R_LARCH_SUB6:
    case R_LARCH_ADD_ULEB128:
    case R_LARCH_SUB_ULEB128:
    case R_LARCH_64_PCREL:
    case R_LARCH_ADD8:
    case R_LARCH_ADD16:
    case R_LARCH_ADD24:
    case R_LARCH_ADD32:
    case R_LARCH_ADD64:
    case R_LARCH_SUB8:
    case R_LARCH_SUB16:
    case R_LARCH_SUB24:
    case R_LARCH_SUB32:
    case R_LARCH_SUB64:
    case R_LARCH_32:
    case R_LARCH_64:
    case R_LARCH_32_PCREL:
        return 0;
    }
    return -1;
}

/* Returns an enumerator to describe whether and when the relocation needs a
   GOT and/or PLT entry to be created. */
ST_FUNC int gotplt_entry_type (int reloc_type)
{
    switch (reloc_type) {
    case R_LARCH_NONE:
    case R_LARCH_RELATIVE:
    case R_LARCH_COPY:
    case R_LARCH_JUMP_SLOT:
    case R_LARCH_IRELATIVE:
    case R_LARCH_ADD8:
    case R_LARCH_ADD16:
    case R_LARCH_ADD24:
    case R_LARCH_ADD32:
    case R_LARCH_ADD64:
    case R_LARCH_SUB8:
    case R_LARCH_SUB16:
    case R_LARCH_SUB24:
    case R_LARCH_SUB32:
    case R_LARCH_SUB64:
    case R_LARCH_DELETE:   /* relax 标记/算术: 不需要 GOT/PLT */
    case R_LARCH_MARK_LA:
    case R_LARCH_MARK_PCREL:
    case R_LARCH_CFA:
    case R_LARCH_ALIGN:
    case R_LARCH_RELAX:
    case R_LARCH_ADD6:
    case R_LARCH_SUB6:
    case R_LARCH_ADD_ULEB128:
    case R_LARCH_SUB_ULEB128:
    case R_LARCH_64_PCREL:
    case R_LARCH_TLS_LE_HI20:  /* local-exec TLS: offsets from $tp, no GOT */
    case R_LARCH_TLS_LE_LO12:
    case R_LARCH_TLS_LE64_LO20:
    case R_LARCH_TLS_LE64_HI12:
    case R_LARCH_TLS_LE_HI20_R:
    case R_LARCH_TLS_LE_LO12_R:
    case R_LARCH_TLS_LE_ADD_R:
        return NO_GOTPLT_ENTRY;

    case R_LARCH_B16:
    case R_LARCH_B21:
    case R_LARCH_B26:
    case R_LARCH_CALL36:
    case R_LARCH_PCALA_HI20:
    case R_LARCH_PCALA_LO12:
    case R_LARCH_ABS_HI20:
    case R_LARCH_ABS_LO12:
    case R_LARCH_ABS64_LO20:
    case R_LARCH_ABS64_HI12:
    case R_LARCH_PCALA64_LO20:
    case R_LARCH_PCALA64_HI12:
    case R_LARCH_32:
    case R_LARCH_64:
    case R_LARCH_32_PCREL:
        return AUTO_GOTPLT_ENTRY;

    case R_LARCH_GOT_HI20:
    case R_LARCH_GOT_LO12:
    case R_LARCH_GOT_PC_HI20:
    case R_LARCH_GOT_PC_LO12:
    case R_LARCH_GOT64_PC_LO20:
    case R_LARCH_GOT64_PC_HI12:
        return ALWAYS_GOTPLT_ENTRY;
    }
    return -1;
}

ST_FUNC unsigned create_plt_entry(TCCState *s1, unsigned got_offset, struct sym_attr *attr)
{
    Section *plt = s1->plt;
    uint8_t *p;
    unsigned plt_offset;

    if (plt->data_offset == 0)
        section_ptr_add(plt, 32);
    plt_offset = plt->data_offset;

    p = section_ptr_add(plt, 16);
    write64le(p, got_offset);
    return plt_offset;
}

/* relocate the PLT: same layout and lazy-binding contract as GNU ld's
   LoongArch PLT (and tcc's riscv64 one): $t0 = link map (got[1]),
   $t1 = 8 * slot index, $t3 = resolver (got[0]).  pcaddu12i is plain
   PC + (si20 << 12), so hi/lo is the RISC-V auipc split. */
ST_FUNC void relocate_plt(TCCState *s1)
{
    uint8_t *p, *p_end;

    if (!s1->plt)
      return;

    p = s1->plt->data;
    p_end = p + s1->plt->data_offset;

    if (p < p_end) {
        uint64_t plt = s1->plt->sh_addr;
        uint64_t got = s1->got->sh_addr;
        uint64_t off = (got - plt + 0x800) >> 12;
        if ((off + ((uint32_t)1 << 20)) >> 21)
            tcc_error_noabort("Failed relocating PLT (off=0x%lx, got=0x%lx, plt=0x%lx)", (long)off, (long)got, (long)plt);
        write32le(p, 0x1c00000e | ((off & 0xfffff) << 5));   // pcaddu12i $t2, %pcrel_hi(got)
        write32le(p + 4, 0x0011bdad);                        // sub.d $t1, $t1, $t3
        write32le(p + 8, 0x28c001cf                          // ld.d $t3, $t2, %pcrel_lo(got)
                         | (((got - plt) & 0xfff) << 10));
        write32le(p + 12, 0x02ff51ad);                       // addi.d $t1, $t1, -(32+12)
        write32le(p + 16, 0x02c001cc                         // addi.d $t0, $t2, %pcrel_lo(got)
                          | (((got - plt) & 0xfff) << 10));
        write32le(p + 20, 0x004505ad);                       // srli.d $t1, $t1, log2(16/PTRSIZE)
        write32le(p + 24, 0x28c0218c);                       // ld.d $t0, $t0, PTRSIZE
        write32le(p + 28, 0x4c0001e0);                       // jr $t3
        p += 32;
        while (p < p_end) {
            uint64_t pc = plt + (p - s1->plt->data);
            uint64_t addr = got + read64le(p);
            uint64_t off = (addr - pc + 0x800) >> 12;
            if ((off + ((uint32_t)1 << 20)) >> 21)
                tcc_error_noabort("Failed relocating PLT (off=0x%lx, addr=0x%lx, pc=0x%lx)", (long)off, (long)addr, (long)pc);
            write32le(p, 0x1c00000f | ((off & 0xfffff) << 5)); // pcaddu12i $t3, %pcrel_hi(func@got)
            write32le(p + 4, 0x28c001ef                        // ld.d $t3, $t3, %pcrel_lo(func@got)
                             | (((addr - pc) & 0xfff) << 10));
            write32le(p + 8, 0x4c0001ed);                      // jirl $t1, $t3, 0
            write32le(p + 12, 0x03400000);                     // nop
            p += 16;
        }
    }

    if (s1->plt->reloc) {
        ElfW_Rel *rel;
        p = s1->got->data;
        for_each_elem(s1->plt->reloc, 0, rel, ElfW_Rel) {
            write64le(p + rel->r_offset, s1->plt->sh_addr);
        }
    }
}

ST_FUNC void relocate(TCCState *s1, ElfW_Rel *rel, int type, unsigned char *ptr,
              addr_t addr, addr_t val)
{
    int sym_index = ELFW(R_SYM)(rel->r_info);

    switch(type) {
    case R_LARCH_RELAX:    /* relax 标记: 不做任何事 */
    case R_LARCH_MARK_LA:
    case R_LARCH_MARK_PCREL:
    case R_LARCH_DELETE:
    case R_LARCH_CFA:
    case R_LARCH_ALIGN:
        return;
    case R_LARCH_PCREL20_S2: {
        /* pcaddi: si20 = (S+A-P) >> 2, imm20@[24:5] (7 位操作码掩码 0xfe00001f) */
        int64_t off = val - addr;
        uint32_t insn = read32le(ptr);
        if ((off + (1 << 21)) & ~(((uint64_t)1 << 22) - 4))
            tcc_error_noabort("R_LARCH_PCREL20_S2 relocation failed");
        insn = (insn & 0xfe00001fu) | ((((uint32_t)(off >> 2)) & 0xfffff) << 5);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_ADD6:
        *ptr = (*ptr & ~0x3f) | ((*ptr + val) & 0x3f);
        return;
    case R_LARCH_SUB6:
        *ptr = (*ptr & ~0x3f) | ((*ptr - val) & 0x3f);
        return;
    case R_LARCH_ADD_ULEB128:
    case R_LARCH_SUB_ULEB128: {
        /* uleb128 原地加减(dwarf/.eh_frame 用): 读出-加减-等长写回 */
        uint8_t *p = ptr;
        uint64_t v = 0, shift = 0, b;
        int len = 0;
        do { b = p[len++]; v |= (uint64_t)(b & 0x7f) << shift; shift += 7; } while (b & 0x80);
        v = (type == R_LARCH_ADD_ULEB128) ? v + val : v - val;
        /* 等长重写; 需要更长则报错(relax 场景 binutils 保证等长) */
        {
            uint64_t t = v; int need = 0;
            do { need++; t >>= 7; } while (t);
            if (need > len) {
                tcc_error_noabort("R_LARCH_(ADD|SUB)_ULEB128: growth unsupported");
                return;
            }
        }
        for (b = 0; b < (uint64_t)len; b++) {
            uint8_t out = (v >> (7 * b)) & 0x7f;
            p[b] = out | (b + 1 < (uint64_t)len ? 0x80 : 0);
        }
        return;
    }
    case R_LARCH_64_PCREL:
        add64le(ptr, val - addr);
        return;
    case R_LARCH_ADD8:
        *ptr += val;
        return;
    case R_LARCH_ADD16:
        write16le(ptr, read16le(ptr) + val);
        return;
    case R_LARCH_ADD24: {
        /* *(int24_t *)P += S+A: three bytes only, the fourth is not ours */
        uint32_t v = (ptr[0] | ptr[1] << 8 | ptr[2] << 16) + (uint32_t)val;
        ptr[0] = v, ptr[1] = v >> 8, ptr[2] = v >> 16;
        return;
    }
    case R_LARCH_ADD32:
        write32le(ptr, read32le(ptr) + val);
        return;
    case R_LARCH_ADD64:
        write64le(ptr, read64le(ptr) + val);
        return;
    case R_LARCH_SUB8:
        *ptr -= val;
        return;
    case R_LARCH_SUB16:
        write16le(ptr, read16le(ptr) - val);
        return;
    case R_LARCH_SUB24: {
        uint32_t v = (ptr[0] | ptr[1] << 8 | ptr[2] << 16) - (uint32_t)val;
        ptr[0] = v, ptr[1] = v >> 8, ptr[2] = v >> 16;
        return;
    }
    case R_LARCH_SUB32:
        write32le(ptr, read32le(ptr) - val);
        return;
    case R_LARCH_SUB64:
        write64le(ptr, read64le(ptr) - val);
        return;

    case R_LARCH_B26: {
        /* b/bl: si26 = (S+A-PC)>>2; bits[25:10]=si26[15:0], bits[9:0]=si26[25:16]
           (原实现把两个字段的移位写反了: 高 10 位误 <<10 到 [19:10],
            低 16 位忘了 <<10 到 [25:10]) */
        int64_t off = val - addr;
        uint32_t insn = read32le(ptr);
        if ((off + (1 << 27)) & ~(((uint64_t)1 << 28) - 4))
            tcc_error_noabort("R_LARCH_B26 relocation failed (val=%lx addr=%lx)", (long)val, (long)addr);
        insn = (insn & 0xfc000000u) | (((off >> 2) & 0xffff) << 10)
               | ((off >> 18) & 0x3ff);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_CALL36: {
        /* pcaddu18i rd, si20 ; jirl rd, rd, si16 (medium code model calls,
           e.g. in glibc's crt1.o): si20 = (S+A-P+0x20000) >> 18,
           si16 = (S+A-P - (si20 << 18)) >> 2 */
        int64_t off = val - addr, hi = (off + 0x20000) >> 18;
        if ((off + ((int64_t)1 << 37)) & ~(((int64_t)1 << 38) - 4))
            tcc_error_noabort("R_LARCH_CALL36 relocation failed");
        write32le(ptr, (read32le(ptr) & 0xfe00001fu) | ((hi & 0xfffff) << 5));
        write32le(ptr + 4, (read32le(ptr + 4) & 0xfc0003ffu)
                           | ((((off - (hi << 18)) >> 2) & 0xffff) << 10));
        return;
    }
    case R_LARCH_B21: {
        /* beqz/bnez: si21 = (S+A-PC)>>2; bits[25:10]=si21[15:0], bits[4:0]=si21[20:16],
           bits[9:5]=rj 必须保留(原实现 mask 0xfc000000 把 rj 清零) */
        int64_t off = val - addr;
        uint32_t insn = read32le(ptr);
        if ((off + (1 << 22)) & ~(((uint64_t)1 << 23) - 4))
            tcc_error_noabort("R_LARCH_B21 relocation failed");
        insn = (insn & 0xfc0003e0u) | (((off >> 2) & 0xffff) << 10)
               | ((off >> 18) & 0x1f);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_B16: {
        /* beq/bne/blt/...: si16 = (S+A-PC)>>2, 在 bits[25:10];
           bits[9:5]=rj, bits[4:0]=rd 必须保留(原实现 mask 0xfc000000 把两者清零) */
        int64_t off = val - addr;
        uint32_t insn = read32le(ptr);
        if ((off + (1 << 17)) & ~(((uint64_t)1 << 18) - 4))
            tcc_error_noabort("R_LARCH_B16 relocation failed");
        insn = (insn & 0xfc0003ffu) | (((off >> 2) & 0xffff) << 10);
        write32le(ptr, insn);
        return;
    }

    case R_LARCH_PCALA_HI20: {
        /* psABI: imm20 = (((S+A+0x800) & ~0xfff) - (P & ~0xfff)) >> 12
           pcalau12i 是页对齐语义: rd = (PC & ~0xfff) + sext20(imm20)<<12,
           与 LO12 的 addi.d 符号扩展配套后对任意 P 正确。
           原实现 (val&~0xfff)-(addr&~0xfff) 缺 +0x800 进位,
           符号地址低 12 位 >= 0x800 时结果差 4KB。 */
        int64_t off = ((val + 0x800) & ~(int64_t)0xfff) - (addr & ~(int64_t)0xfff);
        uint32_t insn = read32le(ptr);
        off >>= 12;  /* 算术右移 */
        if (off > 0x7ffff || off < -0x80000)
            tcc_error_noabort("R_LARCH_PCALA_HI20 relocation failed (off=%lx)", (long)off);
        /* 保留掩码必须是 0xfe00001f: pcalau12i/lu12i.w/lu32i.d 等 1RI20
           格式操作码是 7 位(bits[31:25])! 0xfc00001f 会清掉 bit25,
           把 pcalau12i(0x1a000000) 变成 pcaddi(0x18000000) ——
           所有 PC 相对寻址全毁(_start 第三条指令即崩, badv=指令对) */
        insn = (insn & 0xfe00001fu) | ((((uint32_t)off) & 0xfffff) << 5);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_PCALA_LO12: {
        /* psABI: imm12 = (S+A) & 0xfff —— 不减 P!
           pcalau12i 页对齐语义下, addi.d 对 imm12 的符号扩展
           正好补偿 HI20 的 +0x800 进位。 */
        uint32_t insn = read32le(ptr);
        insn = (insn & 0xffc003ffu) | ((val & 0xfff) << 10);
        write32le(ptr, insn);
        return;
    }
    /* la.abs: lu12i.w / ori / lu32i.d / lu52i.d. ori zero-extends, so the
       psABI fields are plain slices of S+A, with no rounding. */
    case R_LARCH_ABS_HI20:
        write32le(ptr, (read32le(ptr) & 0xfe00001fu) | ((((uint64_t)val >> 12) & 0xfffff) << 5));
        return;
    case R_LARCH_ABS_LO12:
        write32le(ptr, (read32le(ptr) & 0xffc003ffu) | ((val & 0xfff) << 10));
        return;
    case R_LARCH_ABS64_LO20:
        write32le(ptr, (read32le(ptr) & 0xfe00001fu) | ((((uint64_t)val >> 32) & 0xfffff) << 5));
        return;
    case R_LARCH_ABS64_HI12:
        write32le(ptr, (read32le(ptr) & 0xffc003ffu) | ((((uint64_t)val >> 52) & 0xfff) << 10));
        return;
    /* Extreme code model: pcalau12i rA, %pc_hi20 ; addi.d rB, $zero, %pc_lo12 ;
       lu32i.d rB, %pc64_lo20 ; lu52i.d rB, rB, %pc64_hi12 ; add.d/ldx.d rA, rB.
       The high 32 bits must undo both sign extensions (addi.d's of the low
       12 bits, pcalau12i's of bit 31), relative to the pcalau12i 8 (lu32i.d)
       or 12 (lu52i.d) bytes earlier: GNU ld's RELOCATE_CALC_PC64_HI32. */
    case R_LARCH_PCALA64_LO20:
    case R_LARCH_PCALA64_HI12:
    case R_LARCH_GOT64_PC_LO20:
    case R_LARCH_GOT64_PC_HI12: {
        uint64_t t = val, pc, r;
        if (type == R_LARCH_GOT64_PC_LO20 || type == R_LARCH_GOT64_PC_HI12)
            t = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        pc = addr - (type == R_LARCH_PCALA64_LO20 || type == R_LARCH_GOT64_PC_LO20 ? 8 : 12);
        r = (t & ~(uint64_t)0xfff) - (pc & ~(uint64_t)0xfff);
        if ((t & 0xfff) > 0x7ff)
            r += 0x1000 - 0x100000000ULL;
        if (r & 0x80000000)
            r += 0x100000000ULL;
        if (type == R_LARCH_PCALA64_LO20 || type == R_LARCH_GOT64_PC_LO20)
            write32le(ptr, (read32le(ptr) & 0xfe00001fu) | (((r >> 32) & 0xfffff) << 5));
        else
            write32le(ptr, (read32le(ptr) & 0xffc003ffu) | (((r >> 52) & 0xfff) << 10));
        return;
    }
    case R_LARCH_GOT_PC_HI20: {
        /* val 是符号地址, 但 pcalau12i+ld.d 需要的是 GOT 槽地址!
           (对照 riscv64-link.c: val = got->sh_addr + got_offset;
            原实现直接填符号地址 -> ld.d 从函数入口读出两条指令当指针,
            jirl 跳到"指令对"拼成的野地址 -> ADEF, 这是全线崩溃主因)
           公式同 PCALA_HI20(含 +0x800, ld.d 的 imm12 符号扩展补偿) */
        addr_t got = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        int64_t off = ((got + 0x800) & ~(int64_t)0xfff) - (addr & ~(int64_t)0xfff);
        uint32_t insn = read32le(ptr);
        insn = (insn & 0xfe00001fu) | ((((uint32_t)(off >> 12)) & 0xfffff) << 5);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_GOT_PC_LO12: {
        /* GOT 槽地址低 12 位(不减 P, 页对齐语义) */
        addr_t got = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        uint32_t insn = read32le(ptr);
        insn = (insn & 0xffc003ffu) | ((got & 0xfff) << 10);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_GOT_HI20: {
        /* 79: 绝对形式 lu12i.w + ld.d, 同样用 GOT 槽地址 */
        addr_t got = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        uint32_t insn = read32le(ptr);
        insn = (insn & 0xfe00001fu) | ((((uint64_t)(got + 0x800) >> 12) & 0xfffff) << 5);
        write32le(ptr, insn);
        return;
    }
    case R_LARCH_GOT_LO12: {
        addr_t got = s1->got->sh_addr + get_sym_attr(s1, sym_index, 0)->got_offset;
        uint32_t insn = read32le(ptr);
        insn = (insn & 0xffc003ffu) | ((got & 0xfff) << 10);
        write32le(ptr, insn);
        return;
    }
    /* TLS local-exec. $tp points at the start of the executable's TLS
       block (TLS variant I, as on RISC-V), so the offset is S+A minus the
       lowest SHF_TLS address. HI20/LO12 pair with lu12i.w + ori (slices);
       the _R forms pair with addi.d (rounded), as gcc 13+ emits them. */
    case R_LARCH_TLS_LE_HI20:
    case R_LARCH_TLS_LE_LO12:
    case R_LARCH_TLS_LE64_LO20:
    case R_LARCH_TLS_LE64_HI12:
    case R_LARCH_TLS_LE_HI20_R:
    case R_LARCH_TLS_LE_LO12_R: {
        addr_t tls_start = 0;
        uint64_t off;
        int i;
        for (i = 1; i < s1->nb_sections; i++) {
            Section *s = s1->sections[i];
            if (s->sh_flags & SHF_TLS && s->sh_size)
                if (!tls_start || s->sh_addr < tls_start)
                    tls_start = s->sh_addr;
        }
        off = val - tls_start;
        if (type == R_LARCH_TLS_LE_HI20 || type == R_LARCH_TLS_LE_HI20_R) {
            uint64_t hi = type == R_LARCH_TLS_LE_HI20_R ? off + 0x800 : off;
            if ((int64_t)off != (int32_t)off)
                tcc_error_noabort("R_LARCH_TLS_LE_HI20 relocation failed");
            write32le(ptr, (read32le(ptr) & 0xfe00001fu) | (((hi >> 12) & 0xfffff) << 5));
        } else if (type == R_LARCH_TLS_LE64_LO20)
            write32le(ptr, (read32le(ptr) & 0xfe00001fu) | (((off >> 32) & 0xfffff) << 5));
        else if (type == R_LARCH_TLS_LE64_HI12)
            write32le(ptr, (read32le(ptr) & 0xffc003ffu) | (((off >> 52) & 0xfff) << 10));
        else
            write32le(ptr, (read32le(ptr) & 0xffc003ffu) | ((off & 0xfff) << 10));
        return;
    }
    case R_LARCH_TLS_LE_ADD_R:  /* marks the add.d $tp of the _R sequence */
        return;
    case R_LARCH_32:
        if (s1->output_type & TCC_OUTPUT_DYN) {
            qrel->r_offset = rel->r_offset;
            qrel->r_info = ELFW(R_INFO)(0, R_LARCH_RELATIVE);
            qrel->r_addend = (int)read32le(ptr) + val;
            qrel++;
        }
        add32le(ptr, val);
        return;
    case R_LARCH_64:
        if (s1->output_type & TCC_OUTPUT_DYN) {
            qrel->r_offset = rel->r_offset;
            qrel->r_info = ELFW(R_INFO)(0, R_LARCH_RELATIVE);
            qrel->r_addend = read64le(ptr) + val;
            qrel++;
        }
    case R_LARCH_JUMP_SLOT:
        add64le(ptr, val);
        return;
    case R_LARCH_32_PCREL:
        add32le(ptr, val - addr);
        return;
    case R_LARCH_COPY:
        return;
    case R_LARCH_RELATIVE:
        return;

    default:
        fprintf(stderr, "FIXME: handle reloc type %x at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
        return;
    }
}
#endif
