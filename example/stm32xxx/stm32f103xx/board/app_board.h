#ifndef __APP_BOARD_H__
#define __APP_BOARD_H__

#include <stm32f1xx.h>
#include "htd_task.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#ifndef Error_Handler
    void stm32_error_handler(const char *s, int num);
    #define Error_Handler()     stm32_error_handler(__FUNCTION__, __LINE__)
#endif

// @brief 初始化板子
void app_board_init(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __APP_BOARD_H__ */

