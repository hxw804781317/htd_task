// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG   "elfdl"
#include "htd_elfdl_process.h"
#include "htd_elfdl_parse.h"


//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(ELFDL_SECTION, elf_symtab_t);
typedef struct {
    uint32_t elfsize;           // 文件大小
    elf_addr_t npaddr;          // 新程序地址
    elf_addr_t paddr;           // 程序地址
    elf_addr_t vsaddr;          // 虚拟起始地址
    //elf_addr_t veaddr;          // 虚拟结束地址
}elfdl_logic_t;
elfdl_fun_find_t m_symbol_find_handle = NULL;


// @brief 注册标签查找函数
int32_t htd_elfdl_symbol_find_handle_register(elfdl_fun_find_t handle)
{
    m_symbol_find_handle = handle;
    return 0;
}

// @brief 查找函数对应的地址
elf_addr_t htd_elfdl_symboll_find(const char *sym_str)
{
    // 先从静态表中查找
    const uint32_t *p_start = (const uint32_t*)HTD_SECTION_START_ADDR(ELFDL_SECTION);
    const uint32_t *p_end = (const uint32_t*)HTD_SECTION_END_ADDR(ELFDL_SECTION);
    const elf_symtab_t* p_dst = (const elf_symtab_t*)htd_section_first_item_get(p_start, p_end);
    while (p_dst) {
        if (htd_strcmp(p_dst->name, sym_str) == 0) {
            return (elf_addr_t)p_dst->addr;
        }
        p_dst = (const elf_symtab_t*)htd_section_next_item_get((const uint32_t*)p_dst, p_end, sizeof(elf_symtab_t));
    }
    if (m_symbol_find_handle) {
        return m_symbol_find_handle(sym_str);
    }
    return 0;
}

// @brief 获取elfdl区块身份识别码
uint32_t htd_elfdl_section_identify_get(void)
{
    const uint32_t *p_start = (const uint32_t*)HTD_SECTION_START_ADDR(ELFDL_SECTION);
    const uint32_t *p_end = (const uint32_t*)HTD_SECTION_END_ADDR(ELFDL_SECTION);
    return htd_elfdl_crc32_get((const uint8_t*)p_start, p_end - p_start);
}

// @brief 打印symbol
void htd_elfdl_symbol_print(void)
{
    const uint32_t *p_start = (const uint32_t*)HTD_SECTION_START_ADDR(ELFDL_SECTION);
    const uint32_t *p_end = (const uint32_t*)HTD_SECTION_END_ADDR(ELFDL_SECTION);
    const elf_symtab_t* p_dst = (const elf_symtab_t*)htd_section_first_item_get(p_start, p_end);
    while (p_dst) {
        print_debug("symbol name: %s, addr=%p", p_dst->name, p_dst->addr);
        p_dst = (const elf_symtab_t*)htd_section_next_item_get((const uint32_t*)p_dst, p_end, sizeof(elf_symtab_t));
    }
}


//////////////////////////////////////////////////////////////////////
// @brief elf 文件读取
static bool elfdl_file_read(int ifd, uint32_t offset, void* p_data, int32_t data_len)
{
    int32_t ret = lseek(ifd, offset, SEEK_SET);
    if (ret != offset) {
        print_error("file lseek failed. offset=%d, ret=%d", offset, ret);
        ASSERT(0);
        return false;
    }
    ret = read(ifd, p_data, data_len);
    if (ret != data_len) {
        print_error("file read failed. ret=%d, data_len=%d", ret, data_len);
        ASSERT(0);
        return false;
    }
    return true;
}

// @brief 写入安装信息
static int32_t elfdl_install_info_write(const char* p_filepath, elfdl_module_t* p_module)
{
    int fd = open(p_filepath, O_BINARY|O_RDWR|O_CREAT);
    if (fd < 0) {
        print_error("instll file open failed. %s", p_filepath);
        return 0;
    }
    int32_t ret = write(fd, p_module, sizeof(elfdl_module_t));
    close(fd);
    return ret;
}

// @brief 读取安装信息
static int32_t elfdl_install_info_read(const char* p_filepath, elfdl_module_t* p_module)
{
    int fd = open(p_filepath, O_RDONLY|O_BINARY);
    if (fd < 0) {
        print_error("instll file open failed. %s", p_filepath);
        return 0;
    }
    int32_t ret = read(fd, p_module, sizeof(elfdl_module_t));
    close(fd);
    return ret;
}

// @brief 获取程序节的内容, 返回程序空间大小
static uint32_t elfdl_program_info_get(const elf_hdr_t* p_hdr, uint32_t* vsaddr)
{
    elf_addr_t vstart_addr = ELF_ADDR_MAX;
    elf_addr_t vend_addr = ELF_ADDR_MIN;
    elf_phdr_t *p_phdr = (elf_phdr_t*)((uint8_t*)p_hdr + p_hdr->e_phoff);
    for (uint32_t i=0; i<p_hdr->e_phnum; ++i, p_phdr++) {
        if (p_phdr->p_type != PT_LOAD) {
            continue;
        }
        print_debug("LOAD segment: i=%d, p_vaddr=0x%08x, p_memsz=0x%08x, p_filesz=0x%08x", 
                i, p_phdr->p_vaddr, p_phdr->p_memsz, p_phdr->p_filesz);
        if (p_phdr->p_memsz < p_phdr->p_filesz) {
            print_warning("invalid elf: segment=%d, p_memsz=%d, p_filesz=%d", 
                i, p_phdr->p_memsz, p_phdr->p_filesz);
            return 0;
        }
        // 计算最小的虚拟起始地址
        if (vstart_addr > p_phdr->p_vaddr) {
            vstart_addr = p_phdr->p_vaddr;
        }
        // 计算最大的虚拟起始地址
        if (vend_addr < p_phdr->p_vaddr + p_phdr->p_memsz) {
            vend_addr = p_phdr->p_vaddr + p_phdr->p_memsz;
        }
        //print_warning("p_vaddr=%08x, offset=%08x, filesz=%08x", p_phdr->p_vaddr - vstart_addr, p_phdr->p_offset, p_phdr->p_filesz);
        print_debug("p_vaddr=%08x, offset=%08x, filesz=%08x", p_phdr->p_vaddr, p_phdr->p_offset, p_phdr->p_filesz);
    }
    if (vend_addr == ELF_ADDR_MIN) {
        return 0;
    }
    
    if (vsaddr) {
        *vsaddr = vstart_addr;
    }
    print_debug("vstart_addr=%08x, vend_addr=%08x", vstart_addr, vend_addr);
    return vend_addr - vstart_addr;
}

// @brief 获取elf文件程序位置信息
static uint32_t elfdl_file_program_info_get(int ifd, elf_hdr_t *p_hdr, uint32_t* vsaddr)
{
    if (p_hdr == NULL || p_hdr->e_phnum == 0) {
        return 0;
    }

    // 读取程序头部信息
    uint32_t i = sizeof(elf_phdr_t) * p_hdr->e_phnum;
    elf_phdr_t *p_phdr = (elf_phdr_t*)htd_malloc(i);
    ASSERT(p_phdr);
    if (!elfdl_file_read(ifd, p_hdr->e_phoff, p_phdr, i)) {
        htd_free(p_phdr);
        return 0;
    }

    // 提取程序偏移位置
    elf_addr_t vstart_addr = ELF_ADDR_MAX;
    elf_addr_t vend_addr = ELF_ADDR_MIN;
    elf_phdr_t *p_start = p_phdr;
    for (i=0; i<p_hdr->e_phnum; ++i, p_phdr++) {
        if (p_phdr->p_type != PT_LOAD) {
            continue;
        }
        
        print_debug("program info: i=%d, p_vaddr=0x%08x, p_memsz=0x%08x, p_filesz=0x%08x, p_flag=%08x", 
                        i, p_phdr->p_vaddr, p_phdr->p_memsz, p_phdr->p_filesz, p_phdr->p_flags);
        if (p_phdr->p_memsz < p_phdr->p_filesz) {
            print_debug("invalid elf: segment=%d, p_memsz=%d, p_filesz=%d", i, p_phdr->p_memsz, p_phdr->p_filesz);
            htd_free(p_phdr);
            return 0;
        }
        // 计算最小的虚拟起始地址
        if (vstart_addr > p_phdr->p_vaddr) {
            vstart_addr = p_phdr->p_vaddr;
        }
        // 计算最大的虚拟起始地址
        if (vend_addr < p_phdr->p_vaddr + p_phdr->p_memsz) {
            vend_addr = p_phdr->p_vaddr + p_phdr->p_memsz;
        }
        print_warning("p_vaddr=%08x, offset=%08x, filesz=%08x", p_phdr->p_vaddr-vstart_addr, p_phdr->p_offset, p_phdr->p_filesz);
    }
    if (vend_addr == ELF_ADDR_MIN) {
        htd_free(p_start);
        return 0;
    }
    
    if (vsaddr) {
        *vsaddr = vstart_addr;
    }
    htd_free(p_start);
    return vend_addr - vstart_addr;
}

// @brief 提取所有动态连接字段
static int32_t elfdl_dynsym_info_get(elfdl_module_t* p_module, elfdl_fd_t efd)
{
    int i, j, index, count;
    uint32_t length;

    p_module->symtab_cnt = 0;
    p_module->p_symtab = NULL;

    const uint8_t* psaddr = (const uint8_t*)elfdl_space_addr_get(efd);
    const elf_shdr_t *p_sh_dynstr = elf_section_find(psaddr, ".dynsym");
    if (p_sh_dynstr == NULL) {
        print_error("There are no section dynsym in this file.");
        return ERR_ELFDL_NOT_FOUND;
    }
    const elf_shdr_t* p_strtab = elf_shdr_symtab_strtab_get(p_sh_dynstr, psaddr);
    if (NULL == p_strtab) {
        print_error("Thear are no section strtab in this file.");
        return ERR_ELFDL_NOT_FOUND;
    }

    uint8_t st_bind;
    uint8_t st_type;
    const elf_sym_t *symtab = (const elf_sym_t*)(psaddr + p_sh_dynstr->sh_offset);
    index = p_sh_dynstr->sh_size / p_sh_dynstr->sh_entsize;
    for (count=0, i=0; i<index; ++i) {
        st_bind = ELF_ST_BIND(symtab[i].st_info);
        if (st_bind != STB_GLOBAL) {
            continue;
        }
        st_type = ELF_ST_TYPE(symtab[i].st_info);
        if (st_type != STT_FUNC && st_type != STT_OBJECT) {
            continue;
        }
        count++;
    }
    if (count == 0) {
        return ERR_ELFDL_SUCCESS;
    }

    elf_addr_t vsaddr = 0, paddr;
    elfdl_program_info_get((const elf_hdr_t*)psaddr, &vsaddr);
    paddr = (elf_addr_t)psaddr + vsaddr;
    p_module->symtab_cnt = count;
    p_module->p_symtab = (elf_symtab_t*)htd_malloc(count * sizeof(elf_symtab_t));
    ASSERT(p_module->p_symtab);
    const char* p_strtab_name = (const char*)(psaddr + p_strtab->sh_offset);
    const char* p_name;
    for (j=0, i=0; i<index; i++) {
        st_bind = ELF_ST_BIND(symtab[i].st_info);
        if (st_bind != STB_GLOBAL) {
            continue;
        }
        st_type = ELF_ST_TYPE(symtab[i].st_info);
        if (st_type != STT_FUNC && st_type != STT_OBJECT) {
            continue;
        }
        p_name = &p_strtab_name[symtab[i].st_name];
        length = htd_strlen(p_name);
        if (length == 0) {
            continue;
        }
        
        p_module->p_symtab[j].name = p_name;
        p_module->p_symtab[j].addr = (void*)(paddr + symtab[i].st_value - vsaddr);
        //print_debug("%d, count=%d, length=%d, p_name=%s", j, count, length, p_module->p_symtab[j].name);
        j++;
        if (j >= count) {
            break;
        }
    }
    return ERR_ELFDL_SUCCESS;
}

// @brief 查找 
static uint32_t elfdl_symtab_find(elf_symtab_t* p_symtab, uint32_t symtab_count, const char* p_name)
{
    for (uint32_t i=0; i<symtab_count; ++i) {
        if (htd_strcmp(p_name, p_symtab[i].name) == 0) {
            return (uint32_t)p_symtab[i].addr;
        }
    }
    return 0;
}

// @brief 从某个位置读取N长的数据写入到程序中
static bool elfdl_file_copy_to_program(int ifd, uint32_t offset, int32_t data_len, elfdl_fd_t* efd)
{
    print_debug("copy program: offset=%d, data_len", offset, data_len);

    int32_t ret = lseek(ifd, offset, SEEK_SET);
    if (ret != offset) {
        print_error("file lseek failed. offset=%d, ret=%d", offset, ret);
        ASSERT(0);
        return false;
    }
    uint8_t data[128];
    int32_t read_len = sizeof(data);
    while (data_len) {
        if (read_len > data_len) {
            read_len = data_len;
        }
        ret = read(ifd, data, read_len);
        if (ret <= 0) {
            print_error("file read failed. ret=%d", ret);
            ASSERT(0);
            return false;
        }
        elfdl_space_write(efd, data, ret);
        data_len -= ret;
    }
    return true;
}

// @brief 从某个位置读取N长的数据写入到内存
static bool elfdl_file_copy_to_ram(int ifd, uint32_t offset, int32_t data_len, uint8_t* p_ram)
{
    int32_t ret = lseek(ifd, offset, SEEK_SET);
    if (ret != offset) {
        print_error("file lseek failed. offset=%d, ret=%d", offset, ret);
        ASSERT(0);
        return false;
    }

    uint8_t data[128];
    int32_t read_len = sizeof(data);
    while (data_len) {
        if (read_len > data_len) {
            read_len = data_len;
        }
        ret = read(ifd, data, read_len);
        if (ret <= 0) {
            print_error("file read failed. ret=%d", ret);
            ASSERT(0);
            return false;
        }
        htd_memcpy(p_ram, data, ret);
        p_ram += ret;
        data_len -= ret;
    }
    return true;
}

// @brief 读取 elf 文件字符串
static uint32_t elfdl_file_string_read(int ifd, uint32_t offset, char* p_data, int32_t data_len)
{
    int32_t ret = lseek(ifd, offset, SEEK_SET);
    if (ret != offset) {
        print_error("file lseek failed. offset=%d, ret=%d", offset, ret);
        return 0;
    }

    bool find_end = false;
    int32_t i;
    char* p_start = p_data;
    char cache[128];
    while (data_len > 0) {
        ret = read(ifd, cache, sizeof(cache));
        if (ret <= 0) {
            return 0;
        }
        for (i=0; i<ret; ++i) {
            if (cache[i] == 0) {
                find_end = true;
                break;
            }
            *p_data++ = cache[i];
            data_len--;
            if (data_len == 1) {
                break;
            }
        }
    }
    
    if (!find_end) {
        print_error("file string read failed.");
        ASSERT(0);
    }
    *p_data = 0;
    return p_data - p_start;
}

// @brief 重置symbol地址
static void elfdl_symbol_addr_relocate_in_ram(elfdl_logic_t* p_logic, elf_hdr_t *p_hdr, elf_addr_t prev_paddr, elf_addr_t new_paddr)
{
    const elf_sym_t *symtab;
    const elf_relocate_t *p_relocate;
    const char *p_strtab;
    uint32_t i, index;
    
    // 统计多个需要重新连接
    symlink_info_t symlink;
    elf_addr_t vaddr;
    const elf_shdr_t *p_shdr = (const elf_shdr_t*)((uint32_t)p_hdr + p_hdr->e_shoff);
    for (index=0; index<p_hdr->e_shnum; index++) {
        if (p_shdr[index].sh_type != SHT_REL) {
            continue;
        }
        // 
        p_relocate = (const elf_relocate_t*)((uint32_t)p_hdr + p_shdr[index].sh_offset);
        i = p_shdr[index].sh_link;
        symtab = (const elf_sym_t*)((uint32_t)p_hdr + p_shdr[i].sh_offset);
        i = p_shdr[i].sh_link;
        p_strtab = (const char*)((uint32_t)p_hdr + p_shdr[i].sh_offset);
        
        // 多少个
        i = (uint32_t)(p_shdr[index].sh_size / sizeof(elf_relocate_t));
        while (i--) {
            const elf_sym_t *sym = &symtab[ELF_R_SYM(p_relocate->r_info)];
            if ((sym->st_shndx != SHT_NULL) || (sym->st_info >> 4) == STB_LOCAL) {
                symlink.symbol = (uint32_t)(new_paddr + sym->st_value - p_logic->vsaddr);
            } else {
                // 根据 p_strtab + sym->st_name 查找对应的地址
                symlink.symbol = htd_elfdl_symboll_find(p_strtab + sym->st_name);
                print_debug("symbol find: %08x, %s", symlink.symbol, p_strtab + sym->st_name);
            }
            
            symlink.rel = *p_relocate;
            vaddr = p_relocate->r_offset - p_logic->vsaddr;
            // 获取旧地址上的值
            symlink.where = prev_paddr + vaddr;
            symlink.where_val = *(elf_addr_t*)symlink.where;
            // 计算新的位置和新的位置的值
            symlink.where = new_paddr + vaddr;
            symlink.where_val = htd_elfdl_addr_relocate(&symlink);
            // 程序对应的位置填充新的值
            symlink.where = prev_paddr + vaddr;
            *(elf_addr_t*)symlink.where = symlink.where_val;
            // 
            p_relocate++;
        }
    }
}

// @brief 获取重连地址, npaddr 新的程序地址
static symlink_info_t* elfdl_relocate_info_get(const elf_hdr_t *p_hdr, elfdl_logic_t* p_logic, uint32_t* relink_count)
{
    // 统计多个需要重新连接
    uint32_t relink_cnt = 0;
    uint32_t index;
    const elf_shdr_t *p_shdr = (const elf_shdr_t*)((uint32_t)p_hdr + p_hdr->e_shoff);
    for (index=0; index<p_hdr->e_shnum; ++index) {
        if (p_shdr[index].sh_type != SHT_REL) {
            continue;
        }
        relink_cnt += (uint32_t)(p_shdr[index].sh_size / p_shdr[index].sh_entsize);
    }
    if (relink_cnt == 0) {
        return NULL;
    }
    
    symlink_info_t *p_relink = (symlink_info_t*)htd_malloc(relink_cnt * sizeof(symlink_info_t));
    ASSERT(p_relink);
    relink_cnt = 0;

    const elf_sym_t *symtab;
    const elf_relocate_t *p_relocate;
    const char *p_strtab;
    uint32_t i;
    for (index=0; index<p_hdr->e_shnum; index++) {
        if (p_shdr[index].sh_type != SHT_REL) {
            continue;
        }
        
        p_relocate = (const elf_relocate_t*)((uint32_t)p_hdr + p_shdr[index].sh_offset);
        i = p_shdr[index].sh_link;
        symtab = (const elf_sym_t*)((uint32_t)p_hdr + p_shdr[i].sh_offset);
        i = p_shdr[i].sh_link;
        p_strtab = (const char*)((uint32_t)p_hdr + p_shdr[i].sh_offset);
        
        i = (uint32_t)(p_shdr[index].sh_size / sizeof(elf_relocate_t));
        while (i--) {
            const elf_sym_t *sym = &symtab[ELF_R_SYM(p_relocate->r_info)];
            if ((sym->st_shndx != SHT_NULL) || (sym->st_info >> 4) == STB_LOCAL) {
                p_relink[relink_cnt].symbol = (uint32_t)(sym->st_value - p_logic->vsaddr);
                p_relink[relink_cnt].is_relative = true;
            } else {
                // 根据 p_strtab + sym->st_name 查找对应的地址
                p_relink[relink_cnt].symbol = htd_elfdl_symboll_find(p_strtab + sym->st_name);
                p_relink[relink_cnt].is_relative = false;
                print_debug("symbol find: %08x, %s", p_relink[relink_cnt].symbol, p_strtab + sym->st_name);
            } 
            p_relink[relink_cnt].rel = *p_relocate;
            // 旧地址和旧地址上的值
            p_relink[relink_cnt].where = p_logic->paddr + p_relocate->r_offset - p_logic->vsaddr;
            p_relink[relink_cnt].where_val = *(elf_addr_t*)p_relink[relink_cnt].where;
            p_relink[relink_cnt].where = p_relocate[i].r_offset - p_logic->vsaddr;
            p_relocate++;
        } 
    }
    
    if (relink_count) {
        *relink_count = relink_cnt;
    }
    return p_relink;
}

// @brief 获取重连地址
static symlink_info_t* elfdl_file_relocate_info_get(int ifd, elf_hdr_t *p_hdr, elfdl_logic_t* p_logic, uint32_t* relink_count)
{
    // 读取所有section头部信息
    uint32_t i = p_hdr->e_shnum * sizeof(elf_shdr_t);
    elf_shdr_t *p_shdr = (elf_shdr_t*)htd_malloc(i);
    ASSERT(p_shdr);
    if (!elfdl_file_read(ifd, p_hdr->e_shoff, p_shdr, i)) {
        print_error("read elf file failed. %d", i);
        htd_free(p_shdr);
        return NULL;
    }
    
    // 统计多个需要重新连接
    uint32_t relink_cnt = 0;
    uint32_t index;
    for (index=0; index<p_hdr->e_shnum; ++index) {
        if (p_shdr[index].sh_type != SHT_REL) {
            continue;
        }
        relink_cnt += (uint32_t)(p_shdr[index].sh_size / p_shdr[index].sh_entsize);
    }
    if (relink_cnt == 0) {
        htd_free(p_shdr);
        return NULL;
    }
    
    symlink_info_t *p_relink =  (symlink_info_t*)htd_malloc(relink_cnt * sizeof(symlink_info_t));
    ASSERT(p_relink);
    relink_cnt = 0;
    
    char *p_symname = (char*)htd_malloc(ELFDL_SYMBOL_NAME_MAX_LEN);
    ASSERT(p_symname);
    
    elf_sym_t *p_symtab = NULL;
    uint32_t strtab_offset;
    for (index=0; index<p_hdr->e_shnum; index++) {
        if (p_shdr[index].sh_type != SHT_REL) {
            continue;
        }
        
        i = p_shdr[index].sh_link;
        p_symtab = (elf_sym_t*)htd_malloc(p_shdr[i].sh_size);
        ASSERT(p_symtab);
        if (!elfdl_file_read(ifd, p_shdr[i].sh_offset, p_symtab, p_shdr[i].sh_size)) {
            htd_free(p_symtab), p_symtab = NULL;
            continue;
        }
        i = p_shdr[i].sh_link;
        strtab_offset = p_shdr[i].sh_offset;
        
        // 多少个
        uint32_t relocate_cnt = p_shdr[index].sh_size / p_shdr[index].sh_entsize;
        elf_relocate_t *p_relocate = (elf_relocate_t*)htd_malloc(p_shdr[index].sh_size);
        ASSERT(p_relocate);
        if (!elfdl_file_read(ifd, p_shdr[index].sh_offset, p_relocate, p_shdr[index].sh_size)) {
            htd_free(p_symtab), p_symtab = NULL;
            htd_free(p_relocate), p_relocate = NULL;
            continue;
        }
        uint32_t symbol_val;
        elf_addr_t addr_val;
        const elf_sym_t *p_sym;
        for (i=0; i<relocate_cnt; ++i) {
            p_sym = &p_symtab[ELF_R_SYM(p_relocate[i].r_info)];
            if ((p_sym->st_shndx != SHT_NULL) || (p_sym->st_info >> 4) == STB_LOCAL) {
                symbol_val = p_sym->st_value - p_logic->vsaddr;
                p_relink[relink_cnt].is_relative = true;
                print_debug("local symbol: addr=%08x", symbol_val);
            } else {
                // 根据 p_strtab + sym->st_name 查找对应的地址
                elfdl_file_string_read(ifd, strtab_offset+p_sym->st_name, p_symname, ELFDL_SYMBOL_NAME_MAX_LEN);
                symbol_val = htd_elfdl_symboll_find(p_symname);
                p_relink[relink_cnt].is_relative = false;
                print_debug("symbol find: %08x, %s", symbol_val, p_symname);
            }
            // 读取原有地址上的值
            if (elfdl_file_read(ifd, p_relocate[i].r_offset, &addr_val, sizeof(addr_val))) {
                p_relink[relink_cnt].rel = p_relocate[i];
                p_relink[relink_cnt].symbol = symbol_val; 
                p_relink[relink_cnt].where_val = addr_val;
                p_relink[relink_cnt].where = p_relocate[i].r_offset - p_logic->vsaddr;
                relink_cnt++;
            }
        }
        htd_free(p_relocate), p_relocate = NULL;
        htd_free(p_symtab), p_symtab = NULL;
    }
    htd_free(p_symname), p_symname = NULL;
    htd_free(p_shdr);
    
    if (relink_count) {
        *relink_count = relink_cnt;
    }
    return p_relink;
}

// @brief 修改relink地址
static void elf_relink_addr_relocate(elfdl_logic_t* p_logic, symlink_info_t *p_relink, uint32_t relink_count)
{
    uint32_t i, j;
    for (i=0; i<relink_count; ++i) {
        if (p_relink[i].is_relative) {
            p_relink[i].symbol = p_logic->npaddr + p_relink[i].symbol;
        }
        
        p_relink[i].where = p_logic->npaddr + (p_relink->rel.r_offset - p_logic->vsaddr);
        p_relink[i].where_val = htd_elfdl_addr_relocate(&p_relink[i]);
        print_debug("where=%x, val=%x", p_relink[i].where, p_relink[i].where_val);
    }
    
    // relink 地址从小到大排列
    symlink_info_t temp;
    for (i=0; i<relink_count; ++i) {
        for (j=i+1; j<relink_count; ++j) {
            if (p_relink[j].where < p_relink[i].where) {
                temp = p_relink[j];
                p_relink[j] = p_relink[i];
                p_relink[i] = temp;
            }
        }
    }
}

// @brief 拷贝程序
static elfdl_fd_t elf_copy_program_data_from_ram(const uint8_t* p_ram_start, const char* p_program_path, elfdl_logic_t* p_logic, symlink_info_t *p_relink, uint32_t relink_count)
{
    elfdl_fd_t p_program = elfdl_space_create(p_program_path, p_logic->elfsize);
    if (NULL == p_program) {
        return NULL;
    }
    
    // 拷贝头部数据
    uint32_t offset = 0;
    elf_addr_t saddr = elfdl_space_addr_get(p_program);
    elfdl_space_write(p_program, p_ram_start, p_logic->vsaddr);
    offset += p_logic->vsaddr;
    
    // 修改连接地址
    p_logic->npaddr = saddr + p_logic->vsaddr;
    elf_relink_addr_relocate(p_logic, p_relink, relink_count);
    
    // 拷贝数据
    uint32_t size = p_logic->elfsize - offset;
    elf_addr_t waddr = p_logic->npaddr;
    uint32_t data_len, i;
    for (i=0; i<relink_count; ++i) {
        if (p_relink[i].where < waddr) {
            print_error("relocate info error. where=%x, waddr=%x", p_relink[i].where, waddr);
            continue;
        }
        
        data_len = p_relink[i].where - waddr;
        if (data_len > size) {
            data_len = size;
        }
        
        // 从原有数据读取 data_len 的数据，
        elfdl_space_write(p_program, p_ram_start+offset, data_len);
        waddr += data_len;
        offset += data_len;
        size -= data_len;
        if (size == 0) {
            break;
        }
        
        elfdl_space_write(p_program, (const uint8_t*)&p_relink[i].where_val, sizeof(elf_addr_t));
        waddr += sizeof(elf_addr_t);
        offset += sizeof(elf_addr_t);
        size -= sizeof(elf_addr_t);
        if (size == 0) {
            break;
        }
    }
    
    // 剩下的拷贝过去
    if (size) {
        elfdl_space_write(p_program, p_ram_start+offset, size);
    }
    return p_program;
}

// @brief 从文件拷贝程序数据
static elfdl_fd_t elf_copy_program_data_from_file(int ifd, const char* p_program_path, elfdl_logic_t* p_logic, symlink_info_t *p_relink, uint32_t relink_count)
{
    elfdl_fd_t p_program = elfdl_space_create(p_program_path, p_logic->elfsize);
    if (NULL == p_program) {
        return NULL;
    }
    
    // 拷贝头部数据
    uint32_t offset = 0;
    elf_addr_t saddr = elfdl_space_addr_get(p_program);
    elfdl_file_copy_to_program(ifd, offset, p_logic->vsaddr, p_program);
    offset += p_logic->vsaddr;

    // 修改连接地址
    p_logic->npaddr = saddr + p_logic->vsaddr;
    elf_relink_addr_relocate(p_logic, p_relink, relink_count);
    
    // 拷贝数据
    uint32_t size = p_logic->elfsize - offset;
    elf_addr_t waddr = p_logic->npaddr;
    uint32_t data_len, i;
    for (i=0; i<relink_count; ++i) {
        if (p_relink[i].where < waddr) {
            print_error("relocate info error. where=%x, waddr=%x", p_relink[i].where, waddr);
            continue;
        }
        data_len = p_relink[i].where - waddr;
        if (data_len > size) {
            data_len = size;
        }
        
        // 从原有数据读取 data_len 的数据，
        elfdl_file_copy_to_program(ifd, offset, data_len, p_program);
        waddr += data_len;
        offset += data_len;
        size -= data_len;
        if (size == 0) {
            break;
        }
        
        elfdl_space_write(p_program, (const uint8_t*)&p_relink[i].where_val, sizeof(elf_addr_t));
        waddr += sizeof(elf_addr_t);
        offset += sizeof(elf_addr_t);
        size -= sizeof(elf_addr_t);
        if (size == 0) {
            break;
        }
    }
    // 剩下的拷贝过去
    if (size) {
        elfdl_file_copy_to_program(ifd, offset, size, p_program);
    }
    return p_program;
}

// @brief 在文件中重新安装
static elfdl_fd_t elfdl_reinstall_in_file(elfdl_logic_t* p_logic, elfdl_fd_t prev_fd, const char* p_new_path)
{
    // 获取程序块在文件的偏移地址和虚拟起始地址，虚拟结束地址
    const elf_hdr_t* p_hdr = (const elf_hdr_t*)elfdl_space_addr_get(prev_fd);
    if (0 == elfdl_program_info_get(p_hdr, &p_logic->vsaddr)) {
        return NULL;
    }
    
    // 创建一个连续地址的文件
    uint32_t relink_cnt = 0;
    symlink_info_t *p_relink = elfdl_relocate_info_get(p_hdr, p_logic, &relink_cnt);
    if (p_relink == NULL) {
        return NULL;
    }
    
    // 程序安装
    elfdl_fd_t efd = elf_copy_program_data_from_ram((const uint8_t*)p_hdr, p_new_path, p_logic, p_relink, relink_cnt);
    htd_free(p_relink);
    return efd;
}

// @brief 在文件中安装
static elfdl_fd_t elfdl_install_in_file(elfdl_logic_t* p_logic, int ifd, const char* p_program_path)
{
    elf_hdr_t hdr;
    elfdl_file_read(ifd, 0, &hdr, sizeof(elf_hdr_t));
    if (elf_file_head_test(&hdr) == false) {
        print_error("elf head test failed. e_phnum=%d", hdr.e_phnum);
        print_hex("elf head", (uint8_t*)&hdr, 16);
        return NULL;
    }
    
    // 获取程序块在文件的偏移地址和虚拟起始地址，虚拟结束地址
    if (0 == elfdl_file_program_info_get(ifd, &hdr, &p_logic->vsaddr)) {
        return NULL;
    }
    
    // 创建一个连续地址的文件
    uint32_t relink_cnt = 0;
    symlink_info_t *p_relink = elfdl_file_relocate_info_get(ifd, &hdr, p_logic, &relink_cnt);
    if (p_relink == NULL) {
        return NULL;
    }
    
    // 程序安装
    elfdl_fd_t efd = elf_copy_program_data_from_file(ifd, p_program_path, p_logic, p_relink, relink_cnt);
    htd_free(p_relink);
    return efd;
}

// @brief 直接在程序空间中安装和运行(程序空间就是一块内存)
static elfdl_fd_t elfdl_install_in_dev(elfdl_logic_t* p_logic, int ifd, const char* p_program_path)
{
    elfdl_fd_t efd = elfdl_space_create(p_program_path, p_logic->elfsize);
    if (NULL == efd) {
        print_error("create program space failed.");
        return NULL;
    }
    
    // 将整个文件拷贝到目标程序中
    if (!elfdl_file_copy_to_program(ifd, 0, p_logic->elfsize, efd)) {
        elfdl_space_destroy(p_program_path);
        print_error("copy elf data to program failed.");
        return NULL;
    }
    
    // 获取程序地址
    elf_hdr_t *p_hdr = (elf_hdr_t*)elfdl_space_addr_get(efd);
    if (0 == elfdl_program_info_get(p_hdr, &p_logic->vsaddr)) {
        print_error("get program offset failed.");
        elfdl_space_destroy(p_program_path);
        return NULL;
    }
    
    p_logic->npaddr = (elf_addr_t)p_hdr + p_logic->vsaddr;
    p_logic->paddr = p_logic->npaddr;
    elfdl_symbol_addr_relocate_in_ram(p_logic, p_hdr, p_logic->paddr, p_logic->npaddr);
    return efd;
}

// @brief 从内存中安装后写到程序空间中，在程序空间中执行(程序空间是XIP)
static elfdl_fd_t elfdl_install_in_ram(elfdl_logic_t* p_logic, uint8_t* p_ram, const char* p_program_path)
{
    elf_hdr_t *p_hdr = (elf_hdr_t*)p_ram;
    if (0 == elfdl_program_info_get(p_hdr, &p_logic->vsaddr)) {
        return NULL;
    }

    elfdl_fd_t efd = elfdl_space_create(p_program_path, p_logic->elfsize);
    if (NULL == efd) {
        return NULL;
    }
    
    elf_addr_t saddr = elfdl_space_addr_get(efd);
    p_logic->npaddr = saddr + p_logic->vsaddr;
    p_logic->paddr = (elf_addr_t)p_hdr + p_logic->vsaddr;
    elfdl_symbol_addr_relocate_in_ram(p_logic, p_hdr, p_logic->paddr, p_logic->npaddr);
    
    // 将数据全部写到 p_program_path
    elfdl_space_write(efd, p_ram, p_logic->elfsize);
    return efd;
}

// @brief 
static int32_t elfdl_install_info_make(elfdl_module_t* p_module, elfdl_fd_t efd, const char* p_program_path, const char* p_install_path)
{
    // 获取连接函数
    htd_memset(p_module, 0, sizeof(elfdl_module_t));
    int32_t ret = elfdl_dynsym_info_get(p_module, efd);
    if (ret != ERR_ELFDL_SUCCESS) {
        return ret;
    }
    htd_strncpy(p_module->program_path, p_program_path, sizeof(p_module->program_path));
    
    // 查找初始化和去初始化函数接口
    p_module->init_fun = (void*)elfdl_symtab_find(p_module->p_symtab, p_module->symtab_cnt, "elfdl_init");
    p_module->deinit_fun = (void*)elfdl_symtab_find(p_module->p_symtab, p_module->symtab_cnt, "elfdl_deinit");
    // 查找
    p_module->checkval = htd_elfdl_section_identify_get();
    p_module->magic = ELFDL_INSTALL_MAGIC_VAL;
    // 存储结果
    htd_strncpy(p_module->install_path, p_install_path, sizeof(p_module->install_path));
    ret = elfdl_install_info_write(p_install_path, p_module);
    if (ret == 0) {
        return ERR_ELFDL_FAILED;
    }
    return ERR_ELFDL_SUCCESS;
}

// @brief 创建一个新的路径
static char* elfdl_new_path_make(const char* p_filename)
{
    uint32_t filename_len = htd_strlen(p_filename);
    const char* p_point = p_filename + filename_len;
    while (p_point > p_filename) {
        if (p_point[0] == '.') {
            break;
        }
        p_point--;
    }

    char identify_str[8];
    const char* p_val = p_point + 1;
    uint32_t identify = 1;
    uint32_t identify_str_len;
    if (p_val[0] >= '1' && p_val[0] <= '9') {
        identify = atoi(p_val);
        identify++;
    } else {
        p_point = p_filename + filename_len;
    }
    identify_str_len = htd_snprintf(identify_str, sizeof(identify_str), ".%d", identify);

    filename_len = p_point - p_filename;
    char* p_new_filename = htd_malloc(filename_len + 16);
    ASSERT(p_new_filename);
    htd_memset(p_new_filename, 0, filename_len + 16);
    htd_memcpy(p_new_filename, p_filename, filename_len);
    htd_memcpy(&p_new_filename[filename_len], identify_str, identify_str_len);
    return p_new_filename;
}





//////////////////////////////////////////////////////////////////////
// @brief 模块安装, p_infile 是文件，需要通过文件读写接口读取数据
int32_t htd_elfdl_module_install(elfdl_module_t* p_module, const char* p_infilepath, const char* p_program_path, const char* p_install_path)
{
    ASSERT(p_module);
    ASSERT(p_infilepath);
    ASSERT(p_program_path);
    ASSERT(p_install_path);

    int ifd = open(p_infilepath, O_BINARY|O_RDONLY);
    if (ifd < 0) {
        print_error("file open failed. filepath=%s", p_infilepath);
        return ERR_ELFDL_INVALID_FILE;
    }
    uint32_t infile_size = lseek(ifd, 0, SEEK_END);
    lseek(ifd, 0, SEEK_SET);

    elfdl_fd_t efd = NULL;
    elfdl_logic_t logic;
    htd_memset(&logic, 0, sizeof(elfdl_logic_t));
    logic.elfsize = infile_size;
    if (elfdl_space_is_ram()) {
        // 程序空间为 ram，直接在程序空间进行安装
        efd = elfdl_install_in_dev(&logic, ifd, p_program_path);
    } else {
        uint8_t *p_ram = (uint8_t*)htd_malloc(logic.elfsize);
        if (p_ram) {
            // 程序空间非ram，但当前ram够用，优先在内存中安装
            elfdl_file_copy_to_ram(ifd, 0, logic.elfsize, p_ram);
            close(ifd), ifd = -1;
            efd = elfdl_install_in_ram(&logic, p_ram, p_program_path);
            htd_free(p_ram), p_ram = NULL;
        } else {
            // 在文件中安装
            efd = elfdl_install_in_file(&logic, ifd, p_program_path);
        }
    }
    if (ifd > 0) {
        close(ifd);
    }
    
    if (efd == NULL) {
        print_error("install failed.");
        return ERR_ELFDL_FAILED;
    }

    int32_t ret = elfdl_install_info_make(p_module, efd, p_program_path, p_install_path);
    elfdl_space_close(efd), efd = NULL;
    if (ret != ERR_ELFDL_SUCCESS) {
        elfdl_space_destroy(p_program_path);
    }
    return ret;
}

// @brief 使用安装文件，重新安装这个程序
int32_t htd_elfdl_module_reinstall(elfdl_module_t* p_module, const char* p_install_path)
{
    ASSERT(p_module);
    ASSERT(p_install_path);
    ASSERT(elfdl_space_is_ram() == false);
    
    elfdl_fd_t prev_fd = elfdl_space_open(p_module->program_path);
    if (prev_fd == NULL) {
        return ERR_ELFDL_INVALID_FILE;
    }

    char* p_new_path = elfdl_new_path_make(p_module->program_path);
    elfdl_fd_t efd = NULL;
    elfdl_logic_t logic;
    htd_memset(&logic, 0, sizeof(elfdl_logic_t));
    logic.elfsize = elfdl_space_size_get(prev_fd);
    uint8_t *p_ram = (uint8_t*)htd_malloc(logic.elfsize);
    if (p_ram) {
        // 内存够用，优先在内存中安装
        const uint8_t* p_data = (const uint8_t*)elfdl_space_addr_get(prev_fd);
        htd_memcpy(p_ram, p_data, logic.elfsize);
        elfdl_space_close(prev_fd), prev_fd = NULL;
        efd = elfdl_install_in_ram(&logic, p_ram, p_new_path);
        htd_free(p_ram), p_ram = NULL;
    } else {
        efd = elfdl_reinstall_in_file(&logic, prev_fd, p_new_path);
    }
    if (prev_fd) {
        elfdl_space_close(prev_fd);
    }
    if (efd == NULL) {
        print_error("install failed.");
        htd_free(p_new_path);
        return ERR_ELFDL_FAILED;
    }
    // 删除原先的文件
    elfdl_space_destroy(p_module->program_path);

    int32_t ret = elfdl_install_info_make(p_module, efd, p_new_path, p_install_path);
    elfdl_space_close(efd), efd = NULL;
    htd_free(p_new_path);
    if (ret != ERR_ELFDL_SUCCESS) {
        elfdl_space_destroy(p_new_path);
    }
    return ret;
}

// @brief 销毁 symtab 消息
void htd_elfdl_module_symtab_free(elfdl_module_t* p_module)
{
    if (p_module && p_module->p_symtab) {
        //for (uint32_t i=0; i<p_module->symtab_cnt; ++i) {
        //    if (p_module->p_symtab[i].name) {
        //        htd_free((void*)p_module->p_symtab[i].name);
        //    }
        //}
        htd_free(p_module->p_symtab);
        p_module->p_symtab = NULL;
        p_module->symtab_cnt = 0;
    }
}

// @brief 卸载
void htd_elfdl_module_uninstall(elfdl_module_t* p_module)
{
    if (NULL == p_module) {
        return;
    }
    htd_elfdl_module_symtab_free(p_module);

    if (htd_strlen(p_module->program_path)) {
        elfdl_space_destroy(p_module->program_path);
    }
    if (htd_strlen(p_module->install_path)) {
        unlink(p_module->install_path);
    }
}

// @brief 判断安装文件是否正确; 返回负数为出错误，返回0为必须重新安装，返回大于0为正常
int32_t htd_elfdl_module_check(const char* p_install_path, elfdl_module_t* p_install_info)
{
    ASSERT(p_install_path);
    ASSERT(p_install_info);
    
    int32_t ret = elfdl_install_info_read(p_install_path, p_install_info);
    if (ret != sizeof(elfdl_module_t)) {
        print_error("intall info read failed. ret=%d", ret);
        return ERR_ELFDL_INVALID_FILE;
    }
    if (p_install_info->magic != ELFDL_INSTALL_MAGIC_VAL) {
        print_error("install info error. magic=%08x", p_install_info->magic);
        return ERR_ELFDL_INVALID_FORMAT;
    }
    if (htd_strcmp(p_install_info->install_path, p_install_path)) {
        print_error("install path error. %s, %s", p_install_info->install_path, p_install_path);
        return ERR_ELFDL_NOT_FOUND;
    }
#if 0
    if (0 != elfdl_space_stat(p_install_info->program_path)) {
        print_error("program path error. %s", p_install_info->program_path);
        return -1;
    }
#endif
    if (p_install_info->checkval != htd_elfdl_section_identify_get()) {
        print_warning("program must reinstall.");
        return 0;
    }
    return sizeof(elfdl_module_t);
}

// @brief 打印安装信息
void htd_elfdl_module_info_show(elfdl_module_t* p_module)
{
    ASSERT(p_module);
    print_debug("program: magic=%08x, checkval=%08x", p_module->magic, p_module->checkval);
    print_debug("\tprogram_path=%s", p_module->program_path);
    print_debug("\tinstall_path=%s", p_module->install_path);
    print_debug("\tinit_fun=%p, deinit_fun=%p", p_module->init_fun, p_module->deinit_fun);
    for (uint32_t i=0; i<p_module->symtab_cnt; ++i) {
        print_debug("\t[%3d]symbol: addr=%08x, name=%s", i, p_module->p_symtab[i].addr, p_module->p_symtab[i].name);
    }
}

// @brief 运行模块
void htd_elfdl_module_run(elfdl_module_t* p_module, int argc, char** argv)
{
    typedef void (*module_handle_t)(elfdl_module_t *module);
    module_handle_t module_handle;
    typedef int (*module_main_t)(int argc, char** argv);
    module_main_t main_handle;

    ASSERT(p_module);
    if (p_module->init_fun) {
        module_handle = (module_handle_t)p_module->init_fun;
        module_handle(p_module);
    }
    
    if (p_module->p_entry_addr) {
        main_handle = p_module->p_entry_addr;
        main_handle(argc, argv);
    }
    if (p_module->deinit_fun) {
        module_handle = (module_handle_t)p_module->deinit_fun;
        module_handle(p_module);
    }
}


