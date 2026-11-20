#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.gsensor"
#include "htd_common.h"
#include "htd_gsensor_dev.h"



//////////////////////////////////////////////////////////////////////
static const htd_gsensor_dev_t* m_dev_gsensor = NULL;

// @brief 函数注册
int32_t htd_gsensor_dev_register(const htd_gsensor_dev_t* p_dev)
{
    ASSERT(p_dev);
    if (NULL == m_dev_gsensor) {
        m_dev_gsensor = p_dev;
    }
    return 0;
}

// @brief 初始化
int32_t htd_gsensor_dev_init(void)
{
    if (m_dev_gsensor) {
        if (m_dev_gsensor->init) {
            uint8_t m_gsensor_chip_id = 0xff;
            if (0 != m_dev_gsensor->init(&m_gsensor_chip_id)) {
                print_error("gsensor dev init failed.");
                return -1;
            }
            print_debug("gsensor dev init success. chip_id=%d", m_gsensor_chip_id);
            return 0;
        }
        return -2;
    }
    return 0;
}

// @brief 去初始化(设备进入低功耗状态)
void htd_gsensor_dev_uninit(bool is_power_off)
{
    print_debug("gsensor dev uninit");
    if (m_dev_gsensor && m_dev_gsensor->uninit) {
        m_dev_gsensor->uninit(is_power_off);
    }
}

// @brief 获取设备名字
const char* htd_gsensor_dev_name_get(char* p_name)
{
    if (m_dev_gsensor) {
        if (p_name) {
            strcpy(p_name, m_dev_gsensor->chip_name);
        }
        return m_dev_gsensor->chip_name;
    }
    return NULL;
}

// @brief 获取缓冲最大的数据个数
uint8_t htd_gsensor_dev_fifo_max_count_get(void)
{
    if (m_dev_gsensor) {
        return m_dev_gsensor->fifo_max_size;
    }
    return 0;
}

// @brief 获取gsensor缓冲数据，返回数据长度，count输入数据个数，输出返回数据个数
uint32_t htd_gsensor_dev_fifo_data_get(gsensor_val_t *p_xyz, uint8_t *count)
{
    int32_t data_len = 0;
    
    ASSERT(p_xyz);
    ASSERT(count);
    if (m_dev_gsensor) {
        if (m_dev_gsensor->ctrl_handle) {
            data_len = (*count) * sizeof(gsensor_val_t);
            data_len = m_dev_gsensor->ctrl_handle(HTD_GSENSOR_CMD_FIFO_DATA_READ, p_xyz, data_len);
            *count = data_len / sizeof(gsensor_val_t);
        } 
        return data_len;
    }
    return 0;
}

