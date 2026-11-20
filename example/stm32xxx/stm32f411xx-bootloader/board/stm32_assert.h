#ifndef __STM32_ASSERT_H
#define __STM32_ASSERT_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifndef Error_Handler
    void stm32_error_handler(const char *s, int num);
    #define Error_Handler()     stm32_error_handler(__FUNCTION__, __LINE__)
#endif


#ifdef __cplusplus
}
#endif
#endif /* __STM32_ASSERT_H */

