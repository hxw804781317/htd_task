#ifndef _HTD_DRV_GPIO_H_
#define _HTD_DRV_GPIO_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "stm32_assert.h"
#include "htd_dev_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// @brief 将 PA13/PA14复用为SWD引脚
void drv_pin_af_swd(void);


// @brief 引脚复用为 uart1
int32_t drv_pin_af_uart1(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart2
int32_t drv_pin_af_uart2(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart3
int32_t drv_pin_af_uart3(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart4
int32_t drv_pin_af_uart4(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart5
int32_t drv_pin_af_uart5(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart6
int32_t drv_pin_af_uart6(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart7
int32_t drv_pin_af_uart7(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart8
int32_t drv_pin_af_uart8(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart9
int32_t drv_pin_af_uart9(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
// @brief 引脚复用为 uart10
int32_t drv_pin_af_uart10(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);



// @brief 引脚复用为 SPI1
int32_t drv_pin_af_spi1(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
// @brief 引脚复用为 SPI2
int32_t drv_pin_af_spi2(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
// @brief 引脚复用为 SPI3
int32_t drv_pin_af_spi3(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
// @brief 引脚复用为 SPI4
int32_t drv_pin_af_spi4(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
// @brief 引脚复用为 SPI5
int32_t drv_pin_af_spi5(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
// @brief 引脚复用为 SPI6
int32_t drv_pin_af_spi6(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);



// @brief 引脚复用为 I2C1
int32_t drv_pin_af_i2c1(pin_num_t pin_scl, pin_num_t pin_sda);
// @brief 引脚复用为 I2C2
int32_t drv_pin_af_i2c2(pin_num_t pin_scl, pin_num_t pin_sda);
// @brief 引脚复用为 I2C3
int32_t drv_pin_af_i2c3(pin_num_t pin_scl, pin_num_t pin_sda);



#ifdef __cplusplus
extern "C" {
#endif
#endif /*_HTD_DRV_GPIO_H_*/

