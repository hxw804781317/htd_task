// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG       "program.elf_parse"
#include "htd_elfdl_process.h"
#include "htd_elfdl_parse.h"


//////////////////////////////////////////////////////////////////////
static const char* section_type_table[] = {
    "NULL", 
    "PROGBITS", 
    "SYMTAB", 
    "STRTAB", 
    "RELA", 
    "HASH", 
    "DYNAMIC", 
    "NOTE", 
    "NOBITS", 
    "REL", 
    "SHLIB", 
    "DYNSYM", 
    "NUM",
    "ARM_ATTRIBUTES"
};

static const char* dynamic_section_tag_name_table[] = {
    "NULL",
    "NEEDED",
    "PLTRELSZ",
    "PLTGOT",
    "HASH",
    "STRTAB",
    "SYMTAB",
    "RELA",
    "RELASZ",
    "RELAENT",
    "STRSZ",
    "SYMENT",
    "INIT",
    "FINI",
    "SONAME",
    "RPATH",
    "SYMBOLIC",
    "REL",
    "RELSZ",
    "RELENT",
    "PLTREL",
    "DEBUG",
    "TEXTREL",
    "JMPREL",
    "BIND_NOW",
};



//////////////////////////////////////////////////////////////////////
// @brief 获取动态模块名字
const char* elf_dynamic_tag_name_get(uint32_t tag)
{
    if (tag < ARRAY_COUNT(dynamic_section_tag_name_table)) {
        return dynamic_section_tag_name_table[tag];
    } else if (tag == DT_LOOS) {
        return "LOOS";
    } else if (tag == DT_HIOS) {
        return "HIOS";
    } else if (tag == DT_LOPROC) {
        return "LOPROC";
    } else if (tag == DT_HIPROC) {
        return "HIPROC";
    } else if (tag == DT_RELACOUNT) {
        return "RELACOUNT";
    } else if (tag == DT_RELCOUNT) {
        return "RELCOUNT";
    } else if (tag == DT_FLAGS_1) {
        return "FLAGS_1";
    } else {
        return "NULL";
    }
}

// @brief 判断是否为elf文件
bool elf_file_head_test(const elf_hdr_t* p_hdr)
{
    return (p_hdr->e_ident[0] == ELFMAG0 && 
           p_hdr->e_ident[1] == ELFMAG1 && 
           p_hdr->e_ident[2] == ELFMAG2 &&
           p_hdr->e_ident[3] == ELFMAG3);
}

// @brief 查找相关的节
const elf_shdr_t* elf_section_find(const char* p_elf_data, const char* p_section_name)
{
    const char* p_name;
    uint32_t name_len;
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    const elf_shdr_t* p_shdr = (const elf_shdr_t*)&p_elf_data[p_hdr->e_shoff];
    const char* p_shstrab = p_elf_data + p_shdr[p_hdr->e_shstrndx].sh_offset;
    uint32_t section_name_len = htd_strlen(p_section_name);
    for (int i=0; i<p_hdr->e_shnum; ++i) {
        p_name = (const char*)(p_shstrab + p_shdr->sh_name);
        name_len = htd_strlen(p_name);
        if (name_len == section_name_len && 
            htd_memcmp(p_name, p_section_name, section_name_len) == 0) {
            return p_shdr;
        }
        p_shdr++;
    }
    return NULL;
}

// @brief 获取 symtab 的字符表 strtab
const elf_shdr_t* elf_shdr_symtab_strtab_get(const elf_shdr_t* p_symtab, const char* p_elf_data)
{
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    const elf_shdr_t* p_shdr = (const elf_shdr_t*)&p_elf_data[p_hdr->e_shoff];
    if (p_hdr->e_shnum > p_symtab->sh_link) {
        return &p_shdr[p_symtab->sh_link];
    }
    return NULL;
}

// @brief 解析elf头部
uint32_t elf_head_print(const char* p_elf_data)
{
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    htd_print("ELF Header: \r\n");
    print_hex("  identification: ", p_hdr->e_ident, EI_NIDENT);
    htd_print("  type: %04x, machine: %04x\r\n", p_hdr->e_type, p_hdr->e_machine);
    htd_print("  version: %08x, entry: %08x\r\n", p_hdr->e_version, p_hdr->e_entry);
    htd_print("  程序头起点:               %18d\r\n", p_hdr->e_phoff);
    htd_print("  Start of section headers: %18d\r\n", p_hdr->e_shoff);
    htd_print("  flag=%08x\r\n", p_hdr->e_flags);
    htd_print("  本头大小：                %d\r\n", p_hdr->e_ehsize);
    htd_print("  程序头大小：              %d\r\n", p_hdr->e_phentsize);
    htd_print("  程序头数量：              %d\r\n", p_hdr->e_phnum);
    htd_print("  节头大小：                %d\r\n", p_hdr->e_shentsize);
    htd_print("  节头数量：                %d\r\n", p_hdr->e_shnum);
    htd_print("  节名字表索引值：          %d\r\n", p_hdr->e_shstrndx);
    htd_print("\r\n\r\n");
    return sizeof(elf_hdr_t);
}

// @brief 打印section头部信息
void elf_shdr_print(const char* p_elf_data)
{
    uint32_t i;
    uint32_t offset;
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    const elf_shdr_t* p_shdr;

    // 指向段strtab
    p_shdr = (const elf_shdr_t*)&p_elf_data[p_hdr->e_shoff];
    const char* p_shstrab = p_elf_data + p_shdr[p_hdr->e_shstrndx].sh_offset;

    htd_print("节头: \r\n");
    htd_print("  [Nr] Name            |Type          |Addr       |Offset     |Size    |ES  |Flag|Lk|Info|Al|name_idx\r\n");
    for (i=0; i<p_hdr->e_shnum; ++i) {
        offset = p_shdr->sh_type == 0x70000003 ? 13 : p_shdr->sh_type;
        htd_print("  [%2d] %-16.16s|%-14s", i, p_shstrab+p_shdr->sh_name, section_type_table[offset]);
        htd_print("|0x%08x |0x%08x ", p_shdr->sh_addr, p_shdr->sh_offset);
        htd_print("|%-8u|0x%02x", p_shdr->sh_size, p_shdr->sh_entsize);
        htd_print("|%-4d|%-2d", p_shdr->sh_flags, p_shdr->sh_link);
        htd_print("|%-4d|%-2d|%d\r\n", p_shdr->sh_info, p_shdr->sh_addralign, p_shdr->sh_name);
        p_shdr++;
    }
    htd_print("  Offset: 文件偏移量, Size: 段大小\r\n");
    htd_print("\r\n\r\n");
}

// @brief 打印程序头部信息
void elf_program_hdr_print(const char* p_elf_data)
{
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    const elf_phdr_t* p_phdr;
    uint32_t vstart_addr = 0xffffffff, vend_addr = 0;

    htd_print("程序头: \r\n");
    htd_print("  |Type|Offset   |VirtAddr   |PhysAddr   |FileSize |MemSize  |Flag       |Align\r\n");
    p_phdr = (const elf_phdr_t*)&p_elf_data[p_hdr->e_phoff];
    for (int i=0; i<p_hdr->e_phnum; ++i) {
        htd_print("  |%-4d|0x%06x |0x%08x ", p_phdr->p_type, p_phdr->p_offset, p_phdr->p_vaddr);
        htd_print("|0x%08x |0x%06x |0x%06x ", p_phdr->p_paddr, p_phdr->p_filesz, p_phdr->p_memsz);
        htd_print("|0x%08x |0x%x\r\n", p_phdr->p_flags, p_phdr->p_align);
        if (p_phdr->p_type == PT_LOAD) {
            if (vstart_addr > p_phdr->p_vaddr) {
                vstart_addr = p_phdr->p_vaddr;
            }
            if (vend_addr < p_phdr->p_vaddr + p_phdr->p_memsz){
                vend_addr = p_phdr->p_vaddr + p_phdr->p_memsz;
            }
        }
        p_phdr++;
    }
    htd_print("  虚拟程序地址: %x->%x, 大小: %x\r\n", vstart_addr, vend_addr, vend_addr-vstart_addr);
    htd_print("\r\n\r\n");
}

// @brief 打印 symtab
void elf_section_symtab_print(const char* p_elf_data)
{
    const char* p_section_name = ".symtab";
    const elf_shdr_t* p_shdr = elf_section_find(p_elf_data, p_section_name);
    if (NULL == p_shdr) {
        p_section_name = ".dynsym";
        p_shdr = elf_section_find(p_elf_data, p_section_name);
        if (p_shdr == NULL) {
            htd_print("There are no section groups in this file.\r\n\r\n\r\n");
            return;
        }
    }
    
    const elf_shdr_t* p_shdr_strtab = elf_shdr_symtab_strtab_get(p_shdr, p_elf_data);
    if (NULL == p_shdr_strtab) {
        htd_print("Thear are no section strtab in this file.\r\n\r\n\r\n");
        return;
    }

    uint32_t i = 0;
    uint32_t sh_cnt = p_shdr->sh_size / p_shdr->sh_entsize; //sizeof(elf_sym_t);
    htd_print("Symbol table '%s' contains %d entries:\r\n", p_section_name, sh_cnt);
    htd_print("  |Num:  |Value   |Size  |Type |Bind |Ndx     |Name\r\n");
    const char* p_name = &p_elf_data[p_shdr_strtab->sh_offset];
    const elf_sym_t* p_symbol = (const elf_sym_t*)&p_elf_data[p_shdr->sh_offset];
    while (sh_cnt--) {
        htd_print("  |%-6d|%-8d|%-6d|%-5d|%-5d", i, p_symbol->st_value, p_symbol->st_size, ELF_ST_TYPE(p_symbol->st_info), ELF_ST_BIND(p_symbol->st_info));
        htd_print("|%-8d|%-32.32s\r\n", p_symbol->st_shndx, &p_name[p_symbol->st_name]);
        i++;
        p_symbol++;
    }
    htd_print("\r\n\r\n");
}

// @brief 打印 dynamic 节
void elf_section_dynamic_print(const char* p_elf_data)
{
    const elf_shdr_t* p_shdr = elf_section_find(p_elf_data, ".dynamic");
    if (NULL == p_shdr) {
        return;
    }

    uint32_t count = 1;
    const elf32_dyn_t* p_info = (const elf32_dyn_t*)&p_elf_data[p_shdr->sh_offset];
    const elf32_dyn_t* p_start = p_info;
    while (p_start->d_tag) {
        count++;
        p_start++;
    }
    
    htd_print("Dynamic section at offset 0x%x contains %d entries:\r\n", p_shdr->sh_offset, count);
    htd_print("  |标记        |类型            |Name/Value\r\n");
    while (count--) {
        htd_print("  |0x%08x  |%-16s|0x%x\r\n", p_info->d_tag, elf_dynamic_tag_name_get(p_info->d_tag), p_info->d_un.d_val);
        p_info++;
    }
    htd_print("\r\n\r\n");
}

// @brief 打印 .rel.dyn 节信息
void elf_section_rel_dyn_print(const char* p_elf_data)
{
    const elf_shdr_t* p_shdr = elf_section_find(p_elf_data, ".rel.dyn");
    if (NULL == p_shdr) {
        return;
    }

    uint32_t idx;
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)p_elf_data;
    const elf_shdr_t *p_shdr_start = (const elf_shdr_t*)(p_elf_data + p_hdr->e_shoff);
    const elf_relocate_t* p_relocate = (const elf_relocate_t*)&p_elf_data[p_shdr->sh_offset];
    uint32_t count = p_shdr->sh_size / p_shdr->sh_entsize;
    idx = p_shdr->sh_link;
    const elf_sym_t *symtab = (const elf_sym_t*)(p_elf_data + p_shdr_start[idx].sh_offset);
    // 名字表位置
    idx = p_shdr_start[idx].sh_link;
    const char * p_strtab = p_elf_data + p_shdr_start[idx].sh_offset;
    
    htd_print("重定位节 '.rel.dyn' at offset 0x%x contains %d entries:\r\n", p_shdr->sh_offset, count);
    htd_print("  |偏移量     |信息       |符号值      |符号名称\r\n");
    while (count--) {
        const elf_sym_t *sym = &symtab[ELF_R_SYM(p_relocate->r_info)];
        htd_print("  |0x%08x |0x%08x ", p_relocate->r_offset, p_relocate->r_info);
        if (ELF_ST_BIND(sym->st_info) != STB_LOCAL) {  
            htd_print("|0x%08x |%s", sym->st_value, p_strtab + sym->st_name);
        }
        htd_print("  |st_shndx=%d, st_info=%x\r\n", sym->st_shndx, sym->st_info);
        p_relocate++;
    }
    htd_print("\r\n\r\n");
}



/////////////////////////////////////////////////////////////////////////////
#if (1 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE))
#include "htd_console_process.h"
#include "htd_vfs_process.h"


// @brief 加载整个文件到内存中
uint8_t* file_load_to_memory(const char* p_filepath, uint32_t* size)
{
    int fd = open(p_filepath, O_BINARY|O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    uint32_t filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    uint8_t* p_data = htd_malloc(filesize);
    if (NULL == p_data) {
        close(fd);
        return NULL;
    }
    
    htd_memset(p_data, 0, filesize);
    int32_t ret = read(fd, p_data, filesize);
    if (ret != filesize) {
        print_error("file read failed. filesize=%d", filesize);
        close(fd);
        htd_free(p_data);
        return NULL;
    }
    close(fd);

    if (size) {
        *size = filesize;
    }
    return p_data;
}

//////////////////////////////////////////////////////////////////////
// 模块表: 控制台手动验证用, 记录本次运行安装过的模块
// 掉电重启后需要重新 load, 不复用 .d 文件里落盘的指针
typedef struct {
    htd_list_node_t node;
    elfdl_module_t m_module;
    char name[ELFDL_PATH_MAX_LEN];
} elfdl_module_node_t;
static htd_list_head_t m_module_list;
static bool m_module_list_inited = false;

// @brief 初始化模块表
static void elf_module_list_init(void)
{
    if (m_module_list_inited == false) {
        htd_list_head_init(&m_module_list);
        m_module_list_inited = true;
    }
}

// @brief 获取文件名(不含目录与后缀)
static void elf_module_name_get(const char* p_path, char* p_name, uint32_t name_size)
{
    const char* p_base = p_path;
    uint32_t len = 0;
    for (const char* p = p_path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            p_base = p + 1;
        }
    }
    while (p_base[len] && p_base[len] != '.' && len + 1 < name_size) {
        p_name[len] = p_base[len];
        len++;
    }
    p_name[len] = 0;
}

// @brief 替换路径最后的后缀, 例如 a/b.edl -> a/b.d
static bool elf_module_path_set(const char* p_path, const char* p_suffix, char* p_out, uint32_t out_size)
{
    const char* p_dot = NULL;
    const char* p = p_path;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            p_dot = NULL;
        } else if (*p == '.') {
            p_dot = p;
        }
        p++;
    }
    uint32_t base_len = p_dot ? (uint32_t)(p_dot - p_path) : (uint32_t)(p - p_path);
    uint32_t suffix_len = htd_strlen(p_suffix);
    if (base_len + suffix_len + 1 > out_size) {
        return false;
    }
    htd_memcpy(p_out, p_path, base_len);
    htd_memcpy(&p_out[base_len], p_suffix, suffix_len);
    p_out[base_len + suffix_len] = 0;
    return true;
}

// @brief 判断文件名后缀, 不区分大小写
static bool elf_module_suffix_is(const char* p_name, const char* p_suffix)
{
    uint32_t name_len = htd_strlen(p_name);
    uint32_t suffix_len = htd_strlen(p_suffix);
    if (name_len < suffix_len) {
        return false;
    }
    for (uint32_t i = 0; i < suffix_len; ++i) {
        char ch = p_name[name_len - suffix_len + i];
        if (ch >= 'A' && ch <= 'Z') {
            ch += 'a' - 'A';
        }
        if (ch != p_suffix[i]) {
            return false;
        }
    }
    return true;
}

// @brief 查找模块节点
static elfdl_module_node_t* elf_module_find(const char* p_name)
{
    htd_list_node_t* p_pos;
    elfdl_module_node_t* p_node;
    elf_module_list_init();
    htd_list_for_each(p_pos, &m_module_list) {
        p_node = htd_list_entry(p_pos, elfdl_module_node_t, node);
        if (htd_strcmp(p_node->name, p_name) == 0) {
            return p_node;
        }
    }
    return NULL;
}

// @brief 卸载一个节点
static void elf_module_node_unload(elfdl_module_node_t* p_node)
{
    htd_list_del(&p_node->node);
    htd_elfdl_module_uninstall(&p_node->m_module);
    htd_free(p_node);
}

// @brief 解析文件
void elf_file_parser(const char* p_filepath)
{
    uint8_t* p_edata = file_load_to_memory(p_filepath, NULL);
    if (p_edata == NULL) {
        return;
    }

    elf_head_print(p_edata);
    elf_shdr_print(p_edata);
    elf_program_hdr_print(p_edata);
    
    elf_section_dynamic_print(p_edata);
    elf_section_symtab_print(p_edata);
    elf_section_rel_dyn_print(p_edata);
    
    htd_free(p_edata);
}

// @brief 安装模块并登记到模块表
int32_t elf_install_handle(const char* p_filepath)
{
    char program_path[ELFDL_PATH_MAX_LEN + 32];
    char install_path[ELFDL_PATH_MAX_LEN + 32];
    char name[ELFDL_PATH_MAX_LEN];

    if (elf_module_path_set(p_filepath, ".so.1", program_path, sizeof(program_path)) == false ||
        elf_module_path_set(p_filepath, ".d", install_path, sizeof(install_path)) == false) {
        print_error("path too long. filepath=%s", p_filepath);
        return ERR_ELFDL_FAILED;
    }
    elf_module_name_get(p_filepath, name, sizeof(name));
    if (name[0] == 0) {
        print_error("invalid module name. filepath=%s", p_filepath);
        return ERR_ELFDL_INVALID_FILE;
    }

    // 同名模块先卸载, 避免重复占用程序空间
    elfdl_module_node_t* p_old = elf_module_find(name);
    if (p_old) {
        elf_module_node_unload(p_old);
    }

    elfdl_module_node_t* p_node = (elfdl_module_node_t*)htd_malloc(sizeof(elfdl_module_node_t));
    ASSERT(p_node);
    htd_memset(p_node, 0, sizeof(elfdl_module_node_t));
    htd_list_head_init(&p_node->node);
    htd_strncpy(p_node->name, name, sizeof(p_node->name) - 1);

    int32_t ret = htd_elfdl_module_install(&p_node->m_module, p_filepath, program_path, install_path);
    if (ret != 0) {
        print_error("install failed. ret=%d, filepath=%s", ret, p_filepath);
        htd_free(p_node);
        return ret;
    }
    htd_list_insert_after(&p_node->node, &m_module_list);
    htd_print("install %s ok: symtab_cnt=%d\r\n", p_node->name, p_node->m_module.symtab_cnt);
    return 0;
}

// @brief 使用安装文件重新安装
void elf_reinstall_handle(const char* p_filepath)
{
    elfdl_module_t module;
    int32_t ret = htd_elfdl_module_check(p_filepath, &module);
    if (ret != 0) {
        print_warning("no need reinstall. ret=%d, filepath=%s", ret, p_filepath);
        return;
    }
    ret = htd_elfdl_module_reinstall(&module, p_filepath);
    htd_print("reinstall ret=%d\r\n", ret);
    if (ret == 0) {
        htd_elfdl_module_info_show(&module);
    }
    htd_elfdl_module_symtab_free(&module);
}

// @brief 卸载模块
void elf_unload_handle(const char* p_name)
{
    elfdl_module_node_t* p_node = elf_module_find(p_name);
    if (p_node == NULL) {
        htd_print("module not found: %s\r\n", p_name);
        return;
    }
    elf_module_node_unload(p_node);
    htd_print("unload %s ok\r\n", p_name);
}

// @brief 显示已安装的模块
void elf_module_list_show(void)
{
    htd_list_node_t* p_pos;
    elfdl_module_node_t* p_node;
    elf_module_list_init();
    if (htd_list_is_empty(&m_module_list)) {
        htd_print("no module installed\r\n");
        return;
    }
    htd_list_for_each(p_pos, &m_module_list) {
        p_node = htd_list_entry(p_pos, elfdl_module_node_t, node);
        htd_print("%-12s symtab_cnt=%d, init=%p, deinit=%p, program=%s\r\n", p_node->name,
                p_node->m_module.symtab_cnt, p_node->m_module.init_fun, 
                p_node->m_module.deinit_fun, p_node->m_module.program_path);
    }
}

// @brief 扫描目录, 逐个安装 .mo 和 so
int32_t elf_load_dir_handle(const char* p_dir)
{
    char path[ELFDL_PATH_MAX_LEN + 36];
    uint32_t ok = 0, failed = 0, skipped = 0;
    htd_vfs_dirent_t* p_ent;

    DIR* p_dir_hd = htd_vfs_dir_open(p_dir);
    if (p_dir_hd == NULL) {
        print_error("open dir failed. dir=%s", p_dir);
        return ERR_ELFDL_NOT_FOUND;
    }
    while ((p_ent = htd_vfs_dir_read(p_dir_hd)) != NULL) {
        if (htd_strcmp(p_ent->d_name, ".") == 0 || htd_strcmp(p_ent->d_name, "..") == 0) {
            continue;
        }
        if (p_ent->d_type == DT_DIR) {
            continue;
        }
        if (elf_module_suffix_is(p_ent->d_name, ELFDL_INSTALL_MODULE_SUFFIX) == false) {
            skipped++;
            continue;
        }
        if (htd_snprintf(path, sizeof(path), "%s/%s", p_dir, p_ent->d_name) >= (int32_t)sizeof(path)) {
            skipped++;
            continue;
        }
        if (elf_install_handle(path) == 0) {
            ok++;
        } else {
            failed++;
        }
    }
    htd_vfs_dir_close(p_dir_hd);
    htd_print("load dir=%s: ok=%d, failed=%d, skipped=%d\r\n", p_dir, ok, failed, skipped);
    return (failed == 0) ? 0 : ERR_ELFDL_FAILED;
}

void program_test_handle(void)
{
    uint16_t upper, lower;
    uint32_t addr_val = 0x12345678;
    uint32_t *where = &addr_val; 
    
    upper = *(uint16_t*)where;
    lower = *(uint16_t*)((uint32_t)where + 2);
    print_debug("upper=%04x, lower=%04x", upper, lower);

    upper = addr_val & 0xffff;
    lower = (addr_val >> 16) & 0xffff;
    print_debug("upper=%04x, lower=%04x", upper, lower);

    *(uint16_t*)where = 0xab12;
    *(uint16_t*)((uint32_t)where + 2) = 0xcd34;
    print_debug("addr_val=%08x", addr_val);

    addr_val = ((lower & 0xffff) << 16) | (upper & 0xffff);
    print_debug("addr_val=%08x", addr_val);
}

// @brief 
int elf_cmd_handle(int argc, void** argv)
{
    if (argc < 2) {
        print_error("Usage: %s parse/install/reinstall/load/list/unload/test/show", argv[0]);
        return 0;
    }

    // 解析
    if (htd_memcmp(argv[1], "parse", 6) == 0) {
        if (argc != 3) {
            print_warning("Usage: %s parse xxxx.elf", argv[0]);
            return 0;
        }
        elf_file_parser(argv[2]);
        return 0;
    } 

    // 安装
    if (htd_memcmp(argv[1], "install", 8) == 0) {
        if (argc != 3) {
            print_warning("Usage: %s install xxxx.edl", argv[0]);
            return 0;
        }
        elf_install_handle(argv[2]);
        return 0;
    } 

    // 扫描目录, 批量安装
    if (htd_memcmp(argv[1], "load", 5) == 0) {
        if (argc != 3) {
            print_warning("Usage: %s load /mod", argv[0]);
            return 0;
        }
        elf_load_dir_handle(argv[2]);
        return 0;
    }

    // 已安装模块列表
    if (htd_memcmp(argv[1], "list", 5) == 0) {
        elf_module_list_show();
        return 0;
    }

    // 卸载
    if (htd_memcmp(argv[1], "unload", 7) == 0) {
        if (argc != 3) {
            print_warning("Usage: %s unload helloworld", argv[0]);
            return 0;
        }
        elf_unload_handle(argv[2]);
        return 0;
    }

    // 重新安装
    if (htd_memcmp(argv[1], "reinstall", 10) == 0) {
        if (argc != 3) {
            print_warning("Usage: %s reinstall xxxx.d", argv[0]);
            return 0;
        }
        elf_reinstall_handle(argv[2]);
        return 0;
    }

    // 调试
    if (htd_memcmp(argv[1], "test", 5) == 0) {
        program_test_handle();
        return 0;
    } 

    // 显示
    if (htd_memcmp(argv[1], "show", 5) == 0) {
        htd_elfdl_symbol_print();
        return 0;
    }

    print_warning("Usage: %s parse/install/reinstall/load/list/unload/test/show", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(elf_cmd_handle, elf);
#endif /**/



