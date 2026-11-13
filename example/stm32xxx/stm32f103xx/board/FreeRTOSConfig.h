#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "htd_config.h"


/******************************************************************************/
/* Debugging assistance. ******************************************************/
/******************************************************************************/

/* configASSERT() has the same semantics as the standard C assert().  It can
 * either be defined to take an action when the assertion fails, or not defined
 * at all (i.e. comment out or delete the definitions) to completely remove
 * assertions.  configASSERT() can be defined to anything you want, for example
 * you can call a function if an assert fails that passes the filename and line
 * number of the failing assert (for example, "vAssertCalled( __FILE__, __LINE__
 * )" or it can simple disable interrupts and sit in a loop to halt all
 * execution on the failing line for viewing in a debugger. */
extern void htd_assert_failed(const char* file, uint32_t line); 
#define configASSERT(x)                                                 \
    if ((x) == 0) {                                                     \
        taskDISABLE_INTERRUPTS();                                       \
        htd_assert_failed(__FILE__, __LINE__);                          \
    }

/******************************************************************************/
/* Hardware description related definitions. **********************************/
/******************************************************************************/
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
    void xPortSysTickHandler(void);
#endif

#ifndef CMSIS_device_header
    #define CMSIS_device_header "stm32f1xx.h"
#endif /* CMSIS_device_header */


/*
 * 写入实际的CPU内核时钟频率，也就是CPU指令执行频率，通常称为Fclk
 * Fclk为供给CPU内核的时钟信号，我们所说的cpu主频为 XX MHz，
 * 就是指的这个时钟信号，相应的，1/Fclk即为cpu时钟周期；
 */
//#define configCPU_CLOCK_HZ    ( ( unsigned long ) 20000000 )
#define configCPU_CLOCK_HZ                       (SystemCoreClock)


/* configSYSTICK_CLOCK_HZ is an optional parameter for ARM Cortex-M ports only.
 *
 * By default ARM Cortex-M ports generate the RTOS tick interrupt from the
 * Cortex-M SysTick timer. Most Cortex-M MCUs run the SysTick timer at the same
 * frequency as the MCU itself - when that is the case configSYSTICK_CLOCK_HZ is
 * not needed and should be left undefined. If the SysTick timer is clocked at a
 * different frequency to the MCU core then set configCPU_CLOCK_HZ to the MCU
 * clock frequency, as normal, and configSYSTICK_CLOCK_HZ to the SysTick clock
 * frequency.  Not used if left undefined.
 * The default value is undefined (commented out).  If you need this value bring
 * it back and set it to a suitable value. */

/*
 #define configSYSTICK_CLOCK_HZ                  [Platform specific]
 */

// RTOS系统节拍中断的频率。即一秒中断的次数，每次中断RTOS都会进行任务调度
#define configTICK_RATE_HZ                         1000





/************************************************************************
 *               FreeRTOS
 *********************************************************************/
/* 置1：RTOS使用抢占式调度器；置0：RTOS使用协作式调度器（时间片）
 * 
 * 注：在多任务管理机制上，操作系统可以分为抢占式和协作式两种。
 * 协作式操作系统是任务主动释放CPU后，切换到下一个任务。
 * 任务切换的时机完全取决于正在运行的任务。
 */
#define configUSE_PREEMPTION                       1

// 使能时间片调度(默认式使能的)
#define configUSE_TIME_SLICING                     0

/* 任务切换方式，0通用方式，1特殊方式(需要硬件支持)
 * 某些运行FreeRTOS的硬件有两种方法选择下一个要执行的任务：
 * 通用方法和特定于硬件的方法（以下简称“特殊方法”）。
 * 
 * 通用方法：
 *      1.configUSE_PORT_OPTIMISED_TASK_SELECTION 为 0 或者硬件不支持这种特殊方法。
 *      2.可以用于所有FreeRTOS支持的硬件
 *      3.完全用C实现，效率略低于特殊方法。
 *      4.不强制要求限制最大可用优先级数目
 * 特殊方法：
 *      1.必须将configUSE_PORT_OPTIMISED_TASK_SELECTION设置为1。
 *      2.依赖一个或多个特定架构的汇编指令（一般是类似计算前导零[CLZ]指令）。
 *      3.比通用方法更高效
 *      4.一般强制限定最大可用优先级数目为32
 * 一般是硬件计算前导零指令，如果所使用的，MCU没有这些硬件指令的话此宏应该设置为0！
 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION     0


/* 置1：使能低功耗tickless模式；置0：保持系统节拍（tick）中断一直运行
 * 假设开启低功耗的话可能会导致下载出现问题，因为程序在睡眠中,可用以下办法解决
 */
#define configUSE_TICKLESS_IDLE                     0

// 配置预期空闲时间, 它指定了系统至少空闲多少个Tick周期后，才真正进入Tickless模式
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP       2

// 可使用最大优先级
#define configMAX_PRIORITIES                        32

// 空闲任务堆栈大小
#define configMINIMAL_STACK_SIZE                    128

// 任务名字字符最大长度
#define configMAX_TASK_NAME_LEN                     16

/* Time is measured in 'ticks' - which is the number of times the tick interrupt
 * has executed since the RTOS kernel was started.
 * The tick count is held in a variable of type TickType_t.
 *
 * configTICK_TYPE_WIDTH_IN_BITS controls the type (and therefore bit-width) of
 * TickType_t:
 *
 * Defining configTICK_TYPE_WIDTH_IN_BITS as TICK_TYPE_WIDTH_16_BITS causes
 * TickType_t to be defined (typedef'ed) as an unsigned 16-bit type.
 *
 * Defining configTICK_TYPE_WIDTH_IN_BITS as TICK_TYPE_WIDTH_32_BITS causes
 * TickType_t to be defined (typedef'ed) as an unsigned 32-bit type.
 *
 * Defining configTICK_TYPE_WIDTH_IN_BITS as TICK_TYPE_WIDTH_64_BITS causes
 * TickType_t to be defined (typedef'ed) as an unsigned 64-bit type. */
//#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_32_BITS
#define configUSE_16_BIT_TICKS                      0


// 闲任务放弃CPU使用权给其他同优先级的用户任务
#define configIDLE_SHOULD_YIELD                     1

/* Each task has an array of task notifications.
 * configTASK_NOTIFICATION_ARRAY_ENTRIES sets the number of indexes in the
 * array. See https://www.freertos.org/RTOS-task-notifications.html  Defaults to
 * 1 if left undefined. */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES       1

// 设置可以注册的信号量和消息队列个数
#define configQUEUE_REGISTRY_SIZE                   8

/* Set configENABLE_BACKWARD_COMPATIBILITY to 1 to map function names and
 * datatypes from old version of FreeRTOS to their latest equivalent.  Defaults
 * to 1 if left undefined. */
#define configENABLE_BACKWARD_COMPATIBILITY         0

/* Each task has its own array of pointers that can be used as thread local
 * storage.  configNUM_THREAD_LOCAL_STORAGE_POINTERS set the number of indexes
 * in the array.  See
 * https://www.freertos.org/thread-local-storage-pointers.html Defaults to 0 if
 * left undefined. */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS     0

/* When configUSE_MINI_LIST_ITEM is set to 0, MiniListItem_t and ListItem_t are
 * both the same. When configUSE_MINI_LIST_ITEM is set to 1, MiniListItem_t
 * contains 3 fewer fields than ListItem_t which saves some RAM at the cost of
 * violating strict aliasing rules which some compilers depend on for
 * optimization. Defaults to 1 if left undefined. */
#define configUSE_MINI_LIST_ITEM                   1

/* Sets the type used by the parameter to xTaskCreate() that specifies the stack
 * size of the task being created.  The same type is used to return information
 * about stack usage in various other API calls.  Defaults to size_t if left
 * undefined. */
#define configSTACK_DEPTH_TYPE                     size_t

/* configMESSAGE_BUFFER_LENGTH_TYPE sets the type used to store the length of
 * each message written to a FreeRTOS message buffer (the length is also written
 * to the message buffer.  Defaults to size_t if left undefined - but that may
 * waste space if messages never go above a length that could be held in a
 * uint8_t. */
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t

/* If configHEAP_CLEAR_MEMORY_ON_FREE is set to 1, then blocks of memory
 * allocated using pvPortMalloc() will be cleared (i.e. set to zero) when freed
 * using vPortFree(). Defaults to 0 if left undefined. */
#define configHEAP_CLEAR_MEMORY_ON_FREE            1

/* vTaskList and vTaskGetRunTimeStats APIs take a buffer as a parameter and
 * assume that the length of the buffer is configSTATS_BUFFER_MAX_LENGTH.
 * Defaults to 0xFFFF if left undefined. New applications are recommended to use
 * vTaskListTasks and vTaskGetRunTimeStatistics APIs instead and supply the
 * length of the buffer explicitly to avoid memory corruption. */
#define configSTATS_BUFFER_MAX_LENGTH              0xFFFF

/* Set configUSE_NEWLIB_REENTRANT to 1 to have a newlib reent structure
 * allocated for each task.  Set to 0 to not support newlib reent structures.
 * Default to 0 if left undefined.
 *
 * Note Newlib support has been included by popular demand, but is not used or
 * tested by the FreeRTOS maintainers themselves. FreeRTOS is not responsible
 * for resulting newlib operation. User must be familiar with newlib and must
 * provide system-wide implementations of the necessary stubs. Note that (at the
 * time of writing) the current newlib design implements a system-wide malloc()
 * that must be provided with locks. */
#define configUSE_NEWLIB_REENTRANT                  0


/***********************************************************************
                FreeRTOS与软件定时器有关的配置选项      
**********************************************************************/
// 启用软件定时器
#define configUSE_TIMERS                            1

// 软件定时器优先级
#define configTIMER_TASK_PRIORITY                   (configMAX_PRIORITIES - 1)

// 软件定时器任务堆栈大小
#define configTIMER_TASK_STACK_DEPTH                configMINIMAL_STACK_SIZE

//软件定时器队列长度
#define configTIMER_QUEUE_LENGTH                    8



/******************************************************************************/
/* Event Group related definitions. *******************************************/
/******************************************************************************/
/* Set configUSE_EVENT_GROUPS to 1 to include event group functionality in the
 * build. Set to 0 to exclude event group functionality from the build. The
 * FreeRTOS/source/event_groups.c source file must be included in the build if
 * configUSE_EVENT_GROUPS is set to 1. Defaults to 1 if left undefined. */
#define configUSE_EVENT_GROUPS                      0



/******************************************************************************/
/* Stream Buffer related definitions. *****************************************/
/******************************************************************************/

/* Set configUSE_STREAM_BUFFERS to 1 to include stream buffer functionality in
 * the build. Set to 0 to exclude event group functionality from the build. The
 * FreeRTOS/source/stream_buffer.c source file must be included in the build if
 * configUSE_STREAM_BUFFERS is set to 1. Defaults to 1 if left undefined. */
#define configUSE_STREAM_BUFFERS                    0


/******************************************************************************/
/* Memory allocation related definitions. *************************************/
/******************************************************************************/
// 支持静态内存分配
#define configSUPPORT_STATIC_ALLOCATION             1

// 支持动态内存分配
#define configSUPPORT_DYNAMIC_ALLOCATION            1

// 系统总的堆大小
#define configTOTAL_HEAP_SIZE                    ((size_t)1024*1024)

/* Set configAPPLICATION_ALLOCATED_HEAP to 1 to have the application allocate
 * the array used as the FreeRTOS heap.  Set to 0 to have the linker allocate
 * the array used as the FreeRTOS heap.  Defaults to 0 if left undefined. */
#define configAPPLICATION_ALLOCATED_HEAP             0

/* Set configSTACK_ALLOCATION_FROM_SEPARATE_HEAP to 1 to have task stacks
 * allocated from somewhere other than the FreeRTOS heap.  This is useful if you
 * want to ensure stacks are held in fast memory.  Set to 0 to have task stacks
 * come from the standard FreeRTOS heap.  The application writer must provide
 * implementations for pvPortMallocStack() and vPortFreeStack() if set to 1.
 * Defaults to 0 if left undefined. */
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0

/* Set configENABLE_HEAP_PROTECTOR to 1 to enable bounds checking and
 * obfuscation to internal heap block pointers in heap_4.c and heap_5.c to help
 * catch pointer corruptions. Defaults to 0 if left undefined. */
#define configENABLE_HEAP_PROTECTOR                  0



/******************************************************************
            FreeRTOS与中断有关的配置选项                           
******************************************************************/
/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                         4 //__NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                         4
#endif
    
//中断最低优先级
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15
    
//系统可管理的最高中断优先级
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
    
// 内核时钟中断优先级(systick)
#define configKERNEL_INTERRUPT_PRIORITY             (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

// 系统中断优先级，假如串口中断优先级高于该优先级，那么串口中断就可以调用 from_isr 类接口，否则则不可以。
// (5 << (8 - 4)) = 5 << 4 = 80 (0x50) */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY        (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Another name for configMAX_SYSCALL_INTERRUPT_PRIORITY - the name used depends
 * on the FreeRTOS port. */
#define configMAX_API_CALL_INTERRUPT_PRIORITY       configMAX_SYSCALL_INTERRUPT_PRIORITY



/***************************************************************
             FreeRTOS与钩子函数有关的配置选项                   
 **************************************************************/
/* 置1：使用空闲钩子（Idle Hook类似于回调函数）；置0：忽略空闲钩子
 * 
 * 空闲任务钩子是一个函数，这个函数由用户来实现，
 * FreeRTOS规定了函数的名字和参数：void vApplicationIdleHook(void)，
 * 这个函数在每个空闲任务周期都会被调用
 * 对于已经删除的RTOS任务，空闲任务可以释放分配给它们的堆栈内存。
 * 因此必须保证空闲任务可以被CPU执行
 * 使用空闲钩子函数设置CPU进入省电模式是很常见的
 * 不可以调用会引起空闲任务阻塞的API函数
 */
#define configUSE_IDLE_HOOK                             0

/* 置1：使用时间片钩子（Tick Hook）；置0：忽略时间片钩子
 * 
 * 时间片钩子是一个函数，这个函数由用户来实现，
 * FreeRTOS规定了函数的名字和参数：void vApplicationTickHook(void )
 * 时间片中断可以周期性的调用
 * 函数必须非常短小，不能大量使用堆栈，
 * 不能调用以”FromISR" 或 "FROM_ISR”结尾的API函数
 */
#define configUSE_TICK_HOOK                             0
// 使用内存申请失败钩子函数
#define configUSE_MALLOC_FAILED_HOOK                    1

#define configUSE_DAEMON_TASK_STARTUP_HOOK              0

/* Set configUSE_SB_COMPLETED_CALLBACK to 1 to have send and receive completed
 * callbacks for each instance of a stream buffer or message buffer. When the
 * option is set to 1, APIs xStreamBufferCreateWithCallback() and
 * xStreamBufferCreateStaticWithCallback() (and likewise APIs for message
 * buffer) can be used to create a stream buffer or message buffer instance
 * with application provided callbacks. Defaults to 0 if left undefined. */
#define configUSE_SB_COMPLETED_CALLBACK                 0

/*
 * 大于0时启用堆栈溢出检测功能，如果使用此功能 
 * 用户必须提供一个栈溢出钩子函数，如果使用的话
 * 此值可以为1或者2，因为有两种栈溢出检测方法 */
#define configCHECK_FOR_STACK_OVERFLOW                  2


/********************************************************************
          FreeRTOS与运行时间和任务状态收集有关的配置选项   
**********************************************************************/
// 启用运行时间统计功能
#define configGENERATE_RUN_TIME_STATS                   0

// 启用可视化跟踪调试
#define configUSE_TRACE_FACILITY                        1

/* 与宏configUSE_TRACE_FACILITY同时为1时会编译下面3个函数
 * prvWriteNameToBuffer()
 * vTaskList(),
 * vTaskGetRunTimeStats()
*/
#define configUSE_STATS_FORMATTING_FUNCTIONS            1



/********************************************************************
                FreeRTOS与协程有关的配置选项                          
*********************************************************************/
// 启用协程，启用协程以后必须添加文件 croutine.c
#define configUSE_CO_ROUTINES                           0

// 协程的有效优先级数目
#define configMAX_CO_ROUTINE_PRIORITIES                 (2)




/******************************************************************************/
/* FreeRTOS MPU specific definitions. *****************************************/
/******************************************************************************/
/* If configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS is set to 1 then
 * the application writer can provide functions that execute in privileged mode.
 * See:
 * https://www.freertos.org/a00110.html#configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS
 * Defaults to 0 if left undefined.  Only used by the FreeRTOS Cortex-M MPU
 * ports, not the standard ARMv7-M Cortex-M port. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS    0

/* Set configTOTAL_MPU_REGIONS to the number of MPU regions implemented on your
 * target hardware.  Normally 8 or 16.  Only used by the FreeRTOS Cortex-M MPU
 * ports, not the standard ARMv7-M Cortex-M port.  Defaults to 8 if left
 * undefined. */
#define configTOTAL_MPU_REGIONS                                   8

/* configTEX_S_C_B_FLASH allows application writers to override the default
 * values for the for TEX, Shareable (S), Cacheable (C) and Bufferable (B) bits
 * for the MPU region covering Flash.  Defaults to 0x07UL (which means TEX=000,
 * S=1, C=1, B=1) if left undefined.  Only used by the FreeRTOS Cortex-M MPU
 * ports, not the standard ARMv7-M Cortex-M port. */
#define configTEX_S_C_B_FLASH                                     0x07UL

/* configTEX_S_C_B_SRAM allows application writers to override the default
 * values for the for TEX, Shareable (S), Cacheable (C) and Bufferable (B) bits
 * for the MPU region covering RAM. Defaults to 0x07UL (which means TEX=000,
 * S=1, C=1, B=1) if left undefined.  Only used by the FreeRTOS Cortex-M MPU
 * ports, not the standard ARMv7-M Cortex-M port. */
#define configTEX_S_C_B_SRAM                                      0x07UL

/* Set configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY to 0 to prevent any privilege
 * escalations originating from outside of the kernel code itself.  Set to 1 to
 * allow application tasks to raise privilege.  Defaults to 1 if left undefined.
 * Only used by the FreeRTOS Cortex-M MPU ports, not the standard ARMv7-M
 * Cortex-M port. */
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY               1

/* Set configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS to 1 to allow unprivileged
 * tasks enter critical sections (effectively mask interrupts). Set to 0 to
 * prevent unprivileged tasks entering critical sections.  Defaults to 1 if left
 * undefined.  Only used by the FreeRTOS Cortex-M MPU ports, not the standard
 * ARMv7-M Cortex-M port. */
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS                0

/* FreeRTOS Kernel version 10.6.0 introduced a new v2 MPU wrapper, namely
 * mpu_wrappers_v2.c. Set configUSE_MPU_WRAPPERS_V1 to 0 to use the new v2 MPU
 * wrapper. Set configUSE_MPU_WRAPPERS_V1 to 1 to use the old v1 MPU wrapper
 * (mpu_wrappers.c). Defaults to 0 if left undefined. */
#define configUSE_MPU_WRAPPERS_V1                                 0

/* When using the v2 MPU wrapper, set configPROTECTED_KERNEL_OBJECT_POOL_SIZE to
 * the total number of kernel objects, which includes tasks, queues, semaphores,
 * mutexes, event groups, timers, stream buffers and message buffers, in your
 * application. The application will not be able to have more than
 * configPROTECTED_KERNEL_OBJECT_POOL_SIZE kernel objects at any point of
 * time. */
#define configPROTECTED_KERNEL_OBJECT_POOL_SIZE                   10

/* When using the v2 MPU wrapper, set configSYSTEM_CALL_STACK_SIZE to the size
 * of the system call stack in words. Each task has a statically allocated
 * memory buffer of this size which is used as the stack to execute system
 * calls. For example, if configSYSTEM_CALL_STACK_SIZE is defined as 128 and
 * there are 10 tasks in the application, the total amount of memory used for
 * system call stacks is 128 * 10 = 1280 words. */
#define configSYSTEM_CALL_STACK_SIZE                              256

/* When using the v2 MPU wrapper, set configENABLE_ACCESS_CONTROL_LIST to 1 to
 * enable Access Control List (ACL) feature. When ACL is enabled, an
 * unprivileged task by default does not have access to any kernel object other
 * than itself. The application writer needs to explicitly grant the
 * unprivileged task access to the kernel objects it needs using the APIs
 * provided for the same. Defaults to 0 if left undefined. */
#define configENABLE_ACCESS_CONTROL_LIST                          1


/******************************************************************************/
/* SMP( Symmetric MultiProcessing ) Specific Configuration definitions. *******/
/******************************************************************************/
/* Set configNUMBER_OF_CORES to the number of available processor cores.
 * Defaults to 1 if left undefined. */
 #define configNUMBER_OF_CORES                                  1

/* When using SMP (i.e. configNUMBER_OF_CORES is greater than one), set
 * configRUN_MULTIPLE_PRIORITIES to 0 to allow multiple tasks to run
 * simultaneously only if they do not have equal priority, thereby maintaining
 * the paradigm of a lower priority task never running if a higher priority task
 * is able to run. If configRUN_MULTIPLE_PRIORITIES is set to 1, multiple tasks
 * with different priorities may run simultaneously - so a higher and lower
 * priority task may run on different cores at the same time. */
#define configRUN_MULTIPLE_PRIORITIES                           0

/* When using SMP (i.e. configNUMBER_OF_CORES is greater than one), set
 * configUSE_CORE_AFFINITY to 1 to enable core affinity feature. When core
 * affinity feature is enabled, the vTaskCoreAffinitySet and
 * vTaskCoreAffinityGet APIs can be used to set and retrieve which cores a task
 * can run on. If configUSE_CORE_AFFINITY is set to 0 then the FreeRTOS
 * scheduler is free to run any task on any available core. */
#define configUSE_CORE_AFFINITY                                 0

/* When using SMP with core affinity feature enabled, set
 * configTASK_DEFAULT_CORE_AFFINITY to change the default core affinity mask for
 * tasks created without an affinity mask specified. Setting the define to 1
 * would make such tasks run on core 0 and setting it to (1 <<
 * portGET_CORE_ID()) would make such tasks run on the current core. This config
 * value is useful, if swapping tasks between cores is not supported (e.g.
 * Tricore) or if legacy code should be controlled. Defaults to tskNO_AFFINITY
 * if left undefined. */
#define configTASK_DEFAULT_CORE_AFFINITY                        tskNO_AFFINITY

/* When using SMP (i.e. configNUMBER_OF_CORES is greater than one), if
 * configUSE_TASK_PREEMPTION_DISABLE is set to 1, individual tasks can be set to
 * either pre-emptive or co-operative mode using the vTaskPreemptionDisable and
 * vTaskPreemptionEnable APIs. */
#define configUSE_TASK_PREEMPTION_DISABLE                       0

/* When using SMP (i.e. configNUMBER_OF_CORES is greater than one), set
 * configUSE_PASSIVE_IDLE_HOOK to 1 to allow the application writer to use
 * the passive idle task hook to add background functionality without the
 * overhead of a separate task. Defaults to 0 if left undefined. */
#define configUSE_PASSIVE_IDLE_HOOK                             0

/* When using SMP (i.e. configNUMBER_OF_CORES is greater than one),
 * configTIMER_SERVICE_TASK_CORE_AFFINITY allows the application writer to set
 * the core affinity of the RTOS Daemon/Timer Service task. Defaults to
 * tskNO_AFFINITY if left undefined. */
#define configTIMER_SERVICE_TASK_CORE_AFFINITY                  tskNO_AFFINITY


/******************************************************************************/
/* ARMv8-M secure side port related definitions. ******************************/
/******************************************************************************/
/* secureconfigMAX_SECURE_CONTEXTS define the maximum number of tasks that can
 *  call into the secure side of an ARMv8-M chip.  Not used by any other ports.
 */
#define secureconfigMAX_SECURE_CONTEXTS        5

/* Defines the kernel provided implementation of
 * vApplicationGetIdleTaskMemory() and vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Idle task and Timer task
 * respectively. The application can provide it's own implementation of
 * vApplicationGetIdleTaskMemory() and vApplicationGetTimerTaskMemory() by
 * setting configKERNEL_PROVIDED_STATIC_MEMORY to 0 or leaving it undefined. */
#define configKERNEL_PROVIDED_STATIC_MEMORY    1


/******************************************************************************/
/* ARMv8-M port Specific Configuration definitions. ***************************/
/******************************************************************************/
/* Set configENABLE_TRUSTZONE to 1 when running FreeRTOS on the non-secure side
 * to enable the TrustZone support in FreeRTOS ARMv8-M ports which allows the
 * non-secure FreeRTOS tasks to call the (non-secure callable) functions
 * exported from secure side. */
#define configENABLE_TRUSTZONE              0

/* If the application writer does not want to use TrustZone, but the hardware
 * does not support disabling TrustZone then the entire application (including
 * the FreeRTOS scheduler) can run on the secure side without ever branching to
 * the non-secure side. To do that, in addition to setting
 * configENABLE_TRUSTZONE to 0, also set configRUN_FREERTOS_SECURE_ONLY to 1. */
#define configRUN_FREERTOS_SECURE_ONLY      0

/* Set configENABLE_MPU to 1 to enable the Memory Protection Unit (MPU), or 0
 * to leave the Memory Protection Unit disabled. */
#if defined(ARCH_ARM_CORTEX_MPU)
    #define configENABLE_MPU                1
#else
    #define configENABLE_MPU                0
#endif

/* Set configENABLE_FPU to 1 to enable the Floating Point Unit (FPU), or 0
 * to leave the Floating Point Unit disabled. */
#if defined(ARCH_ARM_CORTEX_FPU)
    #define configENABLE_FPU                1
#else
    #define configENABLE_FPU                0
#endif

/* Set configENABLE_MVE to 1 to enable the M-Profile Vector Extension (MVE)
 * support, or 0 to leave the MVE support disabled. This option is only
 * applicable to Cortex-M55 and Cortex-M85 ports as M-Profile Vector Extension
 * (MVE) is available only on these architectures. configENABLE_MVE must be left
 * undefined, or defined to 0 for the Cortex-M23,Cortex-M33 and Cortex-M35P
 * ports. */
#define configENABLE_MVE                    0


/******************************************************************************/
/* ARMv7-M and ARMv8-M port Specific Configuration definitions. ***************/
/******************************************************************************/
/* Set configCHECK_HANDLER_INSTALLATION to 1 to enable additional asserts to
 * verify that the application has correctly installed FreeRTOS interrupt
 * handlers.
 *
 * An application can install FreeRTOS interrupt handlers in one of the
 * following ways:
 *   1. Direct Routing  -  Install the functions vPortSVCHandler and
 * xPortPendSVHandler for SVC call and PendSV interrupts respectively.
 *   2. Indirect Routing - Install separate handlers for SVC call and PendSV
 *                         interrupts and route program control from those
 * handlers to vPortSVCHandler and xPortPendSVHandler functions. The
 * applications that use Indirect Routing must set
 * configCHECK_HANDLER_INSTALLATION to 0.
 *
 * Defaults to 1 if left undefined. */
#define configCHECK_HANDLER_INSTALLATION        1


/******************************************************************************/
/* Definitions that include or exclude functionality. *************************/
/******************************************************************************/
// 开启任务通知功能，默认开启
#define configUSE_TASK_NOTIFICATIONS           1
// 使用互斥信号量
#define configUSE_MUTEXES                      1
// 使用递归互斥信号量
#define configUSE_RECURSIVE_MUTEXES            0
// 为1时使用计数信号量
#define configUSE_COUNTING_SEMAPHORES          1
// 启用队列
#define configUSE_QUEUE_SETS                   0
// 启用任务标签
#define configUSE_APPLICATION_TASK_TAG         0


/* USE_POSIX_ERRNO enables the task global FreeRTOS_errno variable which will
 * contain the most recent error for that task. */
#define configUSE_POSIX_ERRNO                  0



/************************************************************
            FreeRTOS可选函数配置选项                         
************************************************************/
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_xQueueGetMutexHolder            1


/* CMSIS-RTOS V2 flags */
#define configUSE_OS2_THREAD_SUSPEND_RESUME     1
#define configUSE_OS2_THREAD_ENUMERATE          1
#define configUSE_OS2_EVENTFLAGS_FROM_ISR       1
#define configUSE_OS2_THREAD_FLAGS              1
#define configUSE_OS2_TIMER                     1
#define configUSE_OS2_MUTEX                     1


/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler                     SVC_Handler
#define xPortPendSVHandler                  PendSV_Handler

/* IMPORTANT: After 10.3.1 update, Systick_Handler comes from NVIC (if SYS timebase = systick), otherwise from cmsis_os2.c */
#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 1



#endif /* FREERTOS_CONFIG_H */

