#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "9axis"
#include "htd_common.h"
#include "htd_9axis_dev.h"
#include "htd_9axis_process.h"


//////////////////////////////////////////////////////////////////////
static htd_9axis_indicate_handle_t m_indicate_handle_table[2];
static uint8_t m_9axis_init_flag = 0;
static uint32_t m_9axis_sample_interval_ms = 0;


// @brief 初始化
int32_t htd_9axis_init(void)
{
    if (m_9axis_init_flag != 0) {
        return 0;
    }
    
    memset(m_indicate_handle_table, 0, sizeof(m_indicate_handle_table));
    int32_t ret = htd_9axis_dev_init();
    // 设置采样间隔
    m_9axis_init_flag = 0x88;
    htd_9axis_sample_interval_set(HTD_9AXIS_READ_DATA_PERIOD);
    return ret;
}

// @brief 去初始化
void htd_9axis_uninit(bool is_power_off)
{
    if (m_9axis_init_flag == 0) {
        return;
    }
    htd_9axis_dev_uninit(is_power_off);
}

// @brief 注册9axis数据通知句柄
void htd_9axis_indicate_handle_register(htd_9axis_indicate_handle_t handle)
{
    ASSERT(m_9axis_init_flag != 0);
    ASSERT(handle);
    uint32_t i;
    for (i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == handle) {
            return;
        }
    }
    for (i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == NULL) {
            m_indicate_handle_table[i] = handle;
            return;
        }
    }
    // 失败，没有内存
    print_error("indicate handle register failed.");
    ASSERT(0);
}

// @brief 注销9axis数据通知句柄
void htd_9axis_indicate_handle_unregister(htd_9axis_indicate_handle_t handle)
{
    ASSERT(handle);
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == handle) {
            m_indicate_handle_table[i] = NULL;
        }
    }
}

// @brief 获取9axis 采样频率
uint32_t htd_9axis_sample_freq_get(void)
{
    return HTD_9AXIS_SENSOR_SAMPLE_FREQ;
}

// @brief 定时读取 9axis 数据
static void htd_9axis_timer_read_handle(void* arg)
{
    if (!m_9axis_init_flag) {
        return;
    }

    uint32_t param = (uint32_t)arg;
    if (param != 0xab0987ac) {
        htd_task_load_timer_function(true, m_9axis_sample_interval_ms, htd_9axis_timer_read_handle, (void*)0xab0987ac);
        return;
    }
    
    htd_9axis_val_t data[32];
    uint8_t data_count = ARRAY_COUNT(data);
    htd_9axis_dev_fifo_data_get(data, &data_count);
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i]) {
            m_indicate_handle_table[i](data, data_count);
        }
    }
}

// @brief 设置采样间隔
uint32_t htd_9axis_sample_interval_set(uint32_t time_ms)
{
    if (time_ms == 0) {
        time_ms = HTD_9AXIS_READ_DATA_PERIOD;
    }
    if (time_ms == m_9axis_sample_interval_ms) {
        return 0;
    }
    
    m_9axis_sample_interval_ms = time_ms;
    htd_task_load_function(htd_9axis_timer_read_handle, NULL);
    return 0;
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))

// @brief 显示六轴传感器数据
static void htd_9axis_data_print_handle(htd_9axis_val_t* p_data, uint32_t data_count)
{
    while (data_count--) {
        print_debug("ACC:(%5d,%5d,%5d), GYRO:(%5d,%5d,%5d)", p_data->ax, p_data->ay, p_data->az, p_data->gx, p_data->gy, p_data->gz);
        p_data++;
    }
}

// @brief 调试命令
static int htd_9axis_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s init/uninit", argv[0]);
        return 0;
    }
    
    if (0 == memcmp(argv[1], "init", 5)) {
        htd_9axis_init();
        htd_9axis_indicate_handle_register(htd_9axis_data_print_handle);
        
    } else if (0 == memcmp(argv[1], "uninit", 7)) {
        htd_9axis_indicate_handle_unregister(htd_9axis_data_print_handle);
        htd_9axis_uninit(true);
        
    } else {
        print_error("Usage: %s init/uninit", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(htd_9axis_cmd, 9axis);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

