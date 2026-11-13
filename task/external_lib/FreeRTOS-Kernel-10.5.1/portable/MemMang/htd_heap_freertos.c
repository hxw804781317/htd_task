#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif


#include "htd_os.h"


// @brief 申请内存
void *pvPortMalloc(size_t xWantedSize)
{
    void* pvReturn = htd_memheap_sram_malloc(xWantedSize, (void*)pvPortMalloc);
#if (configUSE_MALLOC_FAILED_HOOK == 1)
    if( pvReturn == NULL ) {
        extern void vApplicationMallocFailedHook(void);
        vApplicationMallocFailedHook();
    }
#endif
    return pvReturn;
}

// @brief 释放内存
void vPortFree(void * pv)
{
    htd_memheap_sram_free(pv);
}

// @brief 申请内存
void * pvPortCalloc(size_t xNum, size_t xSize)
{
    return htd_memheap_sram_calloc(xNum, xSize, (void*)pvPortCalloc);
}

// @brief 
size_t xPortGetFreeHeapSize(void)
{
    return 0;
}

// @brief 
size_t xPortGetMinimumEverFreeHeapSize(void)
{
    ASSERT(0);
    return 0;
}

// @brief 
void xPortResetHeapMinimumEverFreeHeapSize(void)
{
    ASSERT(0);
}

// @brief 
void vPortInitialiseBlocks(void)
{
    ASSERT(0);
}



