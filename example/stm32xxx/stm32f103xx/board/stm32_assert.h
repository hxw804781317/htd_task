#ifndef __STM32_ASSERT_H
#define __STM32_ASSERT_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif


#if (defined(USE_FULL_ASSERT) && (USE_FULL_ASSERT == 1))
    #ifndef Error_Handler
        void stm32_error_handler(const char *s, int num);
        #define Error_Handler()     stm32_error_handler(__FUNCTION__, __LINE__)
    #endif
#else
    #define Error_Handler()
#endif



#ifdef __cplusplus
}
#endif
#endif /* __STM32_ASSERT_H */

