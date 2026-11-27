// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_ELFDL_PROCESS_H_
#define _HTD_ELFDL_PROCESS_H_

#include "htd_elfdl_port.h"
#include "htd_elfdl_dev.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 模块后缀
#define ELFDL_INSTALL_MODULE_SUFFIX     ".mo"
// 动态库后缀
#define ELFDL_INSTALL_SO_SUFFIX         ".so"
// 文件路径
#define ELFDL_PATH_MAX_LEN              64
// 函数最长字节
#define ELFDL_SYMBOL_NAME_MAX_LEN       1024


#ifdef ELFDL_EXPORT
    #undef ELFDL_EXPORT
#endif


typedef struct {
    void *addr;
    const char *name;
} elf_symtab_t;
#define ELFDL_SECTION                   htd_elfdl
#define ELFDL_EXPORT(fun)               \
        HTD_SECTION_ITEM_REGISTER(ELFDL_SECTION, static const elf_symtab_t CONCAT_2(elfdl_, fun)) = {       \
            .addr = (void*)&fun,                                                                \
            .name = STR(fun),                                                                   \
        };





//////////////////////////////////////////////////////////////////////
typedef struct {
    uint8_t is_relative;        // symbol是否需要重新关联
    elf_relocate_t rel;
    elf_addr_t symbol;
    elf_addr_t where;
    elf_addr_t where_val;
} symlink_info_t;

// 安装结构体
typedef struct {
    uint32_t checkval;
    char program_path[ELFDL_PATH_MAX_LEN];
    char install_path[ELFDL_PATH_MAX_LEN];

    void* init_fun;
    void* deinit_fun;
    uint8_t* p_entry_addr;      // 程序入口
    
    elf_symtab_t* p_symtab;     // 连接表
    uint32_t symtab_cnt;        // 连接表条目个数
    uint32_t magic;
} elfdl_module_t;
#define ELFDL_INSTALL_MAGIC_VAL           0x8765abcd






//////////////////////////////////////////////////////////////////////
typedef uint32_t (*elfdl_fun_find_t)(const char *sym_str);
// @brief 注册标签查找函数
int32_t htd_elfdl_symbol_find_handle_register(elfdl_fun_find_t handle);
// @brief 查找函数对应的地址
elf_addr_t htd_elfdl_symboll_find(const char *sym_str);
// @brief 获取symtabl 身份识别
uint32_t htd_elfdl_section_identify_get(void);
// @brief 打印symbol
void htd_elfdl_symbol_print(void);


// @brief 安装某个程序文件, p_infile 是文件，需要通过文件读写接口读取数据
int32_t htd_elfdl_module_install(elfdl_module_t* p_module, const char* p_infilepath, const char* p_program_path, const char* p_install_path);
// @brief 使用安装文件，重新安装这个程序
int32_t htd_elfdl_module_reinstall(elfdl_module_t* p_module, const char* p_install_path);
// @brief 销毁 symtab 消息
void htd_elfdl_module_symtab_free(elfdl_module_t* p_module);
// @brief 卸载
void htd_elfdl_module_uninstall(elfdl_module_t* p_module);
// @brief 判断安装文件是否正确; 返回负数为出错误，返回0为必须重新安装，返回大于0为正常
int32_t htd_elfdl_module_check(const char* p_install_path, elfdl_module_t* p_install_info);
// @brief 打印安装信息
void htd_elfdl_module_info_show(elfdl_module_t* p_module);
// @brief 运行模块
void htd_elfdl_module_run(elfdl_module_t* p_module, int argc, char** argv);

// @brief 地址重定向
elf_addr_t htd_elfdl_addr_relocate(symlink_info_t* p_relink);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_PROGRAM_PROCESS_H_*/

