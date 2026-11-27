// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_ELFDL_DEFINED_H_
#define _HTD_ELFDL_DEFINED_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
/* e_ident[] magic number */
#define ELFMAG0                 0x7f           /* e_ident[EI_MAG0] */
#define ELFMAG1                 'E'            /* e_ident[EI_MAG1] */
#define ELFMAG2                 'L'            /* e_ident[EI_MAG2] */
#define ELFMAG3                 'F'            /* e_ident[EI_MAG3] */
#define ELFMAG                  "\177ELF"      /* magic */
#define SELFMAG                 4              /* size of magic */

#define EI_CLASS                4              /* file class */
#define EI_NIDENT               16             /* Size of e_ident[] */

/* e_ident[] file class */
#define ELFCLASSNONE            0              /* invalid */
#define ELFCLASS32              1              /* 32-bit objs */
#define ELFCLASS64              2              /* 64-bit objs */
#define ELFCLASSNUM             3              /* number of classes */

/* e_ident[] data encoding */
#define ELFDATANONE             0              /* invalid */
#define ELFDATA2LSB             1              /* Little-Endian */
#define ELFDATA2MSB             2              /* Big-Endian */
#define ELFDATANUM              3              /* number of data encode defines */

/* e_ident */
#define IS_ELF(ehdr)            ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                                 (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                                 (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                                 (ehdr).e_ident[EI_MAG3] == ELFMAG3)

#define ET_NONE                 0               /* No file type */
#define ET_REL                  1               /* Relocatable file */
#define ET_EXEC                 2               /* Executable file */
#define ET_DYN                  3               /* Shared object file */
#define ET_CORE                 4               /* Core file */

/* ELF Header */
typedef struct {
    uint8_t  e_ident[EI_NIDENT];                /* ELF Identification */
    uint16_t e_type;                            /* 文件类型 */
    uint16_t e_machine;                         /* 对应的芯片类型 */
    uint32_t e_version;                         /* object file version */
    uint32_t e_entry;                           /* virtual entry point */
    uint32_t e_phoff;                           /* program header table offset */
    uint32_t e_shoff;                           /* section header table offset */
    uint32_t e_flags;                           /* processor-specific flags */
    uint16_t e_ehsize;                          /* 头部大小 */
    uint16_t e_phentsize;                       /* program header entry size */
    uint16_t e_phnum;                           /* number of program header entries */
    uint16_t e_shentsize;                       /* section header entry size */
    uint16_t e_shnum;                           /* number of section header entries */
    uint16_t e_shstrndx;                        /* section header table's "section header string table" entry offset */
} elf32_hdr_t;

typedef struct  {
    uint8_t  e_ident[EI_NIDENT];                /* ELF Identification */
    uint16_t e_type;                            /* object file type */
    uint16_t e_machine;                         /* machine */
    uint32_t e_version;                         /* object file version */
    uint64_t e_entry;                           /* virtual entry point */
    uint64_t e_phoff;                           /* program header table offset */
    uint64_t e_shoff;                           /* section header table offset */
    uint32_t e_flags;                           /* processor-specific flags */
    uint16_t e_ehsize;                          /* ELF header size */
    uint16_t e_phentsize;                       /* program header entry size */
    uint16_t e_phnum;                           /* number of program header entries */
    uint16_t e_shentsize;                       /* section header entry size */
    uint16_t e_shnum;                           /* number of section header entries */
    uint16_t e_shstrndx;                        /* section header table's "section
                                                header string table" entry offset */
} elf64_hdr_t;

/* Section Header */
typedef struct {
    uint32_t sh_name;                           /* name - index into section header string table section */
    uint32_t sh_type;                           /* type */
    uint32_t sh_flags;                          /* flags */
    uint32_t sh_addr;                           /* address */
    uint32_t sh_offset;                         /* file offset */
    uint32_t sh_size;                           /* section size */
    uint32_t sh_link;                           /* section header table index link */
    uint32_t sh_info;                           /* extra information */
    uint32_t sh_addralign;                      /* address alignment */
    uint32_t sh_entsize;                        /* section entry size */
} elf32_shdr_t;

typedef struct {
    uint32_t sh_name;                           /* Section name (string tbl index) */
    uint32_t sh_type;                           /* Section type */
    uint64_t sh_flags;                          /* Section flags */
    uint64_t sh_addr;                           /* Section virtual addr at execution */
    uint64_t sh_offset;                         /* Section file offset */
    uint64_t sh_size;                           /* Section size in bytes */
    uint32_t sh_link;                           /* Link to another section */
    uint32_t sh_info;                           /* Additional section information */
    uint64_t sh_addralign;                      /* Section alignment */
    uint64_t sh_entsize;                        /* Entry size if section holds table */
} elf64_shdr_t;


/* Section names */
#define ELF_BSS                 ".bss"          /* uninitialized data */
#define ELF_DATA                ".data"         /* initialized data */
#define ELF_DEBUG               ".debug"        /* debug */
#define ELF_DYNAMIC             ".dynamic"      /* dynamic linking information */
#define ELF_DYNSTR              ".dynstr"       /* dynamic string table */
#define ELF_DYNSYM              ".dynsym"       /* dynamic symbol table */
#define ELF_FINI                ".fini"         /* termination code */
#define ELF_GOT                 ".got"          /* global offset table */
#define ELF_HASH                ".hash"         /* symbol hash table */
#define ELF_INIT                ".init"         /* initialization code */
#define ELF_REL_DATA            ".rel.data"     /* relocation data */
#define ELF_REL_FINI            ".rel.fini"     /* relocation termination code */
#define ELF_REL_INIT            ".rel.init"     /* relocation initialization code */
#define ELF_REL_DYN             ".rel.dyn"      /* relocaltion dynamic link info */
#define ELF_REL_RODATA          ".rel.rodata"   /* relocation read-only data */
#define ELF_REL_TEXT            ".rel.text"     /* relocation code */
#define ELF_RODATA              ".rodata"       /* read-only data */
#define ELF_SHSTRTAB            ".shstrtab"     /* section header string table */
#define ELF_STRTAB              ".strtab"       /* string table */
#define ELF_SYMTAB              ".symtab"       /* symbol table */
#define ELF_TEXT                ".text"         /* code */

/* Symbol Table Entry */
typedef struct {
    uint32_t st_name;                           /* name - index into string table */
    uint32_t st_value;                          /* symbol value */
    uint32_t st_size;                           /* symbol size */
    uint8_t  st_info;                           /* type and binding */
    uint8_t  st_other;                          /* 0 - no defined meaning */
    uint16_t st_shndx;                          /* section header index */
} elf32_sym_t;

typedef struct {
    uint32_t st_name;                           /* Symbol name (string tbl index) */
    uint8_t  st_info;                           /* Symbol type and binding */
    uint8_t  st_other;                          /* Symbol visibility */
    uint16_t st_shndx;                          /* Section index */
    uint64_t st_value;                          /* Symbol value */
    uint64_t st_size;                           /* Symbol size */
} elf64_sym_t;


#define STB_LOCAL               0              /* BIND */
#define STB_GLOBAL              1
#define STB_WEAK                2
#define STB_NUM                 3
#define STB_LOPROC              13             /* processor specific range */
#define STB_HIPROC              15

#define STT_NOTYPE              0              /* symbol type is unspecified */
#define STT_OBJECT              1              /* data object */
#define STT_FUNC                2              /* code object */
#define STT_SECTION             3              /* symbol identifies an ELF section */
#define STT_FILE                4              /* symbol's name is file name */
#define STT_COMMON              5              /* common data object */
#define STT_TLS                 6              /* thread-local data object */
#define STT_NUM                 7              /* # defined types in generic range */
#define STT_LOOS                10             /* OS specific range */
#define STT_HIOS                12
#define STT_LOPROC              13             /* processor specific range */
#define STT_HIPROC              15

#define STN_UNDEF               0              /* undefined */

#define ELF_ST_BIND(info)       ((info) >> 4)
#define ELF_ST_TYPE(info)       ((info) & 0xf)
#define ELF_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))

/* Relocation entry with implicit addend */
typedef struct {
    uint32_t r_offset;                          /* offset of relocation */
    uint32_t r_info;                            /* symbol table index and type */
} elf32_relocate_t;

typedef struct {
    uint64_t r_offset;                          /* Address */
    uint64_t r_info;                            /* Relocation type and symbol index */
} elf64_relocate_t;

/* Relocation entry with explicit addend */
typedef struct {
    uint32_t r_offset;                          /* offset of relocation */
    uint32_t r_info;                            /* symbol table index and type */
    int32_t  r_addend;
} elf32_rela_t;

typedef struct {
    uint64_t r_offset;                          /* Address */
    uint64_t r_info;                            /* Relocation type and symbol index */
    int64_t  r_addend;                          /* Addend */
} elf64_rela_t;


/* Extract relocation info - r_info */
#define ELF32_R_SYM(i)                          ((i) >> 8)
#define ELF32_R_TYPE(i)                         ((unsigned char) (i))
#define ELF32_R_INFO(s,t)                       (((s) << 8) + (unsigned char)(t))

#define ELF64_R_SYM(i)                          ((i) >> 32)
#define ELF64_R_TYPE(i)                         ((i) & 0xffffffff)
#define ELF64_R_INFO(sym,type)                  ((((uint64_t) (sym)) << 32) + (type))


/*
 * Relocation type for arm
 */
#define R_ARM_NONE              0
#define R_ARM_PC24              1
#define R_ARM_ABS32             2
#define R_ARM_REL32             3
#define R_ARM_THM_CALL          10
#define R_ARM_GLOB_DAT          21
#define R_ARM_JUMP_SLOT         22
#define R_ARM_RELATIVE          23
#define R_ARM_GOT_BREL          26
#define R_ARM_PLT32             27
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_THM_JUMP24        30
#define R_ARM_V4BX              40

/*
 * Relocation type for x86
 */
#define R_386_NONE              0
#define R_386_32                1
#define R_386_PC32              2
#define R_386_GOT32             3
#define R_386_PLT32             4
#define R_386_COPY              5
#define R_386_GLOB_DAT          6
#define R_386_JMP_SLOT          7
#define R_386_RELATIVE          8
#define R_386_GOTOFF            9
#define R_386_GOTPC             10

/* Program Header */
typedef struct {
    uint32_t p_type;                            /* segment type */
    uint32_t p_offset;                          /* segment offset */
    uint32_t p_vaddr;                           /* virtual address of segment */
    uint32_t p_paddr;                           /* physical address - ignored? */
    uint32_t p_filesz;                          /* number of bytes in file for seg. */
    uint32_t p_memsz;                           /* number of bytes in mem. for seg. */
    uint32_t p_flags;                           /* flags */
    uint32_t p_align;                           /* memory alignment */
} elf32_phdr_t;

typedef struct {
    uint32_t p_type;                            /* Segment type */
    uint32_t p_flags;                           /* Segment flags */
    uint64_t p_offset;                          /* Segment file offset */
    uint64_t p_vaddr;                           /* Segment virtual address */
    uint64_t p_paddr;                           /* Segment physical address */
    uint64_t p_filesz;                          /* Segment size in file */
    uint64_t p_memsz;                           /* Segment size in memory */
    uint64_t p_align;                           /* Segment alignment */
} elf64_phdr_t;


/* p_type */
#define PT_NULL                 0
#define PT_LOAD                 1
#define PT_DYNAMIC              2
#define PT_INTERP               3
#define PT_NOTE                 4
#define PT_SHLIB                5
#define PT_PHDR                 6
#define PT_TLS                  7
#define PT_NUM                  8
#define PT_LOOS                 0x60000000
#define PT_HIOS                 0x6fffffff
#define PT_LOPROC               0x70000000
#define PT_HIPROC               0x7fffffff

/* p_flags */
#define PF_X                    1
#define PF_W                    2
#define PF_R                    4

/* sh_type */
#define SHT_NULL                0              /* inactive */
#define SHT_PROGBITS            1              /* program defined information */
#define SHT_SYMTAB              2              /* symbol table section */
#define SHT_STRTAB              3              /* string table section */
#define SHT_RELA                4              /* relocation section with addends*/
#define SHT_HASH                5              /* symbol hash table section */
#define SHT_DYNAMIC             6              /* dynamic section */
#define SHT_NOTE                7              /* note section */
#define SHT_NOBITS              8              /* no space section */
#define SHT_REL                 9              /* relocation section without addends */
#define SHT_SHLIB               10             /* reserved - purpose unknown */
#define SHT_DYNSYM              11             /* dynamic symbol table section */
#define SHT_NUM                 12             /* number of section types */
#define SHT_LOPROC              0x70000000     /* reserved range for processor */
#define SHT_HIPROC              0x7fffffff     /* specific section header types */
#define SHT_LOUSER              0x80000000     /* reserved range for application */
#define SHT_HIUSER              0xffffffff     /* specific indexes */

/* Section Attribute Flags - sh_flags */
#define SHF_WRITE               0x1            /* Writable */
#define SHF_ALLOC               0x2            /* occupies memory */
#define SHF_EXECINSTR           0x4            /* executable */
#define SHF_MASKPROC            0xf0000000     /* reserved bits for processor */
/* specific section attributes */

#define IS_PROG(s)        (s.sh_type == SHT_PROGBITS)
#define IS_NOPROG(s)      (s.sh_type == SHT_NOBITS)
#define IS_REL(s)         (s.sh_type == SHT_REL)
#define IS_RELA(s)        (s.sh_type == SHT_RELA)
#define IS_ALLOC(s)       (s.sh_flags == SHF_ALLOC)
#define IS_AX(s)          ((s.sh_flags & SHF_ALLOC) && (s.sh_flags & SHF_EXECINSTR))
#define IS_AW(s)          ((s.sh_flags & SHF_ALLOC) && (s.sh_flags & SHF_WRITE))


/* Dynamic structure */
typedef struct {
    int32_t    d_tag;           /* controls meaning of d_val */
    union {
        uint32_t d_val;    /* Multiple meanings - see d_tag */
        uint32_t d_ptr;    /* program virtual address */
    } d_un;
} elf32_dyn_t;

/* Dynamic Array Tags - d_tag */
#define DT_NULL        0        /* marks end of _DYNAMIC array */
#define DT_NEEDED      1        /* string table offset of needed lib */
#define DT_PLTRELSZ    2        /* size of relocation entries in PLT */
#define DT_PLTGOT      3        /* address PLT/GOT */
#define DT_HASH        4        /* address of symbol hash table */
#define DT_STRTAB      5        /* address of string table */
#define DT_SYMTAB      6        /* address of symbol table */
#define DT_RELA        7        /* address of relocation table */
#define DT_RELASZ      8        /* size of relocation table */
#define DT_RELAENT     9        /* size of relocation entry */
#define DT_STRSZ      10        /* size of string table */
#define DT_SYMENT     11        /* size of symbol table entry */
#define DT_INIT       12        /* address of initialization func. */
#define DT_FINI       13        /* address of termination function */
#define DT_SONAME     14        /* string table offset of shared obj */
#define DT_RPATH      15        /* string table offset of library search path */
#define DT_SYMBOLIC   16        /* start sym search in shared obj. */
#define DT_REL        17        /* address of rel. tbl. w addends */
#define DT_RELSZ      18        /* size of DT_REL relocation table */
#define DT_RELENT     19        /* size of DT_REL relocation entry */
#define DT_PLTREL     20        /* PLT referenced relocation entry */
#define DT_DEBUG      21        /* bugger */
#define DT_TEXTREL    22        /* Allow rel. mod. to unwritable seg */
#define DT_JMPREL     23        /* add. of PLT's relocation entries */
#define DT_BIND_NOW   24        /* Bind now regardless of env setting */
#define DT_LOOS       0x6000000d    /* reserved range for OS */
#define DT_HIOS       0x6ffff000    /*  specific dynamic array tags */
#define DT_LOPROC     0x70000000    /* reserved range for processor */
#define DT_HIPROC     0x7fffffff    /*  specific dynamic array tags */
/* some other useful tags */
#define DT_RELACOUNT  0x6ffffff9    /* if present, number of RELATIVE */
#define DT_RELCOUNT   0x6ffffffa    /* relocs, which must come first */
#define DT_FLAGS_1    0x6ffffffb

/* Dynamic Flags - DT_FLAGS_1 .dynamic entry */
#define DF_1_NOW       0x00000001
#define DF_1_GLOBAL    0x00000002
#define DF_1_GROUP     0x00000004
#define DF_1_NODELETE  0x00000008
#define DF_1_LOADFLTR  0x00000010
#define DF_1_INITFIRST 0x00000020
#define DF_1_NOOPEN    0x00000040
#define DF_1_ORIGIN    0x00000080
#define DF_1_DIRECT    0x00000100
#define DF_1_TRANS     0x00000200
#define DF_1_INTERPOSE 0x00000400
#define DF_1_NODEFLIB  0x00000800
#define DF_1_NODUMP    0x00001000
#define DF_1_CONLFAT   0x00002000

/* ld.so: number of low tags that are used saved internally (0 .. DT_NUM-1) */
#define DT_NUM        (DT_JMPREL+1)

/*
 * Note Definitions
 */
typedef struct {
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
} elf32_note_t;




#if (__SIZEOF_POINTER__ == 8) || (defined _WIN64)
    typedef uint64_t elf_addr_t;
    typedef elf64_hdr_t elf_hdr_t;
    typedef elf64_shdr_t elf_shdr_t;
    typedef elf64_sym_t elf_sym_t;
    typedef elf64_rela_t elf_relocate_t;
    typedef elf64_rela_t elf_rela_t;
    typedef elf64_phdr_t elf_phdr_t;
    
    /* Extract relocation info - r_info */
    #define ELF_R_SYM(i)                          ((i) >> 32)
    #define ELF_R_TYPE(i)                         ((i) & 0xffffffff)
    #define ELF_R_INFO(sym,type)                  ((((uint64_t) (sym)) << 32) + (type))
    
#elif (__SIZEOF_POINTER__ == 4) || (defined _WIN32)
    typedef uint32_t elf_addr_t;
    typedef elf32_hdr_t elf_hdr_t;
    typedef elf32_shdr_t elf_shdr_t;
    typedef elf32_sym_t elf_sym_t;
    typedef elf32_relocate_t elf_relocate_t;
    typedef elf32_rela_t elf_rela_t;
    typedef elf32_phdr_t elf_phdr_t;
    
    #define ELF_R_SYM(i)                          ((i) >> 8)
    #define ELF_R_TYPE(i)                         ((unsigned char) (i))
    #define ELF_R_INFO(s,t)                       (((s) << 8) + (unsigned char)(t))
    
#else
    #error "can't support."
#endif

#define ELF_INVALID_ADDR                      (0)
#define ELF_ADDR_MAX                          ((elf_addr_t)(0-1))
#define ELF_ADDR_MIN                          ((elf_addr_t)0)







//////////////////////////////////////////////////////////////////////
#ifndef __STATIC_INLINE
    #define __STATIC_INLINE                     static inline
#endif
// 拼接两个宏
#ifndef CONCAT_2
    #define CONCAT_2_(p1, p2)                   p1##p2
    #define CONCAT_2(p1, p2)                    CONCAT_2_(p1, p2)
#endif
// 拼接三个宏
#ifndef CONCAT_3
    #define CONCAT_3_(p1, p2, p3)               p1##p2##p3
    #define CONCAT_3(p1, p2, p3)                CONCAT_3_(p1, p2, p3)
#endif
#ifndef STR
    #define STR(s)                              #s
#endif 
#ifndef STRINGIFY
    #define STRINGIFY(val)                      STR(val)
#endif 
// 数组成员个数
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)                      (sizeof(X) / sizeof(X[0]))
#endif



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_ELFDL_DEFINED_H_*/

