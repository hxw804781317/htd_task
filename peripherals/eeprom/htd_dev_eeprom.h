#ifndef _HTD_DEV_EEPROM_H_
#define _HTD_DEV_EEPROM_H_

#include <stdint.h>
#include <stdbool.h>
#include <htd_fifo.h>
#include <htd_device.h>
#include <htd_dev_i2cm.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#define EEPROM_CMD_NONE             0
#define EEPROM_CMD_CLOSE            1
#define EEPROM_CMD_SHOW_INFO        2 
#define EEPROM_CMD_SIZE_GET         3
typedef uint32_t htd_eeprom_cmd_t;

typedef struct {
    const char* p_name;
    int32_t (*open_handle)(void);
    int32_t (*write_handle)(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len);
    int32_t (*read_handle)(uint32_t addr, uint8_t* p_buff, uint32_t buff_len);
    int32_t (*ctrl_handle)(htd_eeprom_cmd_t cmd, void* p_param, uint32_t param_len);
} htd_eeprom_dev_t;

// @brief 注册(只能调用一次)
int32_t htd_dev_eeprom_register(const htd_eeprom_dev_t* p_eeprom);
// @brief 展示已经挂载的eeprom信息
void htd_dev_eeprom_show(void);


// @brief 读数据
int32_t htd_dev_eeprom_read(uint32_t addr, uint8_t* p_data, uint32_t data_len);
// @brief 写数据
int32_t htd_dev_eeprom_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len);
// @brief 获取大小
uint32_t htd_dev_eeprom_size_get(void);
// @brief 初始化
int32_t htd_dev_eeprom_init(void);
// @brief 去初始化
int32_t htd_dev_eeprom_uninit(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_EEPROM_H_*/

