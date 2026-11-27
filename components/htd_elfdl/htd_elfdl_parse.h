// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_ELFDL_PARSE_H_
#define _HTD_ELFDL_PARSE_H_

#include "htd_elfdl_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 获取动态模块名字
const char* elf_dynamic_tag_name_get(uint32_t tag);
// @brief 判断是否为elf文件
bool elf_file_head_test(const elf_hdr_t* p_hdr);
// @brief 查找相关的节
const elf_shdr_t* elf_section_find(const char* p_elf_data, const char* p_section_name);
// @brief 获取 symtab 的字符表 strtab
const elf_shdr_t* elf_shdr_symtab_strtab_get(const elf_shdr_t* p_symtab, const char* p_elf_data);


// @brief 打印elf头部
uint32_t elf_head_print(const char* p_elf_data);
// @brief 打印section头部信息
void elf_shdr_print(const char* p_elf_data);
// @brief 打印程序头部信息
void elf_program_hdr_print(const char* p_elf_data);
// @brief 打印 symtab
void elf_section_symtab_print(const char* p_elf_data);
// @brief 打印 dynamic 节
void elf_section_dynamic_print(const char* p_elf_data);
// @brief 打印 .rel.dyn 节信息
void elf_section_rel_dyn_print(const char* p_elf_data);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_ELFDL_PARSE_H_*/

