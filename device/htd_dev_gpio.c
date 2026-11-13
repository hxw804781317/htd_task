#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "gpio"
#include "htd_device.h"
#include "htd_dev_gpio.h"



//////////////////////////////////////////////////////////////////////
// @brief 查找中断函数
HTD_SECTION_DEF(HTD_PIN_INT_FUNC_SECTION_NAME, pin_int_cb_t);
const pin_int_cb_t* htd_pin_int_func_find(pin_num_t pin_num)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
    
    const pin_int_cb_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_dev = (pin_int_cb_t*)p_item;
        if (p_dev->magic == HTD_PIN_INT_FUNC_MAGIC) {
            if (p_dev->pin_num == pin_num) {
                return p_dev;
            }
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(pin_int_cb_t));
    }
    return NULL;
}

// @brief 获取中断函数列表起始/结束地址
void htd_pin_int_func_addr_get(uint32_t** start_addr, uint32_t** end_addr)
{
    ASSERT(start_addr);
    ASSERT(end_addr);
    
    *start_addr = (uint32_t*)HTD_SECTION_START_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
    *end_addr = (uint32_t*)HTD_SECTION_END_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
}

// @brief 控制电平反转
__WEAK void htd_pin_toggle(pin_num_t pin_num)
{
    print_error("week function: htd_pin_toggle");
}
// @brief 控制引脚电平
__WEAK void htd_pin_write(pin_num_t pin_num, uint8_t val)
{
    print_error("week function: htd_pin_write");
}
// @brief 读取引脚电平
__WEAK uint32_t htd_pin_read(pin_num_t pin_num)
{
    print_error("week function: htd_pin_read");
    return 0;
}
// @brief 配置为推挽输出模式
__WEAK void htd_pin_output_pp_set(pin_num_t pin_num)
{
    print_error("week function: htd_pin_output_pp_set");
}
// @brief 配置为开漏输出模式
__WEAK void htd_pin_output_od_set(pin_num_t pin_num)
{
    print_error("week function: htd_pin_output_od_set");
}
// @brief 配置引脚为输入模式
__WEAK void htd_pin_input_set(pin_num_t pin_num, htd_pin_pull_t pin_pull)
{
    print_error("week function: htd_pin_input_set");
}
// @brief 设置为模拟输入
__WEAK void htd_pin_input_analog_set(pin_num_t pin_num)
{
    print_error("week function: htd_pin_input_analog_set");
}

// @brief 配置引脚为高阻态模式(方向设为入，缓冲要断开，上下拉全关)
__WEAK void htd_pin_hiz_set(pin_num_t pin_num)
{
    print_error("week function: htd_pin_hiz_set");
}

// @brief 设置引脚方向
__WEAK void htd_pin_dir_set(pin_num_t pin_num, htd_pin_dir_t pin_dir)
{
    print_error("week function: htd_pin_dir_set");
}

// @brief 配置为中断引脚
__WEAK int32_t htd_pin_int_set(pin_num_t pin_num, pin_int_mode_t mode, uint8_t is_enable)
{
    print_error("week function: htd_pin_int_set");
    return 0;
}
// @brief 配置为复用I2C模式
__WEAK void htd_pin_afi2c_set(pin_num_t pin_num)
{
    print_error("week function: htd_pin_afi2c_set");
}
// @brief 配置引脚为唤醒引脚
__WEAK void htd_pin_wake_up_cfg(pin_num_t pin_num, uint8_t wake_up_level, bool enable)
{
    print_error("week function: htd_pin_wake_up_cfg");
}

// @brief 显示某个引脚的寄存器信息
__WEAK void htd_pin_info_show(pin_num_t pin_num)
{
    print_error("week function: htd_pin_info_show");
}

// @brief 显示所有中断引脚
void htd_pin_int_func_show(void)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_PIN_INT_FUNC_SECTION_NAME);
    
    const pin_int_cb_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_dev = (pin_int_cb_t*)p_item;
        if (p_dev->magic == HTD_PIN_INT_FUNC_MAGIC) {
            print_warning("pin_num=%d, int_func=%p", p_dev->pin_num, p_dev->int_func);
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(pin_int_cb_t));
    }
}


void invalid_pin_int(void) {}
HTD_PIN_INT_FUNC_REGISTER(PIN_NUM_INVALID, invalid_pin_int);





//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))
#include "htd_components.h"

static pin_num_t pin_name2num(const char* pin_name)
{
    uint32_t i;
    const char* name_table[] = {"PA", "PB", "PC", "PD", "PE", "PF", "PG"};
    for (i=0; i<ARRAY_COUNT(name_table); ++i) {
        if (0 == memcmp(pin_name, name_table[i], 2)) {
            break;
        }
    }
    if (i >= ARRAY_COUNT(name_table)) {
        if (memcmp(pin_name, "PZ", 2) == 0) {
            i = 11;
        } else {
            return PIN_NUM_INVALID;
        }
    }
    const char* p_ptr = &pin_name[2];
    if (*p_ptr == '0') {
        p_ptr++;
    }
    return i*16 + atoi(p_ptr);
}

// @brief 调试命令
static int pin_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_debug("usage: pin <mode|write|read|number|show|int_show> #pin_num <value>");
        print_debug("\texample:");
        print_debug("\t\tpin mode  #pin_num <PIN MODE VALUES>");
        print_debug("\t\tpin write #pin_num 1");
        print_debug("\t\tpin read  #pin_num");
        print_debug("\t\tpin show  #pin_num");
        print_debug("\t\tpin number <PA01>\r\n");
        print_debug("\tPIN MODE VALUES:");
        print_debug("\t\tPIN_MODE_OUTPUT_PP      0x00");
        print_debug("\t\tPIN_MODE_OUTPUT_OD      0x01");
        print_debug("\t\tPIN_MODE_INPUT          0x02");
        print_debug("\t\tPIN_MODE_INPUT_PULLUP   0x03");
        print_debug("\t\tPIN_MODE_INPUT_PULLDOWN 0x04");
        return -1;
    }

    uint8_t level;
    uint8_t pin_mode;
    pin_num_t pin_num;

    // 配置引脚类型
    if (memcmp(argv[1], "mode", 5) == 0) {
        if (argc != 4) {
            print_error("pin mode #pin_num <PIN MODE VALUES>");
            return -1;
        }
        pin_num = atoi(argv[2]);
        pin_mode = atoi(argv[3]);
        if (pin_mode == 0) {
            // 推挽输出
            htd_pin_output_pp_set(pin_num);
            print_warning("pin(%d) config output pp.", pin_num);
        } else if (pin_mode == 1) {
            // 开漏输出
            htd_pin_output_od_set(pin_num);
            print_warning("pin(%d) config output od.", pin_num);
        } else if (pin_mode == 2) {
            // 浮空输入
            htd_pin_input_set(pin_num, HTD_PIN_NOPULL);
            print_warning("pin(%d) config input.", pin_num);
        } else if (pin_mode == 3) {
            // 上拉输入
            htd_pin_input_set(pin_num, HTD_PIN_PULLUP);
            print_warning("pin(%d) config input pullup.", pin_num);
        } else if (pin_mode == 4) {
            // 下拉输入
            htd_pin_input_set(pin_num, HTD_PIN_PULLDOWN);
            print_warning("pin(%d) config input pulldown.", pin_num);
        } else {
            print_error("pin mode set failed. mode=<0~4>");
            return -1;
        }
        return 0;
    }

    // 设置引脚电平
    if (memcmp(argv[1], "write", 6) == 0) {
        if (argc != 4) {
            print_error("pin write #pin_num 1/0");
            return -1;
        }
        pin_num = atoi(argv[2]);
        level = atoi(argv[3]);
        htd_pin_write(pin_num, level);
        print_warning("pin write %d %d", pin_num, level);
        return 0;
    }

    // 读取引脚电平
    if (memcmp(argv[1], "read", 5) == 0) {
        if (argc != 3) {
            print_error("pin read #pin_num");
            return -1;
        }
        pin_num = atoi(argv[2]);
        level = htd_pin_read(pin_num);
        print_warning("pin(%d) level: %d", pin_num, level);
        return 0;
    }

    // 查看引脚序号
    if (memcmp(argv[1], "number", 7) == 0) {
        if (argc != 3) {
            print_error("pin number PA01~PD15");
            return -1;
        }
        pin_num = pin_name2num(argv[2]);
        if (pin_num == PIN_NUM_INVALID) {
            print_error("pin number PA01~PD15");
            return -1;
        }
        print_warning("%s: pin_num=%d", argv[2], pin_num);
        return 0;
    }

    // 查看引脚寄存器值
    if (memcmp(argv[1], "show", 5) == 0) {
        if (argc != 3) {
            print_error("pin show #pin_num");
            return -1;
        }
        pin_num = atoi(argv[2]);
        htd_pin_info_show(pin_num);
        return 0;
    }

    // 显示注册的中断函数
    if (memcmp(argv[1], "int_show", 9) == 0) {
        htd_pin_int_func_show();
        return 0;
    }
    return 0;
}
CONSOLE_CMD_EXPORT(pin_cmd, pin);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/


