#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
htd_elfdl 模块自检

检查 loader 安装时依赖的几项结构, 任何一项不满足 loader 都会装不上或装错:
    1. ELF32 / LSB / EM_ARM
    2. 单段 PT_LOAD 且 p_offset == p_vaddr
    3. p_filesz == p_memsz (没有 bss)
    4. 有 .dynsym, 且至少一个 STB_GLOBAL 的函数/对象符号
    5. 重定位节全是 SHT_REL, 且符号名能在 .dynstr 里查到

--lib 时按库检查: 不要求导出 elfdl_init / elfdl_deinit, 其余结构要求一样

用法: python tools/edl_check.py [--lib] xxx.mo
"""

import struct
import sys

EM_ARM = 40
ET_DYN = 3
PT_LOAD = 1
SHT_REL = 9
SHT_DYNSYM = 11
STB_GLOBAL = 1
STT_OBJECT = 1
STT_FUNC = 2

ARM_RELOC_NAME = {
    0: 'R_ARM_NONE', 2: 'R_ARM_ABS32', 21: 'R_ARM_GLOB_DAT', 22: 'R_ARM_JUMP_SLOT',
    23: 'R_ARM_RELATIVE', 24: 'R_ARM_GOTOFF32', 25: 'R_ARM_BASE_PREL',
    26: 'R_ARM_GOT_BREL', 28: 'R_ARM_CALL', 29: 'R_ARM_JUMP24', 30: 'R_ARM_THM_JUMP24',
}


def fail(msg):
    print('[mo] FAIL: %s' % msg)
    sys.exit(1)


def main(path, is_lib=False):
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF':
        fail('not an ELF file')
    if data[4] != 1 or data[5] != 1:
        fail('need ELF32 little endian')

    (e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags,
     e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx) = \
        struct.unpack('<HHIIIIIHHHHHH', data[16:52])

    if e_machine != EM_ARM:
        fail('e_machine=%d, not EM_ARM' % e_machine)
    if e_type != ET_DYN:
        fail('e_type=%d, not ET_DYN (module must be linked as -shared)' % e_type)

    loads = []
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        (p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz,
         p_flags, p_align) = struct.unpack('<IIIIIIII', data[off:off + 32])
        if p_type == PT_LOAD:
            loads.append((p_offset, p_vaddr, p_filesz, p_memsz, p_flags))

    if len(loads) != 1:
        fail('%d PT_LOAD segments, loader needs exactly one' % len(loads))
    p_offset, p_vaddr, p_filesz, p_memsz, p_flags = loads[0]
    if p_offset != p_vaddr:
        fail('p_offset=0x%x != p_vaddr=0x%x, not identity mapped' % (p_offset, p_vaddr))
    if p_filesz != p_memsz:
        fail('p_filesz=0x%x != p_memsz=0x%x, module must not have bss' % (p_filesz, p_memsz))

    # 节表
    secs = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        secs.append(struct.unpack('<IIIIIIIIII', data[off:off + 40]))
    shstr = secs[e_shstrndx]
    shstr_data = data[shstr[4]:shstr[4] + shstr[5]]

    def name_at(n):
        end = shstr_data.find(b'\0', n)
        return shstr_data[n:end].decode('utf-8', 'replace')

    def find_sec(sec_name):
        for s in secs:
            if name_at(s[0]) == sec_name:
                return s
        return None

    dynsym = find_sec('.dynsym')
    if dynsym is None:
        fail('no .dynsym section, loader returns ERR_ELFDL_NOT_FOUND')

    dynstr = secs[dynsym[6]]
    dynstr_data = data[dynstr[4]:dynstr[4] + dynstr[5]]

    def sym_name(idx):
        st_name, st_value, st_size, st_info, st_other, st_shndx = \
            struct.unpack('<IIIBBH', data[dynsym[4] + idx * 16: dynsym[4] + idx * 16 + 16])
        end = dynstr_data.find(b'\0', st_name)
        return dynstr_data[st_name:end].decode('utf-8', 'replace'), st_value, st_info

    exported = []
    for i in range(1, dynsym[5] // 16):
        sym, val, info = sym_name(i)
        if (info >> 4) != STB_GLOBAL:
            continue
        if (info & 0xF) not in (STT_FUNC, STT_OBJECT):
            continue
        exported.append((sym, val))

    if not exported:
        fail('.dynsym has no global function/object symbol')

    # 重定位: 必须是 SHT_REL, 且 sh_link 指向 .dynsym
    relocs = []
    dynsym_index = [i for i, s in enumerate(secs) if s[1] == SHT_DYNSYM][0]
    for s in secs:
        if s[1] != SHT_REL:
            continue
        if s[6] != dynsym_index:
            continue
        for j in range(s[5] // 8):
            r_offset, r_info = struct.unpack('<II', data[s[4] + j * 8: s[4] + j * 8 + 8])
            idx = r_info >> 8
            rtype = r_info & 0xFF
            sym = '<empty>'
            if idx:
                sym, _, _ = sym_name(idx)
            relocs.append((sym, rtype))

    # 入口
    entry_ok = (e_entry == 0) or (p_vaddr <= e_entry < p_vaddr + p_filesz)

    print('[mo] %s' % path)
    print('  machine  : EM_ARM, ET_DYN, entry=0x%08x' % e_entry)
    print('  load     : 1 segment, offset=0x%x == vaddr=0x%x, filesz=0x%x, memsz=0x%x, flags=%d'
          % (p_offset, p_vaddr, p_filesz, p_memsz, p_flags))
    print('  dynsym   : %d exported (%s)' % (len(exported), ', '.join([x[0] for x in exported])))
    print('  reloc    : %d SHT_REL' % len(relocs))
    for sym, rtype in relocs:
        print('             %-22s type=%d(%s)' % (sym, rtype, ARM_RELOC_NAME.get(rtype, '?')))
    print('  size     : %d bytes' % len(data))

    if not is_lib:
        for fn in ('elfdl_init', 'elfdl_deinit'):
            if fn not in [x[0] for x in exported]:
                fail('module must export %s()' % fn)
    if not entry_ok:
        fail('entry 0x%x out of program space' % e_entry)

    print('  result   : OK')


if __name__ == '__main__':
    opts = [a for a in sys.argv[1:] if a.startswith('-')]
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if not args:
        print('usage: edl_check.py [--lib] <module.mo>')
        sys.exit(-1)
    main(args[0], '--lib' in opts)
