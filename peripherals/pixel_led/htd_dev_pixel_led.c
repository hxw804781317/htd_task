#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "pled"
#include "htd_dev_pixel_led.h"
#include "htd_common.h"


//////////////////////////////////////////////////////////////////////
static const htd_pled_dev_t *m_pled_dev = NULL;
static htd_mutex_lock_t m_pled_lock;
#define pled_dev_lock_init()           do { bool ret = htd_mutex_lock_init(&m_pled_lock, "pled"); ASSERT(ret); }while(0)
#define pled_dev_lock()                do { htd_mutex_lock(&m_pled_lock);} while(0)
#define pled_dev_unlock()              do { htd_mutex_unlock(&m_pled_lock);} while(0)


// @brief 注册(只能调用一次)
int32_t htd_dev_pled_register(const htd_pled_dev_t* p_pled)
{
    ASSERT(m_pled_dev == NULL);
    ASSERT(p_pled);
    ASSERT(p_pled->ctrl_handle);
    m_pled_dev = p_pled;
    return 0;
}

// @brief 初始化
int32_t htd_dev_pled_init(void)
{
    ASSERT(m_pled_dev);
    pled_dev_lock_init();
    return m_pled_dev->ctrl_handle(PLED_CMD_OPEN, NULL, 0);
}

// @brief 去初始化
int32_t htd_dev_pled_uninit(void)
{
    ASSERT(m_pled_dev);

    pled_dev_lock();
    m_pled_dev->ctrl_handle(PLED_CMD_CLOSE, NULL, 0);
    pled_dev_unlock();
    return 0;
}

// @brief 设置像素灯的颜色
// @param index 第几个灯，多芯片时才有用
// @param count 像素灯个数，像素灯每次更新都是全部更新
int32_t htd_dev_pled_set(uint32_t index, const pled_color_t* p_led, uint32_t count)
{
    int32_t ret;
    ASSERT(m_pled_dev);
    pled_dev_lock();
    ret = m_pled_dev->ctrl_handle(PLED_CMD_COLOR_SET, (void*)p_led, count);
    pled_dev_unlock();
    return ret;
}

// @brief 展示已经挂载的pled信息
void htd_dev_pled_show(void)
{
#if defined(DEBUG_LVL)
    if (m_pled_dev == NULL) {
        print_warning("pled unregistered.");
        return;
    }
    pled_dev_lock();
    print_log("pled dev: %s", m_pled_dev->p_name);
    m_pled_dev->ctrl_handle(PLED_CMD_SHOW_INFO, NULL, 0);
    pled_dev_unlock();
#endif
}



//////////////////////////////////////////////////////////////////////
#if defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 1
#include "htd_console_process.h"


// @brief 调试命令
static int cmd_pled(int argc, char **argv)
{
    if (argc != 2) {
        print_error("Usage: %s show/test", argv[0]);
        return 0;
    }
    
    if (memcmp(argv[1], "show", 5) == 0) {
        htd_dev_pled_show();
    } else if (memcmp(argv[1], "test", 5) == 0) {
        //htd_dev_pled_init();
    
        pled_color_t color[2];
        color[0].r = 255;
        color[0].g = 0;
        color[0].b = 0;
        color[1].r = 0;
        color[1].g = 255;
        color[1].b = 0;
        htd_dev_pled_set(0, color, 2);
        print_debug("set color finish");
    } else if (memcmp(argv[1], "black", 6) == 0) {
        pled_color_t color[2];
        htd_memset(color, 0, sizeof(color));
        htd_dev_pled_set(0, color, 2);
        
    } else {
        print_error("Usage: %s show/test", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_pled, pled);

#endif /**/

