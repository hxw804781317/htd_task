// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.pm"
#include "htd_device.h"
#include "htd_dev_pm.h"


////////////////////////////////////////////////////////////
// @brief 初始化
__WEAK int32_t htd_pm_init(system_clock_config_handle_t handle)
{
    print_warning("weak function: htd_pm_init");
    return 0;
}

// @brief 运行
__WEAK uint32_t htd_pm_run(uint32_t sleep_ms)
{
    htd_delay_ms(sleep_ms);
    return sleep_ms;
}

// @brief 获取 CPU 运行百分比
__WEAK uint8_t htd_pm_cpu_usage(void)
{
    print_warning("weak function: htd_pm_cpu_usage");
    return 0;
}

// @brief 获取 CPU 运行毫秒数
__WEAK uint32_t htd_pm_cpu_time_ms_get(void)
{
    print_warning("weak function: htd_pm_cpu_time_ms_get");
    return 0;
}

// @brief 重启
__WEAK void htd_pm_reboot(void)
{
    //NVIC_SystemReset();
    //while (1);
    print_warning("weak function: htd_pm_reboot");
}

// @brief 关机
__WEAK void htd_pm_shutdown(void)
{
    print_warning("weak function: htd_pm_shutdown");
}

// @brief 遍历所有回调区块
__WEAK void htd_pm_section_show(void)
{
}




////////////////////////////////////////////////////////////
#if (!defined(_MSC_VER)) && (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))
// @brief 重启
static int32_t reboot_cmd(int argc, char **argv)
{
    htd_pm_reboot();
    return 0;
}
CONSOLE_CMD_EXPORT(reboot_cmd, reboot);

// @brief 关机
static int32_t shutdown_cmd(int argc, char **argv)
{
    htd_pm_shutdown();
    return 0;
}
CONSOLE_CMD_EXPORT(shutdown_cmd, shutdown);


// @brief 显示区块信息
static int32_t pm_cmd_handle(int argc, char **argv)
{
    if (argc < 2) {
        print_error("usage: %s show", argv[0]);
        return 0;
    }
    
    if (strncmp(argv[1], "show", 5) == 0) {
        htd_pm_section_show();
    } else {
        print_error("usage: %s show", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(pm_cmd_handle, pm);
#endif /*(!defined(_MSC_VER)) && (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/


