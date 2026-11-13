#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.clock"
#include "htd_drv_common.h"
#include "htd_drv_clock.h"
#include "htd_device.h"


// @brief 使能dma时钟
void drv_clock_dma_enable(void* p_dma_intance, bool is_enable)
{
#if defined(DMA1) && defined(DMA1_Stream0)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)DMA1_Stream0 && (uint8_t*)p_dma_intance <= (uint8_t*)DMA1_Stream7) {
        static uint8_t dma1_clock_reference = 0;
        if (is_enable) {
            if (dma1_clock_reference == 0) {
                __HAL_RCC_DMA1_CLK_ENABLE();
                print_debug("dma1 clock enable");
            }
            dma1_clock_reference++;
        } else {
            if (dma1_clock_reference) {
                dma1_clock_reference--;
                if (dma1_clock_reference == 0) {
                    __HAL_RCC_DMA1_CLK_DISABLE();
                }
            }
        }
        return;
    }
#endif
#if defined(DMA1) && defined(DMA1_Channel1)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)DMA1_Channel1 && (uint8_t*)p_dma_intance <= (uint8_t*)DMA1_Channel_MAX) {
        static uint8_t dma1_clock_reference = 0;
        if (is_enable) {
            if (dma1_clock_reference == 0) {
                __HAL_RCC_DMA1_CLK_ENABLE();
                print_debug("dma1 clock enable");
            }
            dma1_clock_reference++;
        } else {
            if (dma1_clock_reference) {
                dma1_clock_reference--;
                if (dma1_clock_reference == 0) {
                    __HAL_RCC_DMA1_CLK_DISABLE();
                }
            }
        }
        return;
    }
#endif

#if defined(DMA2) && defined(DMA2_Stream0)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)DMA2_Stream0 && (uint8_t*)p_dma_intance <= (uint8_t*)DMA2_Stream7) {
        static uint8_t dma2_clock_reference = 0;
        if (is_enable) {
            if (dma2_clock_reference == 0) {
                __HAL_RCC_DMA2_CLK_ENABLE();
                print_debug("dma2 clock enable");
            }
            dma2_clock_reference++;
        } else {
            if (dma2_clock_reference) {
                dma2_clock_reference--;
                if (dma2_clock_reference == 0) {
                    __HAL_RCC_DMA2_CLK_DISABLE();
                }
            }
        }
        return;
    }
#endif
#if defined(DMA2) && defined(DMA2_Channel1)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)DMA2_Channel1 && (uint8_t*)p_dma_intance <= (uint8_t*)DMA2_Channel_MAX) {
        static uint8_t dma2_clock_reference = 0;
        if (is_enable) {
            if (dma2_clock_reference == 0) {
                __HAL_RCC_DMA2_CLK_ENABLE();
                print_debug("dma2 clock enable");
            }
            dma2_clock_reference++;
        } else {
            if (dma2_clock_reference) {
                dma2_clock_reference--;
                if (dma2_clock_reference == 0) {
                    __HAL_RCC_DMA2_CLK_DISABLE();
                }
            }
        }
        return;
    }
#endif

#if defined(BDMA)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)BDMA_Channel0 && (uint8_t*)p_dma_intance <= (uint8_t*)BDMA_Channel7) {
        static uint8_t bdma_clock_reference = 0;
        if (is_enable) {
            if (bdma_clock_reference == 0) {
                __HAL_RCC_BDMA_CLK_ENABLE();
            }
            bdma_clock_reference++;
        } else {
            if (bdma_clock_reference) {
                bdma_clock_reference--;
                if (bdma_clock_reference == 0) {
                    __HAL_RCC_BDMA_CLK_DISABLE();
                }
            }
        }
    }
#endif
#if defined(BDMA1)
    if ((uint8_t*)p_dma_intance >= (uint8_t*)BDMA1_Channel0 && (uint8_t*)p_dma_intance <= (uint8_t*)BDMA1_Channel7) {
        static uint8_t bdma1_clock_reference = 0;
        if (is_enable) {
            if (bdma1_clock_reference == 0) {
                __HAL_RCC_BDMA1_CLK_ENABLE();
            }
            bdma1_clock_reference++;
        } else {
            if (bdma1_clock_reference) {
                bdma1_clock_reference--;
                if (bdma1_clock_reference == 0) {
                    __HAL_RCC_BDMA1_CLK_DISABLE();
                }
            }
        }
    }
#endif
}

// @brief 使能串口时钟
void drv_clock_uart_enable(void* p_instance, bool is_enable)
{
#if defined(UART1)
    if (p_instance == UART1) {
        if (is_enable) {
            __HAL_RCC_UART1_CLK_ENABLE();
        } else {
            __HAL_RCC_UART1_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART2)
    if (p_instance == UART2) {
        if (is_enable) {
            __HAL_RCC_UART2_CLK_ENABLE();
        } else {
            __HAL_RCC_UART2_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART3)
    if (p_instance == UART3) {
        if (is_enable) {
            __HAL_RCC_UART3_CLK_ENABLE();
        } else {
            __HAL_RCC_UART3_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART4)
    if (p_instance == UART4) {
        if (is_enable) {
            __HAL_RCC_UART4_CLK_ENABLE();
        } else {
            __HAL_RCC_UART4_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART5)
    if (p_instance == UART5) {
        if (is_enable) {
            __HAL_RCC_UART5_CLK_ENABLE();
        } else {
            __HAL_RCC_UART5_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART6)
    if (p_instance == UART6) {
        if (is_enable) {
            __HAL_RCC_UART6_CLK_ENABLE();
        } else {
            __HAL_RCC_UART6_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART7)
    if (p_instance == UART7) {
        if (is_enable) {
            __HAL_RCC_UART7_CLK_ENABLE();
        } else {
            __HAL_RCC_UART7_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(UART8)
    if (p_instance == UART8) {
        if (is_enable) {
            __HAL_RCC_UART8_CLK_ENABLE();
        } else {
            __HAL_RCC_UART8_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(LPUART1)
    if (p_instance == LPUART1) {
        if (is_enable) {
            __HAL_RCC_LPUART1_CLK_ENABLE();
        } else {
            __HAL_RCC_LPUART1_CLK_DISABLE();
        }
        return;
    }
#endif
}

// @brief 使能 SPI 时钟
void drv_clock_spi_enable(void* p_instance, bool is_enable)
{
#if defined(SPI1)
    if (p_instance == SPI1) {
        if (is_enable) {
            __HAL_RCC_SPI1_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI1_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(SPI2)
    if (p_instance == SPI2) {
        if (is_enable) {
            __HAL_RCC_SPI2_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI2_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(SPI3)
    if (p_instance == SPI3) {
        if (is_enable) {
            __HAL_RCC_SPI3_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI3_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(SPI4)
    if (p_instance == SPI4) {
        if (is_enable) {
            __HAL_RCC_SPI4_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI4_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(SPI5)
    if (p_instance == SPI5) {
        if (is_enable) {
            __HAL_RCC_SPI5_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI5_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(SPI6)
    if (p_instance == SPI6) {
        if (is_enable) {
            __HAL_RCC_SPI6_CLK_ENABLE();
        } else {
            __HAL_RCC_SPI6_CLK_DISABLE();
        }
        return;
    }
#endif
}

// @brief 使能 i2c 时钟
void drv_clock_i2c_enable(void* p_instance, bool is_enable)
{
#if defined(I2C1)
    if (p_instance == I2C1) {
        if (is_enable) {
            __HAL_RCC_I2C1_CLK_ENABLE();
        } else {
            __HAL_RCC_I2C1_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(I2C2)
    if (p_instance == I2C2) {
        if (is_enable) {
            __HAL_RCC_I2C2_CLK_ENABLE();
        } else {
            __HAL_RCC_I2C2_CLK_DISABLE();
        }
        return;
    }
#endif
#if defined(I2C3)
    if (p_instance == I2C3) {
        if (is_enable) {
            __HAL_RCC_I2C3_CLK_ENABLE();
        } else {
            __HAL_RCC_I2C3_CLK_DISABLE();
        }
        return;
    }
#endif
}


