#ifndef __STM32_ASSERT_H
#define __STM32_ASSERT_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif

#if (defined(USE_FULL_ASSERT) && (USE_FULL_ASSERT == 1))
    void htd_assert_failed(const char* file, uint32_t line);
    #define assert_param(expr) ((expr) ? (void)0U : htd_assert_failed(__FILE__, __LINE__))
#else
    #define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */


#ifndef Error_Handler
    void stm32_error_handler(const char *s, int num);
    #define Error_Handler()     stm32_error_handler(__FUNCTION__, __LINE__)
#endif


#ifdef __cplusplus
}
#endif
#endif /* __STM32_ASSERT_H */

