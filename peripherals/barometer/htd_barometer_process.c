// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "baro.filter"
#include "htd_common.h"
#include "htd_barometer_filter.h"
#include "htd_barometer_process.h"
#include "htd_barometer_dev.h"


//////////////////////////////////////////////////////////////////////
// 出厂校准值
typedef struct {
    uint32_t magic;
    int32_t inter_pa;
    uint32_t sea_level_pa;
} barometer_factory_calibrate_t;
#define BARO_FACTORY_MAGIC          0x9ad82332

typedef struct {
    barometer_factory_calibrate_t factory;
    barometer_filter_t pressure_filter;
    barometer_val_t pressure_filter_table[11];
} barometer_t;
static int32_t m_sea_level_pa = BAR_AIR_PRESSUR_DEFAULT_VALUE;
static uint32_t m_air_pressure_pa = BAR_AIR_PRESSUR_DEFAULT_VALUE;
static barometer_t *m_barometer = NULL;


// @brief 读 flash 的出厂校准值(weak函数，需要外部实现)
__WEAK int32_t htd_barometer_factory_flash_read(void* p_data, uint32_t data_len)
{
    barometer_factory_calibrate_t* p_calibrate = (barometer_factory_calibrate_t*)p_data;
    p_calibrate->magic = BARO_FACTORY_MAGIC;
    p_calibrate->inter_pa = 103;
    p_calibrate->sea_level_pa = BAR_AIR_PRESSUR_DEFAULT_VALUE;
    return sizeof(barometer_factory_calibrate_t);
}

// @brief 写入出厂校准值(weak函数，需要外部实现)
__WEAK int32_t htd_barometer_factory_flash_write(const void* p_data, uint32_t data_len)
{
    return data_len;
}

// @brief 定时检测句柄
static void barometer_detect_handle(void* arg)
{
    barometer_t* p_barometer = m_barometer;
    if (p_barometer == NULL) {
        return;
    }

    // 获取计算气压值
    uint32_t pressure_pa;
    if (0 == htd_barometer_dev_read(&pressure_pa)) {
        barometer_filter_add(&p_barometer->pressure_filter, pressure_pa);
    }
    uint32_t count = barometer_filter_data_count_get(&p_barometer->pressure_filter);
    if (count > 0) {
        pressure_pa = barometer_filter_get(&p_barometer->pressure_filter);
        pressure_pa += p_barometer->factory.inter_pa;
        m_air_pressure_pa = pressure_pa;
    }

    // 定时读取
    uint32_t timeout_ms = 500;
    if (count < ARRAY_COUNT(p_barometer->pressure_filter_table)) {
        timeout_ms = 50;
    }
    htd_task_load_timer_function(false, timeout_ms, barometer_detect_handle, NULL);
}

// @brief 开始计量
int32_t htd_barometer_start(void)
{
    if (m_barometer != NULL) {
        return 0;
    }

    // 初始化设备
    int32_t err_code = htd_barometer_dev_init();
    ASSERT(err_code == 0);

    // 初始化数据
    m_barometer = htd_malloc(sizeof(barometer_t));
    ASSERT(m_barometer);
    memset(m_barometer, 0, sizeof(barometer_t));
    barometer_filter_init(&m_barometer->pressure_filter, 
        m_barometer->pressure_filter_table, ARRAY_COUNT(m_barometer->pressure_filter_table));
    htd_barometer_factory_flash_read(&m_barometer->factory, sizeof(m_barometer->factory));
    
    // 开启定时检测
    htd_task_load_function(barometer_detect_handle, m_barometer);
    return 0;
}

// @brief 停止
int32_t htd_barometer_stop(void)
{
    if (m_barometer == NULL) {
        return 0;
    } 
    htd_barometer_dev_uninit(true);
    htd_free(m_barometer);
    m_barometer = NULL;
    return 0;
}

// @brief 读取当前气压值，单位pa
uint32_t htd_barometer_pressure_get(void)
{
    return m_air_pressure_pa;
}

// @brief 读取当前海拔高度
int32_t htd_barometer_altitude_get(void)
{
    float pressure_pa_f = (float)htd_barometer_pressure_get();
    float sea_level_pa_f = (float)m_sea_level_pa;
    // 国际大气模型公式
    // H = 44330 * (1 - (P / P0)^(1/5.255))
    float altitude = 100 * 44330.0f * (1.0f - powf(pressure_pa_f / sea_level_pa_f, 1.0f / 5.255f));
    return (int32_t)altitude;
}

// @brief 出厂时，设置当前气压值
int32_t htd_barometer_factory_calibrate_set(uint32_t pressure_pa, uint32_t sea_level_pa)
{
    if (!BAR_AIR_PRESSURE_IS_VALID(pressure_pa) || !BAR_AIR_PRESSURE_IS_VALID(sea_level_pa)) {
        print_error("invalid pressure value. pressure_pa=%u, sea_level_pa=%u", pressure_pa, sea_level_pa);
        return -1;
    }
    htd_barometer_stop();

    // 初始化设备
    int32_t err_code = htd_barometer_dev_init();
    ASSERT(err_code == 0);

    // 初始化数据
    barometer_t *p_barometer = htd_malloc(sizeof(barometer_t));
    ASSERT(p_barometer);
    memset(p_barometer, 0, sizeof(barometer_t));
    barometer_filter_init(&p_barometer->pressure_filter, 
        p_barometer->pressure_filter_table, ARRAY_COUNT(p_barometer->pressure_filter_table));
    htd_barometer_factory_flash_read(&p_barometer->factory, sizeof(m_barometer->factory));

    // 读取数据并过滤
    uint32_t current_pa;
    for (;;) {
        if (0 == htd_barometer_dev_read(&current_pa)) {
            barometer_filter_add(&p_barometer->pressure_filter, current_pa);
        }
        if (barometer_filter_data_count_get(&p_barometer->pressure_filter) > ARRAY_COUNT(p_barometer->pressure_filter_table)) {
            break;
        }
    }
    current_pa = barometer_filter_get(&p_barometer->pressure_filter);
    htd_barometer_dev_uninit(true);
    htd_free(p_barometer);

    // 保存参考数据
    barometer_factory_calibrate_t factory;
    factory.magic = BARO_FACTORY_MAGIC;
    factory.inter_pa = (int32_t)(pressure_pa - current_pa);
    factory.sea_level_pa = sea_level_pa;
    m_sea_level_pa = sea_level_pa;
    htd_barometer_factory_flash_write(&factory, sizeof(barometer_factory_calibrate_t));
    return 0;
}

// @brief 设置当前海平面气压值
int32_t htd_barometer_sea_leval_pressure_set(int32_t sea_level_pa)
{
    // 数值是否有效
    if (!BAR_AIR_PRESSURE_IS_VALID(sea_level_pa)) {
        print_error("invalid pressure value. sea_level_pa=%u", sea_level_pa);
        return -1;
    }

    // 大气压值
    if (m_sea_level_pa == sea_level_pa) {
        return 0;
    }
    m_sea_level_pa = sea_level_pa;

    // 保存参考数据
    barometer_factory_calibrate_t factory;
    htd_barometer_factory_flash_read(&factory, sizeof(barometer_factory_calibrate_t));
    if (factory.magic != BARO_FACTORY_MAGIC) {
        memset(&factory, 0, sizeof(barometer_factory_calibrate_t));
    }
    factory.magic = BARO_FACTORY_MAGIC;
    factory.sea_level_pa = sea_level_pa;
    htd_barometer_factory_flash_write(&factory, sizeof(barometer_factory_calibrate_t));
    return 0;
}

