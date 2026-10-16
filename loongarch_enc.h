# LoongArch64 指令编码表(从 enc.txt 反推,已验证)
# 统一格式: 指令 = 模板常量 | 操作数字段
#   3R:    rd[4:0] | rj[9:5] | rk[14:10]
#   2RI12: rd[4:0] | rj[9:5] | imm12[21:10]
#   移位:  rd[4:0] | rj[9:5] | ui5/ui6[15:10]
#   分支:  rd[4:0] | rj[9:5] | offs16[25:10](offs = 字节偏移>>2)
#   jirl:  rd[4:0] | rj[9:5] | offs16[25:10]

# === 3R 格式 ===
ADD_D   = 0x00108000   # add.d rd, rj, rk
SUB_D   = 0x00118000   # sub.d
AND     = 0x00148000   # and
OR      = 0x00150000   # or (move = or rd, rj, r0)
XOR     = 0x00158000   # xor
SLL_D   = 0x00188000   # sll.d
SRL_D   = 0x00190000   # srl.d
SRA_D   = 0x00198000   # sra.d
MUL_D   = 0x001d8000   # mul.d
DIV_D   = 0x00220000   # div.d
MOD_D   = 0x00228000   # mod.d
DIV_DU  = 0x00230000   # div.du
MOD_DU  = 0x00238000   # mod.du
SLT     = 0x00120000   # slt
SLTU    = 0x00128000   # sltu
FADD_D  = 0x01010000   # fadd.d
FSUB_D  = 0x01030000   # fsub.d
FMUL_D  = 0x01050000   # fmul.d
FDIV_D  = 0x01070000   # fdiv.d
FADD_S  = 0x01008000   # fadd.s

# === 2RI12 格式 ===
ADDI_D  = 0x02c00000   # addi.d rd, rj, si12
ADDI_W  = 0x02800000   # addi.w (li.w = addi.w rd, r0, si12)
ANDI    = 0x03400000   # andi rd, rj, ui12
ORI     = 0x03800000   # ori
XORI    = 0x03c00000   # xori
SLTUI   = 0x02400000   # sltui rd, rj, ui12
LD_B    = 0x28000000   # ld.b
LD_H    = 0x28400000   # ld.h
LD_W    = 0x28800000   # ld.w
LD_D    = 0x28c00000   # ld.d
ST_B    = 0x29000000   # st.b (修正: 原 0x28800000 是 ld.w)
ST_H    = 0x29400000   # st.h (修正: 原 0x28c00000 是 ld.d)
ST_W    = 0x29800000   # st.w
ST_D    = 0x29c00000   # st.d
FLD_S   = 0x2b000000   # fld.s
FLD_D   = 0x2b800000   # fld.d
FST_S   = 0x2b400000   # fst.s
FST_D   = 0x2bc00000   # fst.d
LDPTR_W = 0x24000000   # ldptr.w (14位无符号偏移)
LDPTR_D = 0x26000000   # ldptr.d
STPTR_W = 0x25000000   # stptr.w
STPTR_D = 0x27000000   # stptr.d

# === 移位(ui5/ui6) ===
SLLI_W  = 0x00408000   # slli.w rd, rj, ui5
SLLI_D  = 0x00410000   # slli.d rd, rj, ui6
SRLI_D  = 0x00450000   # srli.d
SRAI_D  = 0x00490000   # srai.d

# === 分支/跳转 ===
B       = 0x50000000   # b offs26
BL      = 0x54000000   # bl offs26
BEQ     = 0x58000000   # beq rj, rd, offs16
BNE     = 0x5c000000   # bne
BLT     = 0x60000000   # blt
BGE     = 0x64000000   # bge
BLTU    = 0x68000000   # bltu
BGEU    = 0x6c000000   # bgeu
JIRL    = 0x4c000000   # jirl rd, rj, offs16 (ret = jirl r0, r1, 0 = 0x4c000020)
BEQZ    = 0x40000000   # beqz rj, offs21 (修正: 原 0x54000000 与 BL 冲突)
BNEZ    = 0x44000000   # bnez rj, offs21

# === 浮点/转换 ===
FCMP_SLT_D = 0x0c218020  # fcmp.slt.d fcc, rj, rk
MOVCF2FR  = 0x0114d400   # movcf2fr fd, fcc
MOVFR2GR_S = 0x0114b40c  # movfr2gr.s rd, fj
MOVFR2GR_D = 0x0114b80c  # movfr2gr.d rd, fj
MOVGR2FR_D = 0x0114a980  # movgr2fr.d fd, rj
FFINT_D_L  = 0x011d2800  # ffint.d.l fd, fj (fcvt.d.l)
FTINTRZ_L_D = 0x011aa800 # ftintrz.l.d fd, fj (fcvt.l.d, rtz)
FCVT_S_D   = 0x01191800  # fcvt.s.d fd, fj
FCVT_D_S   = 0x01192400  # fcvt.d.s fd, fj
NOP        = 0x03400000  # nop (andi r0, r0, 0)
BREAK      = 0x002a0007  # break 0x7

# === 大地址加载(已验证, 见 loongarch64-gen.c) ===
LU12I_W    = 0x14000000   # lu12i.w rd, si20:  rd = sext32(si20 << 12)
LU32I_D    = 0x16000000   # lu32i.d rd, si20:  rd[51:32] = si20
LU52I_D    = 0x03000000   # lu52i.d rd, rj, si12: rd[63:52] = si12
PCALAU12I  = 0x1a000000   # pcalau12i rd, si20: rd = PC + sext(si20<<12), 配 PCALA_HI20/LO12
PCADDU12I  = 0x1c000000   # pcaddu12i rd, si20: 配 PCADD_HI20/LO12
# === 尚未验证(需补充反汇编): ===
#  FCVT 系列其他(fcvt.s.l, fcvt.l.s, fcvt.w.d 等)
#  bstrpick.w
