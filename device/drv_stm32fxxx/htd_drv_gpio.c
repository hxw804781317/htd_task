#define DEBUG_LVL   2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.gpio"
#include "htd_drv_common.h"
#include "htd_drv_gpio.h"
#include "htd_device.h"


//////////////////////////////////////////////////////////////////////
#if defined(GPIOZ)
    #define GPIO_PORT_IDX_MAX 16u
#elif defined(GPIOP)
    #define GPIO_PORT_IDX_MAX 15u
#elif defined(GPIOO)
    #define GPIO_PORT_IDX_MAX 14u
#elif defined(GPION)
    #define GPIO_PORT_IDX_MAX 13u
#elif defined(GPIOM)
    #define GPIO_PORT_IDX_MAX 12u
#elif defined(GPIOK)
    #define GPIO_PORT_IDX_MAX 11u
#elif defined(GPIOJ)
    #define GPIO_PORT_IDX_MAX 10u
#elif defined(GPIOI)
    #define GPIO_PORT_IDX_MAX 9u
#elif defined(GPIOH)
    #define GPIO_PORT_IDX_MAX 8u
#elif defined(GPIOG)
    #define GPIO_PORT_IDX_MAX 7u
#elif defined(GPIOF)
    #define GPIO_PORT_IDX_MAX 6u
#elif defined(GPIOE)
    #define GPIO_PORT_IDX_MAX 5u
#elif defined(GPIOD)
    #define GPIO_PORT_IDX_MAX 4u
#elif defined(GPIOC)
    #define GPIO_PORT_IDX_MAX 3u
#elif defined(GPIOB)
    #define GPIO_PORT_IDX_MAX 2u
#elif defined(GPIOA)
    #define GPIO_PORT_IDX_MAX 1u
#else
    #define GPIO_PORT_IDX_MAX 0u
    #error Unsupported STM32 GPIO peripheral.
#endif

#if defined(GPIOA)
    #define H_GPIOA GPIOA
#else
    #define H_GPIOA NULL
#endif
#if defined(GPIOB)
    #define H_GPIOB GPIOB
#else
    #define H_GPIOB NULL
#endif
#if defined(GPIOC)
    #define H_GPIOC GPIOC
#else
    #define H_GPIOC NULL
#endif
#if defined(GPIOD)
    #define H_GPIOD GPIOD
#else
    #define H_GPIOD NULL
#endif
#if defined(GPIOE)
    #define H_GPIOE GPIOE
#else
    #define H_GPIOE NULL
#endif
#if defined(GPIOF)
    #define H_GPIOF GPIOF
#else
    #define H_GPIOF NULL
#endif
#if defined(GPIOG)
    #define H_GPIOG GPIOG
#else
    #define H_GPIOG NULL
#endif
#if defined(GPIOH)
    #define H_GPIOH GPIOH
#else
    #define H_GPIOH NULL
#endif
#if defined(GPIOI)
    #define H_GPIOI GPIOI
#else
    #define H_GPIOI NULL
#endif
#if defined(GPIOJ)
    #define H_GPIOJ GPIOJ
#else
    #define H_GPIOJ NULL
#endif
#if defined(GPIOK)
    #define H_GPIOK GPIOK
#else
    #define H_GPIOK NULL
#endif
#if defined(GPIOM)
    #define H_GPIOM GPIOM
#else
    #define H_GPIOM NULL
#endif
#if defined(GPION)
    #define H_GPION GPION
#else
    #define H_GPION NULL
#endif
#if defined(GPIOO)
    #define H_GPIOO GPIOO
#else
    #define H_GPIOO NULL
#endif
#if defined(GPIOP)
    #define H_GPIOP GPIOP
#else
    #define H_GPIOP NULL
#endif
#if defined(GPIOZ)
    #define H_GPIOZ GPIOZ
#else
    #define H_GPIOZ NULL
#endif

const char* const m_gpio_name_table[] = {"GPIOA", "GPIOB", "GPIOC", "GPIOD", "GPIOE", "GPIOF", "GPIOG", "GPIOH", "GPIOI", "GPIOJ", "GPIOK", "GPIOM", "GPION", "GPIOO", "GPIOP", "GPIOZ"};
static GPIO_TypeDef* const GPIO_PORT_TABLE[] = {H_GPIOA, H_GPIOB, H_GPIOC, H_GPIOD, H_GPIOE, H_GPIOF, H_GPIOG, H_GPIOH, H_GPIOI, H_GPIOJ, H_GPIOK, H_GPIOM, H_GPION, H_GPIOO, H_GPIOP, H_GPIOZ};
#define PIN_PORT_IDX(pin_num)           ((uint8_t)(((pin_num) >> 4) & 0xFu))
#define PIN_IDX(pin)                    ((uint8_t)((pin)&0xFu))
#define STM_GPIO_PORT(pin_num)          GPIO_PORT_TABLE[PIN_PORT_IDX(pin_num)]
#define STM_GPIO_PIN(pin_num)           ((uint16_t)(1u << PIN_IDX(pin_num)))



/////////////////////////////////////////////////////////////////////
static const IRQn_Type m_pin_int_type_table[] = {
    EXTI0_IRQn,
    EXTI1_IRQn,
    EXTI2_IRQn,
    EXTI3_IRQn,
    EXTI4_IRQn,
    EXTI5_IRQn,
    EXTI6_IRQn,
    EXTI7_IRQn,
    EXTI8_IRQn,
    EXTI9_IRQn,
    EXTI10_IRQn,
    EXTI11_IRQn,
    EXTI12_IRQn,
    EXTI13_IRQn,
    EXTI14_IRQn,
    EXTI15_IRQn,
};
static uint32_t m_pin_int_type_mask = 0;
static uint8_t m_pin_channel_mask[16] = {0};


// @brief 控制引脚电平
void htd_pin_write(pin_num_t pin_num, uint8_t val)
{
    ASSERT(PIN_PORT_IDX(pin_num) < GPIO_PORT_IDX_MAX);
    GPIO_TypeDef *gpio_port = STM_GPIO_PORT(pin_num);
    uint32_t gpio_pin = STM_GPIO_PIN(pin_num);
    HAL_GPIO_WritePin(gpio_port, gpio_pin, val == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// @brief 控制引脚电平反转
void htd_pin_toggle(pin_num_t pin_num)
{
    ASSERT(PIN_PORT_IDX(pin_num) < GPIO_PORT_IDX_MAX);
    GPIO_TypeDef *gpio_port = STM_GPIO_PORT(pin_num);
    uint32_t gpio_pin = STM_GPIO_PIN(pin_num);
    HAL_GPIO_TogglePin(gpio_port, gpio_pin);
}

// @brief 读取引脚电平
uint32_t htd_pin_read(pin_num_t pin_num)
{
    ASSERT(PIN_PORT_IDX(pin_num) < GPIO_PORT_IDX_MAX);
    GPIO_TypeDef *gpio_port = STM_GPIO_PORT(pin_num);
    uint32_t gpio_pin = STM_GPIO_PIN(pin_num);
    GPIO_PinState state = HAL_GPIO_ReadPin(gpio_port, gpio_pin);
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

// @brief 配置为推挽输出模式
void htd_pin_output_pp_set(pin_num_t pin_num)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
}

// @brief 配置为开漏输出模式
void htd_pin_output_od_set(pin_num_t pin_num)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
}

// @brief 配置为复用I2C开漏
void htd_pin_afi2c_set(pin_num_t pin_num)
{
    // 开漏输出 + 上拉打开
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  
    HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
}

// @brief 配置为输入
void htd_pin_input_set(pin_num_t pin_num, htd_pin_pull_t pin_pull)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    if (pin_pull == HTD_PIN_NOPULL) {
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    } else if (pin_pull == HTD_PIN_PULLDOWN) {
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    } else {
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
}

// @brief 设置为模拟输入
void htd_pin_input_analog_set(pin_num_t pin_num)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
}

// @brief 配置引脚为高阻态模式(方向设为入，缓冲要断开，上下拉全关), 一般选择模拟输入作为高阻态
void htd_pin_hiz_set(pin_num_t pin_num)
{
    htd_pin_input_analog_set(pin_num);
}

// @brief 设置引脚方向
void htd_pin_dir_set(pin_num_t pin_num, htd_pin_dir_t pin_dir)
{
    GPIO_TypeDef* port = STM_GPIO_PORT(pin_num);
    uint32_t pin_bit = STM_GPIO_PIN(pin_num);
    if (pin_dir == HTD_PIN_DIR_OUTPUT) {
        LL_GPIO_SetPinMode(port, pin_bit, LL_GPIO_MODE_OUTPUT);
    } else {
        LL_GPIO_SetPinMode(port, pin_bit, LL_GPIO_MODE_INPUT);
    }
}

// @brief 配置为中断引脚
int32_t htd_pin_int_set(pin_num_t pin_num, pin_int_mode_t mode, uint8_t is_enable)
{
    uint32_t port_idx = PIN_PORT_IDX(pin_num);
    ASSERT(port_idx < GPIO_PORT_IDX_MAX);
    
    uint32_t pin_idx = PIN_IDX(pin_num);
    IRQn_Type IRQ_type = m_pin_int_type_table[pin_idx];
    uint32_t pin_bit = STM_GPIO_PIN(pin_num);
    if (is_enable) {
        // 配置引脚为中断模式
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = pin_bit;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        if (mode == HTD_PIN_INT_RISING) {
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        } else if (mode == HTD_PIN_INT_FALLING) {
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        } else {
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        }
        HAL_GPIO_Init(STM_GPIO_PORT(pin_num), &GPIO_InitStruct);
        // 配置中断优先级和使能中断
        HAL_NVIC_SetPriority(IRQ_type, 3, 10);
        HAL_NVIC_EnableIRQ(IRQ_type);
        SET_BIT(m_pin_int_type_mask, pin_idx);
        SET_BIT(m_pin_channel_mask[pin_idx], port_idx);
        return 0;
    } 

    // 关闭对应的中断
    HAL_GPIO_DeInit(STM_GPIO_PORT(pin_num), pin_bit);
    __CLR_BIT(m_pin_int_type_mask, pin_idx);
    __CLR_BIT(m_pin_channel_mask[pin_idx], port_idx);
    if ((pin_idx >= 5) && (pin_idx <= 9)) {
        if (!(m_pin_int_type_mask & (GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9))) {
            if (m_pin_channel_mask[pin_idx] == 0) {
                HAL_NVIC_DisableIRQ(IRQ_type);
            }
        }
    } else if ((pin_idx >= 10) && (pin_idx <= 15)) {
        if (!(m_pin_int_type_mask & (GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15))) {
            if (m_pin_channel_mask[pin_idx] == 0) {
                HAL_NVIC_DisableIRQ(IRQ_type);
            }
        }
    } else {
        if (m_pin_channel_mask[pin_idx] == 0) {
            HAL_NVIC_DisableIRQ(IRQ_type);
        }
    }
    return 0;
}

// @brief 显示某个引脚的寄存器信息
void htd_pin_info_show(pin_num_t pin_num)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    GPIO_TypeDef* port = STM_GPIO_PORT(pin_num);
    uint32_t pin_idx = PIN_IDX(pin_num);
    uint32_t pin_bit = STM_GPIO_PIN(pin_num);
    
    uint32_t port_idx = PIN_PORT_IDX(pin_num);
    print_debug("Pin: %s%d", m_gpio_name_table[port_idx], pin_idx);
    
    // MODE: 0(输入模式), 1(输出模式), 2(复用模式), 3(模拟模式)
    uint8_t mode = LL_GPIO_GetPinMode(port, pin_bit);
    if (mode == 1) {
        if (LL_GPIO_GetPinOutputType(port, pin_bit) == LL_GPIO_OUTPUT_PUSHPULL) {
            print_debug("Output mode: Push-Pull");
        } else {
            print_debug("Output mode: Open-Drain");
        }
        print_warning("Level: %d", LL_GPIO_IsOutputPinSet(port, pin_bit) ? 1 : 0);
        
    } else if (mode == 0) {
        uint32_t pull = LL_GPIO_GetPinPull(port, pin_bit);
        if (pull == LL_GPIO_PULL_DOWN) {
            print_debug("Input mode: Pull-Down");
        } else if (pull == LL_GPIO_PULL_UP) {
            print_debug("Input mode: Pull-Up");
        } else {
            print_debug("Input mode: Pull-No");
        }
        print_warning("Level: %d", LL_GPIO_IsInputPinSet(port, pin_bit) ? 1 : 0);
        
    } else if (mode == 2) {
    #ifdef SOC_SERIES_STM32F1
        print_debug("Af mode");
    #else
        uint32_t afval;
        if (pin_idx < 8) {
            afval = LL_GPIO_GetAFPin_0_7(port, pin_bit);
        } else {
            afval = LL_GPIO_GetAFPin_8_15(port, pin_bit);
        }
        print_debug("Af mode: 0x%08x", afval);
    #endif
    } else {
        print_debug("Anolog mode: ");
    }
#endif 
}

// @brief 初始化，使能所有引脚时钟
int drv_pin_init(void)
{
    memset(m_pin_channel_mask, 0, sizeof(m_pin_channel_mask));
#ifdef GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif
#ifdef GPIOB
    __HAL_RCC_GPIOB_CLK_ENABLE();
#endif
#ifdef GPIOC
    __HAL_RCC_GPIOC_CLK_ENABLE();
#endif
#ifdef GPIOD
    __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
    __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
#ifdef GPIOI
    __HAL_RCC_GPIOI_CLK_ENABLE();
#endif
#ifdef GPIOJ
    __HAL_RCC_GPIOJ_CLK_ENABLE();
#endif
#ifdef GPIOK
    __HAL_RCC_GPIOK_CLK_ENABLE();
#endif
#ifdef GPIOM
    __HAL_RCC_GPIOM_CLK_ENABLE();
#endif
#ifdef GPION
    __HAL_RCC_GPION_CLK_ENABLE();
#endif
#ifdef GPIOO
    __HAL_RCC_GPIOO_CLK_ENABLE();
#endif
#ifdef GPIOP
    __HAL_RCC_GPIOO_CLK_ENABLE();
#endif
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_pin_init);


// @brief 中断回调
void stm32_gpio_int_handle(uint8_t pin_idx)
{
    uint16_t GPIO_Pin = 1 << pin_idx;
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);

        uint32_t *start_addr, *end_addr;
        htd_pin_int_func_addr_get(&start_addr, &end_addr);
        
        const pin_int_cb_t* p_dev;
        const uint32_t* p_item = htd_section_first_item_get(start_addr, end_addr);
        while (p_item) {
            p_dev = (pin_int_cb_t*)p_item;
            if (p_dev->magic == HTD_PIN_INT_FUNC_MAGIC && PIN_IDX(p_dev->pin_num) == pin_idx) {
                p_dev->int_func();
            }
            p_item = htd_section_next_item_get(p_item, end_addr, sizeof(pin_int_cb_t));
        }
    }
}

void EXTI0_IRQHandler(void){stm32_gpio_int_handle(0);}
void EXTI1_IRQHandler(void){stm32_gpio_int_handle(1);}
void EXTI2_IRQHandler(void){stm32_gpio_int_handle(2);}
void EXTI3_IRQHandler(void){stm32_gpio_int_handle(3);}
void EXTI4_IRQHandler(void){stm32_gpio_int_handle(4);}
void EXTI5_IRQHandler(void){stm32_gpio_int_handle(5);}
void EXTI6_IRQHandler(void){stm32_gpio_int_handle(6);}
void EXTI7_IRQHandler(void){stm32_gpio_int_handle(7);}
void EXTI8_IRQHandler(void){stm32_gpio_int_handle(8);}
void EXTI9_IRQHandler(void){stm32_gpio_int_handle(9);}
void EXTI10_IRQHandler(void){stm32_gpio_int_handle(10);}
void EXTI11_IRQHandler(void){stm32_gpio_int_handle(11);}
void EXTI12_IRQHandler(void){stm32_gpio_int_handle(12);}
void EXTI13_IRQHandler(void){stm32_gpio_int_handle(13);}
void EXTI14_IRQHandler(void){stm32_gpio_int_handle(14);}
void EXTI15_IRQHandler(void){stm32_gpio_int_handle(15);}








//////////////////////////////////////////////////////////////////////
// @brief 将 PA13/PA14复用为SWD引脚
void drv_pin_af_swd(void)
{
#if defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F0)
    
#else
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_SWJ;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif
}

// @brief 引脚复用为 I2C
int32_t drv_pin_af_i2c(uint32_t af_i2c, pin_num_t pin_scl, pin_num_t pin_sda)
{
#if defined(SOC_SERIES_STM32F1)
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    // SCK
    port = STM_GPIO_PORT(pin_scl);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_scl);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // SDA
    port = STM_GPIO_PORT(pin_sda);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_sda);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    return 0;
#else
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#ifdef GPIO_SPEED_FREQ_VERY_HIGH
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
#elif defined(GPIO_SPEED_FREQ_HIGH)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#endif
    GPIO_InitStruct.Alternate = af_i2c;

    // SCK
    port = STM_GPIO_PORT(pin_scl);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_scl);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // SDA
    port = STM_GPIO_PORT(pin_sda);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_sda);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    return 0;
#endif
}

#ifdef I2C1
// @brief 引脚复用为 I2C1
int32_t drv_pin_af_i2c1(pin_num_t pin_scl, pin_num_t pin_sda)
{
#if defined(GPIO_AF1_I2C1)
    return drv_pin_af_i2c(GPIO_AF1_I2C1, pin_scl, pin_sda);
#elif defined(GPIO_AF4_I2C1)
    return drv_pin_af_i2c(GPIO_AF4_I2C1, pin_scl, pin_sda);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_i2c(0, pin_scl, pin_sda);
#else 
    #error "Please defined(AF_I2C1)."
#endif
}
#endif /*I2C1*/
#ifdef I2C2
// @brief 引脚复用为 I2C2
int32_t drv_pin_af_i2c2(pin_num_t pin_scl, pin_num_t pin_sda)
{
#if defined(GPIO_AF1_I2C2)
    return drv_pin_af_i2c(GPIO_AF1_I2C2, pin_scl, pin_sda);
#elif defined(GPIO_AF4_I2C2)
    return drv_pin_af_i2c(GPIO_AF4_I2C2, pin_scl, pin_sda);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_i2c(0, pin_scl, pin_sda);
#else
    #error "Please defined(AF_I2C2)."
#endif
}
#endif /*I2C2*/
#ifdef I2C3
// @brief 引脚复用为 I2C3
int32_t drv_pin_af_i2c3(pin_num_t pin_scl, pin_num_t pin_sda)
{
#if defined(GPIO_AF4_I2C3)
    return drv_pin_af_i2c(GPIO_AF4_I2C3, pin_scl, pin_sda);
#elif defined(GPIO_AF2_I2C3)
    return drv_pin_af_i2c(GPIO_AF2_I2C3, pin_scl, pin_sda);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_i2c(0, pin_scl, pin_sda);
#else
    #error "Please defined(AF_I2C3)."
#endif
}
#endif /*I2C3*/



//////////////////////////////////////////////////////////////////////
// @brief 引脚复用为 SPI
int32_t drv_pin_af_spi(uint32_t af_spi, pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(SOC_SERIES_STM32F1)
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    // SCK
    port = STM_GPIO_PORT(pin_sck);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_sck);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // MOSI
    port = STM_GPIO_PORT(pin_mosi);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_mosi);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // MISO
    port = STM_GPIO_PORT(pin_miso);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_miso);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#ifdef GPIO_SPEED_FREQ_VERY_HIGH
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
#elif defined(GPIO_SPEED_FREQ_HIGH)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#endif
    GPIO_InitStruct.Alternate = af_spi; //

    // SCK
    port = STM_GPIO_PORT(pin_sck);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_sck);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // MOSI
    port = STM_GPIO_PORT(pin_mosi);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_mosi);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    // MISO
    port = STM_GPIO_PORT(pin_miso);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_miso);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
#endif
    return 0;
}
#ifdef SPI1
// @brief 引脚复用为 SPI1
int32_t drv_pin_af_spi1(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF0_SPI1)
    return drv_pin_af_spi(GPIO_AF0_SPI1, pin_sck, pin_mosi, pin_miso);
#elif defined(GPIO_AF1_SPI1)
    return drv_pin_af_spi(GPIO_AF1_SPI1, pin_sck, pin_mosi, pin_miso);
#elif defined(GPIO_AF5_SPI1)
    return drv_pin_af_spi(GPIO_AF5_SPI1, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI1)."
#endif
}
#endif /*SPI1*/
#ifdef SPI2
// @brief 引脚复用为 SPI2
int32_t drv_pin_af_spi2(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF1_SPI2)
    return drv_pin_af_spi(GPIO_AF1_SPI2, pin_sck, pin_mosi, pin_miso);
#elif defined(GPIO_AF5_SPI2)
    return drv_pin_af_spi(GPIO_AF5_SPI2, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI2)."
#endif
}
#endif /*SPI2*/
#ifdef SPI3
// @brief 引脚复用为 SPI3
int32_t drv_pin_af_spi3(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF5_SPI3)
    return drv_pin_af_spi(GPIO_AF5_SPI3, pin_sck, pin_mosi, pin_miso);
#elif defined(GPIO_AF6_SPI3)
    return drv_pin_af_spi(GPIO_AF6_SPI3, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI3)."
#endif
}
#endif /*SPI3*/
#ifdef SPI4
// @brief 引脚复用为 SPI4
int32_t drv_pin_af_spi4(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF5_SPI4)
    return drv_pin_af_spi(GPIO_AF5_SPI4, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI4)."
#endif
}
#endif /*SPI4*/
#ifdef SPI5
// @brief 引脚复用为 SPI5
int32_t drv_pin_af_spi5(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF5_SPI5)
    return drv_pin_af_spi(GPIO_AF5_SPI5, pin_sck, pin_mosi, pin_miso);
#elif defined(GPIO_AF6_SPI5)
    return drv_pin_af_spi(GPIO_AF6_SPI5, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI5)."
#endif
}
#endif /*SPI5*/
#ifdef SPI6
// @brief 引脚复用为 SPI6
int32_t drv_pin_af_spi6(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso)
{
#if defined(GPIO_AF5_SPI6)
    return drv_pin_af_spi(GPIO_AF5_SPI6, pin_sck, pin_mosi, pin_miso);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_spi(0, pin_sck, pin_mosi, pin_miso);
#else
    #error "Please defined(AF_SPI6)."
#endif
}
#endif /*SPI6*/






//////////////////////////////////////////////////////////////////////
// @brief 引脚复用为 uart
int32_t drv_pin_af_uart(uint32_t af_uart, pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(SOC_SERIES_STM32F1)
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    port = STM_GPIO_PORT(pin_tx);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_tx);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    port = STM_GPIO_PORT(pin_rx);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_rx);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
#else
    GPIO_TypeDef* port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = af_uart;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
#ifdef GPIO_SPEED_FREQ_VERY_HIGH
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
#elif defined(GPIO_SPEED_FREQ_HIGH)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#endif
    
    port = STM_GPIO_PORT(pin_tx);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_tx);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    port = STM_GPIO_PORT(pin_rx);
    GPIO_InitStruct.Pin = STM_GPIO_PIN(pin_rx);
    HAL_GPIO_Init(port, &GPIO_InitStruct);
#endif
    return 0;
}
#ifdef USART1
// @brief 引脚复用为 uart1
int32_t drv_pin_af_uart1(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF1_USART1)
    return drv_pin_af_uart(GPIO_AF1_USART1, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF7_USART1)
    return drv_pin_af_uart(GPIO_AF7_USART1, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART1)."
#endif
}
#endif /*USART1*/
#ifdef USART2
// @brief 引脚复用为 uart2
int32_t drv_pin_af_uart2(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF0_USART2)
    return drv_pin_af_uart(GPIO_AF0_USART2, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF1_USART2)
    return drv_pin_af_uart(GPIO_AF1_USART2, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF7_USART2)
    return drv_pin_af_uart(GPIO_AF7_USART2, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART2)."
#endif
}
#endif /*USART2*/
#ifdef USART3
// @brief 引脚复用为 uart3
int32_t drv_pin_af_uart3(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF1_USART3)
    return drv_pin_af_uart(GPIO_AF1_USART3, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF7_USART3)
    return drv_pin_af_uart(GPIO_AF7_USART3, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART3)."
#endif
}
#endif /*USART3*/
#ifdef UART4
// @brief 引脚复用为 uart4
int32_t drv_pin_af_uart4(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF0_USART4)
    return drv_pin_af_uart(GPIO_AF0_USART4, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF1_USART4)
    return drv_pin_af_uart(GPIO_AF1_USART4, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF8_UART4)
    return drv_pin_af_uart(GPIO_AF8_UART4, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F3)
    return drv_pin_af_uart(GPIO_AF5_UART4, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART4)."
#endif
}
#endif /*UART4*/
#ifdef UART5
// @brief 引脚复用为 uart5
int32_t drv_pin_af_uart5(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF1_USART5)
    return drv_pin_af_uart(GPIO_AF1_USART5, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF8_UART5)
    return drv_pin_af_uart(GPIO_AF8_UART5, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F3)
    return drv_pin_af_uart(GPIO_AF5_UART5, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART5)."
#endif
}
#endif /*UART5*/
#ifdef USART6
// @brief 引脚复用为 uart6
int32_t drv_pin_af_uart6(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF1_USART6)
    return drv_pin_af_uart(GPIO_AF1_USART6, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF7_USART6)
    return drv_pin_af_uart(GPIO_AF7_USART6, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF8_USART6)
    return drv_pin_af_uart(GPIO_AF8_USART6, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART6)."
#endif
}
#endif /*USART6*/
#ifdef UART7
// @brief 引脚复用为 uart7
int32_t drv_pin_af_uart7(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF1_USART7)
    return drv_pin_af_uart(GPIO_AF1_USART7, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF7_UART7)
    return drv_pin_af_uart(GPIO_AF7_UART7, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(GPIO_AF8_UART7)
    return drv_pin_af_uart(GPIO_AF8_UART7, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART7)."
#endif
}
#endif /*UART7*/
#ifdef UART8
// @brief 引脚复用为 uart8
int32_t drv_pin_af_uart8(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF8_UART8)
    return drv_pin_af_uart(GPIO_AF8_UART8, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART8)."
#endif
}
#endif /*UART8*/
#ifdef UART9
// @brief 引脚复用为 uart9
int32_t drv_pin_af_uart9(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF11_UART9)
    return drv_pin_af_uart(GPIO_AF11_UART9, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART9)."
#endif
}
#endif /*UART9*/
#ifdef UART10
// @brief 引脚复用为 uart10
int32_t drv_pin_af_uart10(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts)
{
#if defined(GPIO_AF11_UART10)
    return drv_pin_af_uart(GPIO_AF11_UART10, pin_tx, pin_rx, pin_rts, pin_cts);
#elif defined(SOC_SERIES_STM32F1)
    return drv_pin_af_uart(0, pin_tx, pin_rx, pin_rts, pin_cts);
#else
    #error "Please defined(AF_USART10)."
#endif
}
#endif /*UART10*/



