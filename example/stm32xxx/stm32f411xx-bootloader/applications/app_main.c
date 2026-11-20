#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "htd_device.h"
#include "htd_components.h"
#include "app_board.h"




//////////////////////////////////////////////////////////////////////
// @brief 模拟休眠函数
uint32_t app_sleep_func(uint32_t sleep_ms)
{
    return 0;
}

// @brief 
void stm32_error_handler(const char *s, int num)
{
    while (1) {
        print_debug("error: file=%s, num=%d", s, num);
        htd_delay_ms(1000);
    }
}

void led_blink_loop_test(void)
{
    htd_pin_output_pp_set(PIN_NUM_GET(C, 13));
    uint8_t led_val = 0;
    htd_pin_write(PIN_NUM_GET(C, 13), led_val);
    while (1) {
        led_val = led_val ? 0 : 1;
        htd_pin_write(PIN_NUM_GET(C, 13), led_val);
        //HAL_Delay(1000);
        htd_delay_ms(1000);
    }
}

// @brief 重置bootloader环境
static void app_bootloader_jumpto(uint32_t app_addr)
{
    uint32_t app_msp = *(volatile uint32_t*)app_addr; 
    uint32_t jump_addr = *(volatile uint32_t *)(app_addr + 4);
    
    // 检查栈顶地址是否合法
    if ((app_msp & 0xFF000000) != 0x20000000) {
        print_error("msp failed. app_msp=0x%08x", app_msp);
        return;
    }
    // 检测 Thub 位, 最低bit必须为1
    if ((jump_addr & 1) == 0) {
        print_error("Thub failed. jump_addr=0x%08x", jump_addr);
        return;
    }
    print_debug("app_msp=0x%08X, reset_handler=0x%08X", app_msp, jump_addr);
    htd_delay_ms(100);

    // 关闭全局中断，避免跳转过程中被中断打断 
    __disable_irq();

#if 0
    // 复位所有外设
    RCC->AHB1RSTR = 0xFFFFFFFF;
    RCC->APB1RSTR = 0xFFFFFFFF;
    RCC->APB2RSTR = 0xFFFFFFFF;
    RCC->AHB1RSTR = 0;
    RCC->APB1RSTR = 0;
    RCC->APB2RSTR = 0;
#endif
#if 1
    // 关闭SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
#endif
#if 1
    // 关闭所有外设中断
    for (int i = 0; i < ARRAY_COUNT(NVIC->ICER); i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;  // 禁用中断
        NVIC->ICPR[i] = 0xFFFFFFFF;  // 清除挂起标志
    }
#endif
    // 时钟必须重置
    HAL_RCC_DeInit();
    // 跳转到对应程序
    typedef void (*jump_func)(void);
    jump_func jump_app = (jump_func)(jump_addr);
    __set_MSP(app_msp);
    jump_app();
    // 不会到这里
    while (1);
}

// @brief 主函数
int main(void)
{
    // 初始化板子
    app_board_init();
    print_debug("----main---%u", htd_rcc_freq_hz_get());

#if 1
    uint32_t program_addr = 0;
    if (htd_boot_process(&program_addr)) {
        print_debug("program_addr=0x%08x", program_addr);
        // 设置栈指针和清理平台部分配置
        app_bootloader_jumpto(program_addr);
    } 
#endif

    uint32_t err_code;
    while (1) {
        err_code = htd_boot_download_process();
        if (0 == err_code) {
            continue;
        } else if (1 == err_code) {
            htd_pm_reboot();
        } else {
            break;
        }
    }

    // 进入休眠
    while (1) {
        htd_delay_ms(1000);
    }
    return 0;
}


