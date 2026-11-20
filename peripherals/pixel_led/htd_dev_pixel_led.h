#ifndef _HTD_DEV_PIXEL_LED_H_
#define _HTD_DEV_PIXEL_LED_H_

#include <stdint.h>
#include <stdbool.h>
#include <htd_fifo.h>
#include <htd_device.h>
#include <htd_dev_i2cm.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#define PLED_CMD_NONE               0
#define PLED_CMD_OPEN               1
#define PLED_CMD_CLOSE              2
#define PLED_CMD_COLOR_SET          3
#define PLED_CMD_SHOW_INFO          4 
typedef uint32_t htd_pled_cmd_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} pled_color_t;

typedef struct htd_pled_dev_s htd_pled_dev_t;
struct htd_pled_dev_s{
    const char* p_name;
    int32_t (*ctrl_handle)(htd_pled_cmd_t cmd, void* p_param, uint32_t param_len);
};

// @brief 注册(只能调用一次)
int32_t htd_dev_pled_register(const htd_pled_dev_t* p_pled);
// @brief 初始化
int32_t htd_dev_pled_init(void);
// @brief 去初始化
int32_t htd_dev_pled_uninit(void);

// @brief 设置像素灯的颜色
// @param index 第几个灯，多芯片时才有用
int32_t htd_dev_pled_set(uint32_t index, const pled_color_t* p_led, uint32_t count);
// @brief 展示信息
void htd_dev_pled_show(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_PIXEL_LED_H_*/

