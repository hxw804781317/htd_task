#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "pled.ws2812b"
#include "htd_common.h"
#include "htd_dev_pixel_led.h"
#include "htd_device.h"


#if (defined(HTD_PIXEL_LED_USING_WS2812B) && (HTD_PIXEL_LED_USING_WS2812B == 1))
//////////////////////////////////////////////////////////////////////
#if defined(HTD_PIXEL_LED_1WIRE_USE_PWM)
static htd_pwm_handle_t* m_ws2812b_bus = NULL;
#endif

// @brief 总线初始化
static int32_t ws2812b_bus_init(void)
{
#if defined(HTD_PIXEL_LED_1WIRE_USE_PWM)
    uint32_t freq_hz;
    htd_pwm_user_config_t pwm_config = {
        .pin_num = {PIN_NUM_INVALID, PIN_NUM_INVALID, PIN_NUM_INVALID, PIN_NUM_INVALID},
        .pin_auto = HTD_PIXEL_LED_1WIRE_PIN,            // 使用自适配引脚通道方式
        .mode = HTD_PWM_MODE_HL,
        .prescaler = 0,
    };
    htd_delay_ms(100);
    m_ws2812b_bus = htd_dev_pwm_open(HTD_PIXEL_LED_BUS_NAME, &pwm_config);
    ASSERT(m_ws2812b_bus);
    // 设置周期
    freq_hz = htd_dev_pwm_freq_hz_get(m_ws2812b_bus);
    freq_hz = 125 * (freq_hz / (1000*1000)) / 100;       // 1.25us
    htd_dev_pwm_period_set(m_ws2812b_bus, freq_hz);
#else
    htd_pin_output_pp_set(HTD_PIXEL_LED_1WIRE_PIN);
    htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 0);
#endif
    return 0;
}

// @brief 发送数据, 数据格式按照 g->r->b 为一组
void ws2812b_1wire_data_write(uint8_t* p_data, uint32_t data_len)
{
    uint8_t byte;
    uint32_t start_ticks, i, irq_level;
    extern uint32_t SystemCoreClock;
    //uint32_t T1H_ticks_400ns = 220 * (SystemCoreClock / 1000000) / 1000;
    //uint32_t T0H_ticks_400ns = 300 * (SystemCoreClock / 1000000) / 1000;
    uint32_t T1H_ticks_800ns = 720 * (SystemCoreClock / 1000000) / 1000;
    uint32_t T0H_ticks_800ns = 700 * (SystemCoreClock / 1000000) / 1000;

    // 关中断
    irq_level = htd_interrupt_disable();
    // 记录DWT计时器的寄存器状态
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;
    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    while (data_len--) {
        byte = *p_data++;
        for (i=0; i<8; i++) {
            if (byte & 0x80) {
                start_ticks = DWT->CYCCNT;
                htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 1);
                while (DWT->CYCCNT - start_ticks < T1H_ticks_800ns);
                
                start_ticks = DWT->CYCCNT;
                htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 0);
                //while (DWT->CYCCNT - start_ticks < T1H_ticks_400ns);
            } else {
                start_ticks = DWT->CYCCNT;
                htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 1);
                //while (DWT->CYCCNT - start_ticks < T0H_ticks_400ns);
                
                start_ticks = DWT->CYCCNT;
                htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 0);
                while (DWT->CYCCNT - start_ticks < T0H_ticks_800ns);
            }
            byte <<= 1;
        }
    }
    
    // 恢复DWT计时器的原始状态
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
    // 开中断
    htd_interrupt_enable(irq_level);
}

// @brief 发送数据, 数据格式按照 g->r->b 为一组
void ws2812b_pwm_data_write(uint8_t* p_data, uint32_t data_len)
{
#if defined(HTD_PIXEL_LED_1WIRE_USE_PWM)
    // 设置数据
    uint32_t low_hz = m_ws2812b_bus->period * 32 / 100;
    uint32_t high_hz = m_ws2812b_bus->period - low_hz;
    uint8_t *p_buff = htd_malloc(data_len * 8 + 10);
    ASSERT(p_buff);
    uint8_t byte;
    uint32_t i, buff_len = 0;
    while (data_len--) {
        byte = *p_data++;
        for (i=0; i<8; ++i) {
            if (byte & 0x80) {          // 高位在前
                p_buff[buff_len] = high_hz;
            } else {
                p_buff[buff_len] = low_hz;
            }
            //print_debug("freq=%d", p_buff[buff_len]);
            byte <<= 1;
            buff_len++;
        }
    }
    htd_memset(&p_buff[buff_len], 0, 10);
    buff_len += 10;
    htd_dev_pwm_duty8_array_set(m_ws2812b_bus, 0xff, p_buff, buff_len, 1);
    htd_free(p_buff);
    htd_dev_pwm_start(m_ws2812b_bus, 0xff);
#endif
}

// @brief 总线去初始化
static int32_t ws2812b_bus_deinit(void)
{
#if defined(HTD_PIXEL_LED_1WIRE_USE_PWM)
    if (m_ws2812b_bus) {
        htd_dev_pwm_close(m_ws2812b_bus);
        m_ws2812b_bus = NULL;
    }
#endif
    htd_pin_output_pp_set(HTD_PIXEL_LED_1WIRE_PIN);
    htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 0);
    return 0;
}

// @brief 写数据
static int32_t drv_ws2812b_color_set(const pled_color_t* p_led, uint32_t count)
{
    if (count > HTD_PIXEL_LED_COUNT) {
        count = HTD_PIXEL_LED_COUNT;
    }

    uint8_t* p_data = (uint8_t*)htd_malloc(3 * sizeof(uint8_t) * count);
    ASSERT(p_data);
    uint8_t* p_ptr = p_data;
    while (count--) {
        *p_ptr++ = p_led->g;
        *p_ptr++ = p_led->r;
        *p_ptr++ = p_led->b;
        p_led++;
    }
    
#if defined(HTD_PIXEL_LED_1WIRE_USE_PWM)
    // 使用 PWM 模拟发送
    ws2812b_pwm_data_write(p_data, p_ptr - p_data);
    htd_free(p_data);
#else
    ws2812b_1wire_data_write(p_data, p_ptr - p_data);
    htd_free(p_data);
    htd_pin_write(HTD_PIXEL_LED_1WIRE_PIN, 0);
    htd_delay_us(52);
#endif
    return 0;
}

// @brief 初始化，启动设备
static int32_t drv_ws2812b_open(void)
{
    // 初始化硬件接口
    ws2812b_bus_init();
    
    pled_color_t led[HTD_PIXEL_LED_COUNT];
    htd_memset(led, 0, sizeof(led));
    drv_ws2812b_color_set(led, HTD_PIXEL_LED_COUNT);
    htd_delay_us(HTD_PIXEL_LED_COUNT * 30 + 2);
    return 0;
}

// @brief 去初始化，设备处于低功耗状态
static void drv_ws2812b_close(bool is_power_off)
{
    print_warning("ws2812b uninit. is_power_off=%d", is_power_off);
    if (is_power_off) {
        ws2812b_bus_deinit();
    } 
}

// @brief 控制句柄
int32_t drv_ws2812b_ctrl(htd_pled_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == PLED_CMD_OPEN) {
        drv_ws2812b_open();
    } else if (cmd == PLED_CMD_CLOSE) {
        drv_ws2812b_close(true);
    } else if (cmd == PLED_CMD_COLOR_SET) {
        drv_ws2812b_color_set((pled_color_t*)p_param, param_len);
    } else if (cmd == PLED_CMD_SHOW_INFO) {
        
    } else {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_ws2812b_register(void)
{
    const static htd_pled_dev_t m_pled_dev = {
        .p_name = "ws2812b",
        .ctrl_handle = drv_ws2812b_ctrl,
    };
    return htd_dev_pled_register(&m_pled_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_ws2812b_register);
#endif /*(defined(HTD_PIXEL_LED_USING_WS2812B) && (HTD_PIXEL_LED_USING_WS2812B == 1))*/

