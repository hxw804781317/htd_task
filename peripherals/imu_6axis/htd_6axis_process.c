#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "6axis"
#include "htd_common.h"
#include "htd_6axis_dev.h"
#include "htd_6axis_process.h"


//////////////////////////////////////////////////////////////////////
static htd_6axis_indicate_handle_t m_indicate_handle_table[2];
static uint8_t m_6axis_init_flag = 0;
static uint32_t m_6axis_sample_interval_ms = 0;


// @brief 初始化
int32_t htd_6axis_init(void)
{
    if (m_6axis_init_flag != 0) {
        return 0;
    }
    
    htd_memset(m_indicate_handle_table, 0, sizeof(m_indicate_handle_table));
    int32_t ret = htd_6axis_dev_init();
    if (ret != 0) {
        print_error("6axis init failed.");
        return ret;
    }
    
    // 设置采样间隔
    m_6axis_init_flag = 0x88;
    htd_6axis_sample_interval_set(HTD_6AXIS_READ_DATA_PERIOD);
    return ret;
}

// @brief 去初始化
void htd_6axis_uninit(bool is_power_off)
{
    if (m_6axis_init_flag == 0) {
        return;
    }
    m_6axis_init_flag = 0;
    htd_6axis_dev_uninit(is_power_off);
}

// @brief 注册6axis数据通知句柄
void htd_6axis_indicate_handle_register(htd_6axis_indicate_handle_t handle)
{
    ASSERT(m_6axis_init_flag != 0);
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

// @brief 注销6axis数据通知句柄
void htd_6axis_indicate_handle_unregister(htd_6axis_indicate_handle_t handle)
{
    ASSERT(handle);
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == handle) {
            m_indicate_handle_table[i] = NULL;
        }
    }
}

// @brief 获取6axis 采样频率
uint32_t htd_6axis_sample_freq_get(void)
{
    return HTD_6AXIS_SENSOR_SAMPLE_FREQ;
}

// @brief 定时读取 6axis 数据
static void htd_6axis_timer_read_handle(void* arg)
{
    if (!m_6axis_init_flag) {
        return;
    }

    uint32_t param = (uint32_t)arg;
    if (param != 0xab0987ac) {
        htd_task_load_timer_function(true, m_6axis_sample_interval_ms, htd_6axis_timer_read_handle, (void*)0xab0987ac);
        return;
    }

    htd_6axis_val_t data[32];
    uint8_t data_count = ARRAY_COUNT(data);
    uint32_t data_len = htd_6axis_dev_fifo_data_get(data, &data_count);
    if (data_len == 0) {
        print_warning("no data.");
    }
    
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i]) {
            m_indicate_handle_table[i](data, data_count);
        }
    }
}

// @brief 设置采样间隔
uint32_t htd_6axis_sample_interval_set(uint32_t time_ms)
{
    if (time_ms == 0) {
        time_ms = HTD_6AXIS_READ_DATA_PERIOD;
    }
    if (time_ms == m_6axis_sample_interval_ms) {
        return 0;
    }
    
    m_6axis_sample_interval_ms = time_ms;
    htd_task_load_function(htd_6axis_timer_read_handle, NULL);
    return 0;
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))
#include "htd_console_process.h"


// @brief 显示六轴传感器数据
static void htd_6axis_data_print_handle(htd_6axis_val_t* p_data, uint32_t data_count)
{
    while (data_count--) {
        print_debug("ACC:(%5d,%5d,%5d), GYRO:(%5d,%5d,%5d)", p_data->ax, p_data->ay, p_data->az, p_data->gx, p_data->gy, p_data->gz);
        p_data++;
    }
}

// @brief 调试命令
static int32_t imu_6axis_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s init/uninit", argv[0]);
        return 0;
    }
    
    if (0 == memcmp(argv[1], "init", 5)) {
        htd_6axis_init();
        htd_6axis_indicate_handle_register(htd_6axis_data_print_handle);
        
    } else if (0 == memcmp(argv[1], "uninit", 7)) {
        htd_6axis_indicate_handle_unregister(htd_6axis_data_print_handle);
        htd_6axis_uninit(true);
        
    } else {
        print_error("Usage: %s init/uninit", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(imu_6axis_cmd, imu_6axis);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))*/

