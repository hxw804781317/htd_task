#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.i2cm"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_dma.h"
#include "htd_drv_clock.h"
#include "htd_dev_i2cm.h"


#if defined(BSP_USING_I2CM1) || defined(BSP_USING_I2CM2) || defined(BSP_USING_I2CM3)
//////////////////////////////////////////////////////////////////////
enum {
#if defined(BSP_USING_I2CM1) && defined(I2C1)
    I2CM1_INDEX,
#endif /* BSP_USING_I2CM1 */
#if defined(BSP_USING_I2CM2) && defined(I2C2)
    I2CM2_INDEX,
#endif /* BSP_USING_I2CM2 */
#if defined(BSP_USING_I2CM3) && defined(I2C3)
    I2CM3_INDEX,
#endif /* BSP_USING_I2CM3 */
    I2CM_ENABLE_COUNT,
};

typedef struct {
    I2C_TypeDef *Instance;
    IRQn_Type evirq_type;
    IRQn_Type erirq_type;
    pin_num_t pin_sck;
    pin_num_t pin_sda;
    int32_t (*pin_af_handle)(pin_num_t pin_scl, pin_num_t pin_sda);
}stm_i2c_config_t;

typedef struct {
    I2C_HandleTypeDef handle;              // 这个变量必须放在最前面, 不能使用指针
    i2cm_handle_t* who;
    const stm_i2c_config_t* i2c_config;
    uint8_t reference;
    htd_mutex_lock_t *lock;
    htd_sem_t* sem;                       // stm32f103 i2c 不使用dma模式，使用中断模式
}i2cm_object_t;
static i2cm_object_t m_i2cm_object[I2CM_ENABLE_COUNT] = {0};


// @brief 打开
static int32_t drv_i2cm_open(i2cm_handle_t *p_handle)
{
    // 创建锁
    i2cm_object_t* p_object = (i2cm_object_t*)p_handle->p_dev->p_object;
    if (p_object->lock == NULL) {
        p_object->lock = htd_mutex_lock_create(p_handle->p_dev->p_name);
        ASSERT(p_object->lock);
    }

    // 判断是否已经打开
    htd_mutex_lock(p_object->lock);
    i2cm_handle_t* p_prev = p_object->who;
    const i2cm_user_config_t* p_user_config = p_handle->p_user_config;
    if (p_prev) {
        if (p_prev == p_handle) {
            print_error("i2c master is opened.");
            htd_mutex_unlock(p_object->lock);
            return -1;
        }
        p_object->reference++;
        ASSERT(p_prev->p_user_config->pin_sda == p_user_config->pin_sda);
        ASSERT(p_prev->p_user_config->pin_sck == p_user_config->pin_sck);
        htd_mutex_unlock(p_object->lock);
        return 0;
    }

    ASSERT(p_object->i2c_config);
    ASSERT(p_user_config->pin_sck == p_object->i2c_config->pin_sck);
    ASSERT(p_user_config->pin_sda == p_object->i2c_config->pin_sda);
    
    // 使能时钟
    drv_clock_i2c_enable(p_object->i2c_config->Instance, true);
    // 配置引脚
    p_object->i2c_config->pin_af_handle(p_user_config->pin_sck, p_user_config->pin_sda);
    // 配置
    I2C_HandleTypeDef *i2c_handle = &p_object->handle;
    memset(i2c_handle, 0, sizeof(I2C_HandleTypeDef));
    i2c_handle->Instance = p_object->i2c_config->Instance;
#if !defined(SOC_SERIES_STM32F0) && !defined(SOC_SERIES_STM32F3) && !defined(SOC_SERIES_STM32F7)
    i2c_handle->Init.ClockSpeed = p_user_config->freq_hz;
    i2c_handle->Init.DutyCycle = I2C_DUTYCYCLE_2;
#endif
    i2c_handle->Init.OwnAddress1 = 0;
    i2c_handle->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handle->Init.OwnAddress2 = 0;
    //i2c_handle->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c_handle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handle->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_DeInit(i2c_handle) != HAL_OK) {
        drv_clock_i2c_enable(p_object->i2c_config->Instance, false);
        htd_mutex_unlock(p_object->lock);
        return -1;
    }
    // 初始化 i2c
    if (HAL_I2C_Init(i2c_handle) != HAL_OK) {
        drv_clock_i2c_enable(p_object->i2c_config->Instance, false);
        htd_mutex_unlock(p_object->lock);
        return -1;
    }

    // 创建信号量
    if (p_object->sem == NULL) {
        p_object->sem = htd_sem_create(p_handle->p_dev->p_name, 0, 1);
        ASSERT(p_object->sem);
    }
    // 使能事件中断
    HAL_NVIC_SetPriority(p_object->i2c_config->evirq_type, 3, 5);
    HAL_NVIC_EnableIRQ(p_object->i2c_config->evirq_type);
    // 使能错误中断
    HAL_NVIC_SetPriority(p_object->i2c_config->erirq_type, 3, 5);
    HAL_NVIC_EnableIRQ(p_object->i2c_config->erirq_type);
    p_object->reference = 1;
    p_object->who = p_handle;
    htd_mutex_unlock(p_object->lock);
    print_debug("i2c init finish.");
    return 0;
}

// @brief 关闭
static int32_t drv_i2cm_close(i2cm_handle_t* p_handle)
{
    int32_t ret = ERROR_CODE_SUCCESS;
    i2cm_object_t* p_object = (i2cm_object_t*)p_handle->p_dev->p_object;
    htd_mutex_lock(p_object->lock);
    if (p_object->reference <= 1) {
        // 关闭中断
        HAL_NVIC_DisableIRQ(p_object->i2c_config->evirq_type);
        HAL_I2C_DeInit(&p_object->handle);
        drv_clock_i2c_enable(p_object->i2c_config->Instance, false);
        p_object->who = NULL;
        p_object->reference = 0;
        if (p_object->sem) {
            htd_sem_destroy(p_object->sem);
            p_object->sem = NULL;
        }
    } else {
        p_object->reference--;
        ret = -ERROR_CODE_COMPLETED;
    }
    htd_mutex_unlock(p_object->lock);
    return ret;
}

// @brief 错误
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    uint32_t err_code = HAL_I2C_GetError(hi2c);
    print_error("HAL_I2C_ErrorCallback. err_code=0x%08x", err_code);
    if (err_code == HAL_I2C_ERROR_BERR) {
        // 总线错误：通常意味着总线死锁，需要复位外设来释放 SDA/SCL 线
        // 注意：这里不能直接调用 Init，通常需要先 DeInit
        HAL_I2C_DeInit(hi2c); 
        htd_delay_ms(10);
        HAL_I2C_Init(hi2c); 
    } else if (err_code == HAL_I2C_ERROR_AF) {
        // 应答错误：从机没反应。
        // 这种情况很常见，不一定需要复位硬件，可能是地址错了或者从机忙。
        
    } else {
        // 其他错误 (超时, DMA错误等)
        
    }
}

// @brief 发送完成
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    //print_debug("HAL_I2C_MasterTxCpltCallback");
    i2cm_object_t* p_object = (i2cm_object_t*)hi2c;
    htd_sem_give_fisr(p_object->sem);
}

// @brief 接收完成
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    //print_debug("HAL_I2C_MasterRxCpltCallback");
    i2cm_object_t* p_object = (i2cm_object_t*)hi2c;
    htd_sem_give_fisr(p_object->sem);
}

// @brief 读数据
static int32_t drv_i2cm_xfer(i2cm_handle_t* p_handle, i2cm_msg_t* msg, uint32_t msg_count, uint32_t timeout_ms)
{
    bool is_first, is_last;
    uint32_t mode;
    int32_t ret;
    uint16_t slave_addr;
    i2cm_msg_t* start = msg;
    i2cm_object_t* p_object = (i2cm_object_t*)p_handle->p_dev->p_object;

    htd_mutex_lock(p_object->lock);
    while (msg_count--) {
        is_last = msg_count == 0;
        is_first = msg == start;
        if (is_first && is_last) {
            mode = I2C_FIRST_AND_LAST_FRAME;
        } else if (is_first) {
            mode = I2C_FIRST_AND_NEXT_FRAME;
        } else if (is_last) {
            mode = I2C_LAST_FRAME;
        } else {
            mode = I2C_NEXT_FRAME;
        }
        slave_addr = msg->slave_addr << 1;
        
        ASSERT(msg);
        ASSERT(msg->buff);
        ASSERT(msg->len > 0);
        print_debug("msg_count=%d, mode=%d", msg_count, mode);
        htd_sem_reset(p_object->sem);
        if (msg->flags & I2CM_MSG_FLAG_RX) {
            ret = HAL_I2C_Master_Seq_Receive_IT(&p_object->handle, slave_addr, msg->buff, msg->len, mode);
            if (ret != 0) {
                print_error("HAL_I2C_Master_Seq_Receive_IT failed. ret=%d", ret);
                break;
            }
        } else {
            ret = HAL_I2C_Master_Seq_Transmit_IT(&p_object->handle, slave_addr, msg->buff, msg->len, mode);
            if (ret != 0){
                print_error("HAL_I2C_Master_Seq_Transmit_IT failed. ret=%d", ret);
                break;
            }
        }
        // 等待收发结束
        if (false == htd_sem_take(p_object->sem, 10*1000)) {        // 等待上一个接收完毕
            print_error("waite read timeout.");
            break;
        }
        msg++;
    }
    htd_mutex_unlock(p_object->lock);
    return msg-start;
}

#if defined(BSP_USING_I2CM1) && defined(I2C1)
    const static stm_i2c_config_t m_i2c1_config = {
        .Instance = I2C1,
        .evirq_type = I2C1_EV_IRQn,  
        .erirq_type = I2C1_ER_IRQn,
        .pin_sck = PIN_NUM_GET(B, 6),
        .pin_sda = PIN_NUM_GET(B, 7),
        .pin_af_handle = drv_pin_af_i2c1,
    };
    void I2C1_EV_IRQHandler(void){HAL_I2C_EV_IRQHandler(&m_i2cm_object[I2CM1_INDEX].handle);}
    void I2C1_ER_IRQHandler(void){HAL_I2C_ER_IRQHandler(&m_i2cm_object[I2CM1_INDEX].handle);}
    HTD_I2CM_DEV_REGISTER(i2cm1, drv_i2cm_open, drv_i2cm_close, drv_i2cm_xfer, &m_i2cm_object[I2CM1_INDEX]);
#endif /* BSP_USING_I2CM1 */
#if defined(BSP_USING_I2CM2) && defined(I2C2)
    const static stm_i2c_config_t m_i2c2_config = {
        .Instance = I2C2,
        .evirq_type = I2C2_EV_IRQn,
        .erirq_type = I2C2_ER_IRQn,
        .pin_sck = PIN_NUM_GET(B, 3),
        .pin_sda = PIN_NUM_GET(B, 10),
        .pin_af_handle = drv_pin_af_i2c2,
    };
    void I2C2_EV_IRQHandler(void){HAL_I2C_EV_IRQHandler(&m_i2cm_object[I2CM2_INDEX].handle);}
    void I2C2_ER_IRQHandler(void){HAL_I2C_ER_IRQHandler(&m_i2cm_object[I2CM2_INDEX].handle);}
    HTD_I2CM_DEV_REGISTER(i2cm2, drv_i2cm_open, drv_i2cm_close, drv_i2cm_xfer, &m_i2cm_object[I2CM2_INDEX]);
#endif /* BSP_USING_I2CM2 */
#if defined(BSP_USING_I2CM3) && defined(I2C3)
    const static stm_i2c_config_t m_i2c3_config = {
        .Instance = I2C3,
        .evirq_type = I2C3_EV_IRQn,
        .erirq_type = I2C3_ER_IRQn,
        .pin_sck = PIN_NUM_GET(A, 8),
        .pin_sda = PIN_NUM_GET(B, 4),
        .pin_af_handle = drv_pin_af_i2c3,
    };
    void I2C3_EV_IRQHandler(void){HAL_I2C_EV_IRQHandler(&m_i2cm_object[I2CM3_INDEX].handle);}
    void I2C3_ER_IRQHandler(void){HAL_I2C_ER_IRQHandler(&m_i2cm_object[I2CM3_INDEX].handle);}
    HTD_I2CM_DEV_REGISTER(i2cm3, drv_i2cm_open, drv_i2cm_close, drv_i2cm_xfer, &m_i2cm_object[I2CM3_INDEX]);
#endif /* BSP_USING_I2CM3 */

// @brief 初始化
static int32_t drv_i2cm_init(void)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_i2cm_object); ++i) {
        memset(&m_i2cm_object[i], 0, sizeof(i2cm_object_t));

    #if defined(BSP_USING_I2CM1) && defined(I2C1)
        if (i == I2CM1_INDEX) {
            m_i2cm_object[i].i2c_config = &m_i2c1_config;
        }
    #endif
    #if defined(BSP_USING_I2CM2) && defined(I2C2)
        if (i == I2CM2_INDEX) {
            m_i2cm_object[i].i2c_config = &m_i2c2_config;
        }
    #endif
    #if defined(BSP_USING_I2CM3) && defined(I2C3)
        if (i == I2CM3_INDEX) {
            m_i2cm_object[i].i2c_config = &m_i2c3_config;
        }
    #endif
    }
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_i2cm_init);
#endif /* defined(BSP_USING_I2CM1) || defined(BSP_USING_I2CM2)*/


