#ifndef _HTD_COMPONENTS_H_
#define _HTD_COMPONENTS_H_

#include "htd_task.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#ifdef HTD_CONSOLE_ENABLE
    #include "htd_console_process.h"
#endif

#ifdef HTD_BOOT_ENABLE
    #include "htd_boot_process.h"
#endif

#ifdef HTD_AUTHOR_ENABLE
    #include "htd_author.h"
#endif

#ifdef HTD_BUTTON_ENABLE
    #include "htd_button_gpio.h"
    #include "htd_button_matrix.h"
    #include "htd_button_charlie.h"
#endif
#ifdef HTD_LED_ENABLE
    #include "htd_led_matrix.h"
    #include "htd_led_charlie.h"
    #include "htd_led_blink.h"
#endif

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_COMPONENTS_H_*/

