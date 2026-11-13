#ifndef _HTD_DEV_GPIO_H_
#define _HTD_DEV_GPIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "htd_config.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#if defined(SOC_FAMILY_STM32) || defined(SOC_FAMILY_GD32) || defined(SOC_FAMILY_PY32)
    #define PORT_A                  (0x00U)
    #define PORT_B                  (0x01U) 
    #define PORT_C                  (0x02U) 
    #define PORT_D                  (0x03U)
    #define PORT_E                  (0x04U)
    #define PORT_F                  (0x05U)
    #define PORT_G                  (0x06U)
    #define PORT_H                  (0x07U)
    #define PORT_I                  (0x08U)
    #define PORT_J                  (0x09U)
    #define PORT_K                  (0x0aU)
    #define PORT_M                  (0x0bU)
    #define PORT_N                  (0x0cU)
    #define PORT_O                  (0x0dU)
    #define PORT_P                  (0x0eU)
    #define PORT_Z                  (0x0fU)
    #define PORT_IDX_GET(Port)      CONCAT_2(PORT_, Port)
    typedef uint16_t pin_num_t;
    #define PIN_NUM_INVALID         0xffff
    #define PIN_NUM_GET(Port, Pin)  ((PORT_IDX_GET(Port)<<4) | ((Pin)&0x0f))
    
#elif defined(SOC_FAMILY_HC32)
    #define PORT_A                  (0x00U)
    #define PORT_B                  (0x01U) 
    #define PORT_C                  (0x02U) 
    #define PORT_D                  (0x03U)
    #define PORT_E                  (0x04U)
    #define PORT_H                  (0x05U)
    #define PORT_IDX_GET(Port)      CONCAT_2(PORT_, Port)
    typedef uint8_t pin_num_t;
    #define PIN_NUM_INVALID         0xff
    #define PIN_NUM_GET(Port, Pin)  ((PORT_IDX_GET(Port)<<4) | ((Pin)&0x0f))
    
#else
    typedef uint16_t pin_num_t;
    #define PIN_NUM_INVALID         0xffff
    #define PIN_NUM_GET(PORTx, PIN) (PIN)
    
#endif /*defined(SOC_FAMILY_STM32)*/
#define PIN_NUM_T_DEFINED           1



//////////////////////////////////////////////////////////////////////
// 0(输入); 1(输出); 2(复用); 3(模拟)
#define HTD_PIN_DIR_INPUT           0x00
#define HTD_PIN_DIR_OUTPUT          0x01
typedef uint8_t htd_pin_dir_t;

// 引脚中断模式
#define HTD_PIN_INT_RISING          0x00
#define HTD_PIN_INT_FALLING         0x01
#define HTD_PIN_INT_RISING_FALLING  0x02
#define HTD_PIN_INT_HIGH_LEVEL      0x03
#define HTD_PIN_INT_LOW_LEVEL       0x04
typedef uint8_t pin_int_mode_t;

// 输入拉高拉低模式
#define HTD_PIN_PULLDOWN            0x00
#define HTD_PIN_PULLUP              0x01
#define HTD_PIN_NOPULL              0x02
typedef uint8_t htd_pin_pull_t;

// 中断回调
typedef struct {
    uint32_t magic;
    pin_num_t pin_num;
    void (*int_func)(void);
}pin_int_cb_t;
#define HTD_PIN_INT_FUNC_MAGIC      0x15928741

// 注册中断回调函数
// 注意：  部分芯片 PA01 和 PB01 共用一个中断通道，如果这两个引脚都作为中断引脚，只要其中一个触发中断，两个中断函数都会被调用
#define HTD_PIN_INT_FUNC_SECTION_NAME    htd_dev_pin_int
#define HTD_PIN_INT_FUNC_REGISTER(_pin_num, func)   \
    HTD_SECTION_ITEM_REGISTER(HTD_PIN_INT_FUNC_SECTION_NAME, static const pin_int_cb_t CONCAT_2(m_pin_, func)) = {\
        .magic = HTD_PIN_INT_FUNC_MAGIC,    \
        .pin_num = _pin_num,                \
        .int_func = func,                   \
    };


// @brief 控制引脚电平
void htd_pin_write(pin_num_t pin_num, uint8_t val);
// @brief 控制电平反转
void htd_pin_toggle(pin_num_t pin_num);
// @brief 读取引脚电平(输入引脚)
uint32_t htd_pin_read(pin_num_t pin_num);


// @brief 配置为推挽输出模式
void htd_pin_output_pp_set(pin_num_t pin_num);
// @brief 配置为开漏输出模式
void htd_pin_output_od_set(pin_num_t pin_num);
// @brief 配置引脚为输入模式
void htd_pin_input_set(pin_num_t pin_num, htd_pin_pull_t pin_pull);
// @brief 设置为模拟输入
void htd_pin_input_analog_set(pin_num_t pin_num);
// @brief 配置引脚为高阻态模式(方向设为入，缓冲要断开，上下拉全关)
void htd_pin_hiz_set(pin_num_t pin_num);
// @brief 设置引脚方向
void htd_pin_dir_set(pin_num_t pin_num, htd_pin_dir_t pin_dir);
// @brief 配置为复用I2C模式
void htd_pin_afi2c_set(pin_num_t pin_num);



// @brief 配置为中断引脚
int32_t htd_pin_int_set(pin_num_t pin_num, pin_int_mode_t mode, uint8_t is_enable);
// @brief 查找中断函数
const pin_int_cb_t* htd_pin_int_func_find(pin_num_t pin_num);
// @brief 获取中断函数列表起始/结束地址
void htd_pin_int_func_addr_get(uint32_t** start_addr, uint32_t** end_addr);

// @brief 配置引脚为唤醒引脚
// @param wake_up_level 高电平唤醒还是低电平唤醒
// @param enable 是否使能唤醒功能
void htd_pin_wake_up_set(pin_num_t pin_num, uint8_t wake_up_level, bool enable);
// @brief 显示某个引脚的寄存器信息
void htd_pin_info_show(pin_num_t pin_num);





#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_GPIO_H_*/

