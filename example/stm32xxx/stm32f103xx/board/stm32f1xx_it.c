#define DEBUG_LVL 4
#define DEBUG_TAG              "it"
#include "stm32f1xx_it.h"
#include "app_board.h"

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    while (1){
        print_error("HardFault_Handler");
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    while (1) {
        print_error("MemManage_Handler");
    }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    while (1) {
        print_error("BusFault_Handler");
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    while (1) {
        print_error("UsageFault_Handler");
    }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
    
}

/**
  * @brief This function handles Pendable request for system service.
  */
#if !defined(HTD_TASK_FREERTOS)
void PendSV_Handler(void)
{

}
#endif

/**
  * @brief This function handles System service call via SWI instruction.
  */
#if !defined(HTD_TASK_FREERTOS)
void SVC_Handler(void)
{

}
#endif


/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
#if defined(HTD_TASK_FREERTOS) && 1
    extern void xPortSysTickHandler(void);
    #if (INCLUDE_xTaskGetSchedulerState  == 1)
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED){
            xPortSysTickHandler();
        }
    #else
        xPortSysTickHandler();
    #endif  /* INCLUDE_xTaskGetSchedulerState */
#endif

    HAL_IncTick();
}



