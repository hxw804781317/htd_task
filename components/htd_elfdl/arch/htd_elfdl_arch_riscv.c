// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG   "elfdl.riscv"
#include "htd_elfdl_process.h"


#if (__riscv_xlen == 64)
#define R_RISCV_NONE           0
#define R_RISCV_32             1
#define R_RISCV_64             2
#define R_RISCV_RELATIVE       3
#define R_RISCV_COPY           4
#define R_RISCV_JUMP_SLOT      5
#define R_RISCV_TLS_DTPMOD32   6
#define R_RISCV_TLS_DTPMOD64   7
#define R_RISCV_TLS_DTPREL32   8
#define R_RISCV_TLS_DTPREL64   9
#define R_RISCV_TLS_TPREL32   10
#define R_RISCV_TLS_TPREL64   11


// @brief 地址重定向
elf_addr_t htd_elfdl_addr_relocate(symlink_info_t* p_relink)
{
    elf_addr_t addr_val = p_relink->where;
    uint32_t rel_type = ELF_R_TYPE(p_relink->rel.r_info);
    print_debug("rel_type=%d", rel_type);
    switch (rel_type) {
        case R_RISCV_NONE:
            break;
        case R_RISCV_64:
            addr_val = p_relink->symbol + p_relink->rel.r_addend;
            print_debug("R_RISCV_64: %x", addr_val);
            break;
        case R_RISCV_RELATIVE:
            addr_val = p_relink->where + p_relink->rel.r_addend - p_relink->rel.r_offset;
            print_debug("R_RISCV_RELATIVE: %x", addr_val);
            break;
        case R_RISCV_JUMP_SLOT:
            addr_val = p_relink->symbol;
            print_debug("R_RISCV_JUMP_SLOT: %x", addr_val);
            break;
        default:
            print_debug("__riscv__ELF: invalid relocate TYPE %d", rel_type);
            return ELF_INVALID_ADDR;
    }
    return addr_val;
}
#endif

