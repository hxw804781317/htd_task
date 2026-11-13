#ifndef _HTD_CONFIG_PROJECT_H_
#define _HTD_CONFIG_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 默认使用 sdk 自带的功能模块
#define xUSE_SDK_SEGGER_RTT                      1

// 选择 FreeRTOS 的版本
#ifdef HTD_TASK_FREERTOS
    //#define USE_FREERTOS_V1031                  1
    //#define USE_FREERTOS_V1046                  1
    #define USE_FREERTOS_V1051                  1
#endif

// 是否使用外部晶振(32768)
#define BSP_CLOCK_USE_XTAL32768                 1

// 消息队列是否支持超时唤醒(1支持，0不支持)
#define OS_SUPPORT_MQ_TIMEROUT_WAKEUP           1


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_CONFIG_PROJECT_H_*/

