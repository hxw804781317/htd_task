// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LED_MATRIX_H_
#define _HTD_LED_MATRIX_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "htd_led_port.h"


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 注意：共阴的为行，共阳的为列，列拉高，对应的行拉低，灯亮
typedef struct {
    uint32_t *state_table; 
    pin_num_t *pin_row_table;
    pin_num_t *pin_col_table;
    uint8_t pin_row_count;
    uint8_t pin_col_count;
    uint8_t state_table_count;
    uint8_t is_running;
}led_matrix_t;


// @brief 创建一个矩阵控制LED对象
led_matrix_t* htd_led_matrix_create(const pin_num_t *pin_row_table, uint8_t pin_row_count, const pin_num_t* pin_column_table, uint8_t pin_column_count);
// @brief 销毁矩阵控制 LED 对象
int32_t htd_led_matrix_destroy(led_matrix_t* p_object);
// @brief 启动矩阵控制
int32_t htd_led_matrix_start(led_matrix_t* p_object);
// @brief 关闭矩阵控制扫描, 所有灯都会灭
void htd_led_matrix_stop(led_matrix_t* p_object);
// @brief 控制某个灯亮/灭
int32_t htd_led_matrix_onoff(led_matrix_t* p_object, uint32_t led_idx, uint8_t onoff);
// @brief 显示按键矩阵序号
void htd_led_matrix_show(led_matrix_t* p_object);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LED_MATRIX_H_*/

