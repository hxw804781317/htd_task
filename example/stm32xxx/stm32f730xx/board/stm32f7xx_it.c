#define DEBUG_LVL 4
#define DEBUG_TAG              "it"
#include "stm32f7xx_it.h"
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

#define __get_LR() ((uint32_t)__current_sp()) 


/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    print_error("HardFault_Handler");
    print_debug("\t CFSR=0x%08x, HFSR=0x%08x", SCB->CFSR, SCB->HFSR);
    print_debug("\t BFAR=0x%08x, ", SCB->BFAR);
    print_debug("\t LR=%p", __get_LR());
    while (1){
        
        htd_delay_ms(3000);
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    while (1) {
        print_error("MemManage_Handler");
        htd_delay_ms(3000);
    }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    while (1) {
        print_error("BusFault_Handler");
        htd_delay_ms(3000);
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    while (1) {
        print_error("UsageFault_Handler");
        htd_delay_ms(3000);
    }
}



/**
  * @brief This function handles System service call via SWI instruction.
  */
#if !defined(BSP_USING_FREERTOS)
void SVC_Handler(void)
{

}
#endif

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
    
}


/**
  * @brief This function handles Pendable request for system service.
  */
#if !defined(BSP_USING_FREERTOS)
void PendSV_Handler(void)
{

}
#endif

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
    HAL_IncTick();
    
#if defined(BSP_USING_FREERTOS) && 1
    extern void xPortSysTickHandler(void);
    #if ((INCLUDE_xTaskGetSchedulerState == 1) && 1)
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED){
            xPortSysTickHandler();
        }
    #else
        xPortSysTickHandler();
    #endif  /* INCLUDE_xTaskGetSchedulerState */
#endif
}



