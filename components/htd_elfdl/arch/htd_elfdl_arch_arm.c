// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG   "elfdl.arm"
#include "htd_elfdl_process.h"


#if defined(__arm__) || defined(ELFDL_LOAD_IN_SIMULATE)
// @brief 地址重定向
elf_addr_t htd_elfdl_addr_relocate(symlink_info_t* p_relink)
{
    elf_addr_t tmp;
    int32_t addend, offset;
    uint32_t upper, lower, sign, j1, j2;

    // 原有地址上的值
    elf_addr_t addr_val = p_relink->where_val;
    uint32_t rel_type = ELF_R_TYPE(p_relink->rel.r_info);
    print_debug("rel_type=%d", rel_type);
    switch (rel_type) {
        case R_ARM_NONE: {
            break;
        }
        case R_ARM_ABS32: {         // 这是一个绝对地址重定位。它表示一个32位的地址应该被解析，并且应该用该地址在目标文件的实际地址替换它。
            addr_val = addr_val + p_relink->symbol;
            print_debug("ARM_ABS32: %x, %x", addr_val, p_relink->symbol);
            break;
        }
        case R_ARM_PC24:
        case R_ARM_PLT32:
        case R_ARM_CALL:            // 这是一个ARM的thm（Thumb）指令的调用重定位。它表示一个函数调用的地址应该被解析，并且应该用该函数在目标文件的实际地址替换它
        case R_ARM_JUMP24: {
            addend = addr_val & 0x00ffffff;
            if (addend & 0x00800000) {
                addend |= 0xff000000;
            }
            tmp = p_relink->symbol - p_relink->where + (addend << 2);
            tmp >>= 2;
            addr_val = (addr_val & 0xff000000) | (tmp & 0x00ffffff);
            break;
        }
        case R_ARM_REL32: {
            addr_val = addr_val + (p_relink->symbol - p_relink->where);
            break;
        }
        case R_ARM_V4BX:  {
            addr_val &= 0xf000000f;
            addr_val |= 0x01a0f000;
            break;
        }
        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT: {         // 外部变量或者函数
            addr_val = p_relink->symbol;
            break;
        }
        case R_ARM_RELATIVE: {          // 内部全局变量或者静态函数
            addr_val = p_relink->symbol + addr_val;
            break;
        }
        case R_ARM_THM_CALL:
        case R_ARM_THM_JUMP24: {
            upper = addr_val & 0xffff;
            lower = (addr_val >> 16) & 0xffff;
            sign = (upper >> 10) & 1;
            j1 = (lower >> 13) & 1;
            j2 = (lower >> 11) & 1;
            offset = (sign << 24) |
                     ((~(j1 ^ sign) & 1) << 23) |
                     ((~(j2 ^ sign) & 1) << 22) |
                     ((upper & 0x03ff) << 12) |
                     ((lower & 0x07ff) << 1);
            if (offset & 0x01000000) {
                offset -= 0x02000000;
            }
            offset += p_relink->symbol - p_relink->where;
            if (!(offset & 1) || offset <= (int32_t)0xff000000 || offset >= (int32_t)0x01000000) {
                return ELF_INVALID_ADDR;
            }

            sign = (offset >> 24) & 1;
            j1 = sign ^ (~(offset >> 23) & 1);
            j2 = sign ^ (~(offset >> 22) & 1);
            upper = (uint16_t)((upper & 0xf800) | (sign << 10) | ((offset >> 12) & 0x03ff));
            lower = (uint16_t)((lower & 0xd000) | (j1 << 13) | (j2 << 11) | ((offset >> 1) & 0x07ff));
            addr_val = ((lower & 0xffff) << 16) | (upper & 0xffff);
            break;
        }
        default: {
            return ELF_INVALID_ADDR;
        }
    }
    return addr_val;
}
#endif

