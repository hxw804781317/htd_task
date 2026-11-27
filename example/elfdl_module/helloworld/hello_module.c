// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
/*
 * hello-world 模块 (ARM ELF 版, 由 scons --app=helloworld 编译)
 *
 * 与固件的区别:
 *   - 不带内核头文件, 只用 stdint.h, 内核接口按符号名在安装时解析
 *   - 必须导出 elfdl_init / elfdl_deinit (loader 按名字找), elfdl_entry 作为入口(mod.lds 里 ENTRY)
 *   - 不能有 bss, 不能有绝对地址(编译带 -fPIC, 链接见 mod.lds)
 */
#include <stdint.h>
#include "htd_task.h"
#include "htd_elfdl_process.h"

//typedef struct elfdl_module elfdl_module_t;

// 内核导出的打印接口, 名字要和内核里 ELFDL_EXPORT(xxx) 的符号名一致
extern int32_t htd_print(const char *fmt, ...);

// 模块自描述信息
const char g_module_name[]    = "helloworld";
const char g_module_version[] = "1.0";

// @brief 模块初始化
int32_t elfdl_init(elfdl_module_t* p_module)
{
    htd_print("hello: elfdl_init\r\n");
    return 0;
}

// @brief 模块入口
int32_t elfdl_entry(int argc, char** argv)
{
    htd_print("hello, world!\r\n");
    htd_print("argc=%d\r\n", argc);
    for (int i = 0; i < argc; ++i) {
        htd_print("argv[%d]=%s\r\n", i, argv[i]);
    }
    return 0;
}

// @brief 模块去初始化
int32_t elfdl_deinit(elfdl_module_t* p_module)
{
    htd_print("hello: elfdl_deinit\r\n");
    return 0;
}
