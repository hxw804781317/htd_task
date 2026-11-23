#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "lwip.arch"
#include "htd_common.h"
#include <lwip/sys.h>
#include <lwip/opt.h>
#include <lwip/stats.h>
#include <lwip/err.h>
#include <lwip/debug.h>
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/sio.h>
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <lwip/inet.h>
#include <lwip/timeouts.h>
#include <netif/etharp.h>
#include <arch/sys_arch.h>
#include "htd_network_thread.h"


//////////////////////////////////////////////////////////////////////
// @brief 系统初始化
void sys_init(void)
{
}

// @brief 获取当前系统时钟，单位毫秒
uint32_t sys_now(void)
{
    return htd_time_ms_get();
}

// @brief 获取系统节拍数， sys_ticks()
uint32_t sys_jiffies(void)
{
    return htd_time_ms_get();
}

// @brief 系统断言
void sys_arch_assert(const char *file, int line)
{
    extern void htd_assert_failed(const char* file, uint32_t line);
    htd_assert_failed(file, line);
}

// @brief 打开串口
sio_fd_t sio_open(u8_t devnum)
{
    return NULL;
}

// @brief 通过串口发送数据
void sio_send(u8_t c, sio_fd_t fd)
{

}

// @brief 尝试从串口读取数据
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
    return 0;
}

// @brief 从网卡阻塞读取数据
uint32_t sio_read(sio_fd_t fd, uint8_t *buf, uint32_t size)
{
    print_warning("sio_read");
    return 0;
}


#if (defined(PPPOS_SUPPORT) && PPPOS_SUPPORT)
// @brief 把 PPP 数据包通过网卡发送出去
uint32_t sio_write(sio_fd_t fd, uint8_t *buf, uint32_t size)
{
    print_warning("sio_write");
    return 0;
}

// @brief 强行打断 sio_read 的等待
void sio_read_abort(sio_fd_t fd)
{
    print_warning("read_abort\n");
}
#endif /*(defined(PPPOS_SUPPORT) && PPPOS_SUPPORT)*/

// @brief 输出调试信息
void ppp_trace(int level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    htd_console_fprintf(fmt, &args);
    va_end(args);
}

#if LWIP_VERSION_MAJOR >= 2 /* >= v2.x */
#ifdef LWIP_HOOK_IP4_ROUTE_SRC
struct netif *lwip_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src)
{
    struct netif *netif;
    if (src == NULL) {
        return NULL;
    }
    // 遍历所有网卡链路
    NETIF_FOREACH(netif) {
        if (netif_is_up(netif) && netif_is_link_up(netif)) {
            if (ip4_addr_cmp(src, netif_ip4_addr(netif))) {
                return netif;
            }
        }
    }
    return NULL;
}
#endif /* LWIP_HOOK_IP4_ROUTE_SRC */
#endif /*LWIP_VERSION_MAJOR >= 2 */



#if NO_SYS == 0
//////////////////////////////////////////////////////////////////////
int errno;

// @brief 创建线程
sys_thread_t sys_thread_new(const char    *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    return htd_thread_create(name, thread, arg, stacksize, prio, 1);;
}

// @brief 创建信号量
err_t sys_sem_new(sys_sem_t *sem, uint8_t count)
{
    char tname[16];
    static unsigned short counter = 0;
    htd_snprintf(tname, sizeof(tname), "lwip_sem_%d", counter);
    counter++;
    
    htd_sem_t* tmpsem = htd_sem_create(tname, count, count);
    if (tmpsem == NULL) {
        return ERR_MEM;
    } else {
        *sem = tmpsem;
        return ERR_OK;
    }
}

// @brief 销毁信号量
void sys_sem_free(sys_sem_t *sem)
{
    if (*sem) {
        htd_sem_destroy(*sem);
        *sem = NULL;
    }
}

// @brief 发送信号(信号量加1)
void sys_sem_signal(sys_sem_t *sem)
{
    htd_sem_give(*sem);
}

// @brief 等待信号量
uint32_t sys_arch_sem_wait(sys_sem_t *sem, uint32_t timeout_ms)
{
    uint32_t start_ms = htd_time_ms_get();
    if (timeout_ms == 0) {
        timeout_ms = 0xffffffff;
    }
    if (false == htd_sem_take(*sem, timeout_ms)) {
        return SYS_ARCH_TIMEOUT;
    }
    // 计算耗时时间，单位毫秒
    uint32_t diff_ms = htd_time_ms_diff(htd_time_ms_get(), start_ms);
    if (diff_ms == 0) {
        diff_ms = 1;
    }
    return diff_ms;
}

// @brief 判断信号量是否有效
#ifndef sys_sem_valid
int sys_sem_valid(sys_sem_t *sem)
{
    return ((*sem) == NULL ? false : true);
}
#endif /*sys_sem_valid*/

// @brief 将信号量配置为无效
#ifndef sys_sem_set_invalid
void sys_sem_set_invalid(sys_sem_t *sem)
{
    *sem = NULL;
}
#endif /*sys_sem_set_invalid*/

// @brief 创建互斥锁
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    char tname[16];
    static unsigned short counter = 0;
    htd_snprintf(tname, sizeof(tname), "lwip_mutex_%d", counter);
    counter++;

    sys_mutex_t tmpmutex = htd_mutex_lock_create(tname);
    if (tmpmutex == NULL) {
        return ERR_MEM;
    } else {
        *mutex = tmpmutex;
        return ERR_OK;
    }
}

// @brief 销毁锁
void sys_mutex_free(sys_mutex_t *mutex)
{
    htd_mutex_lock_destroy(*mutex);
    *mutex = NULL;
}

// @brief 加锁
void sys_mutex_lock(sys_mutex_t *mutex)
{
    htd_mutex_lock(*mutex);
}

// @brief 解锁
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    htd_mutex_unlock(*mutex);
}

// 判断互斥量是否有效
#ifndef sys_mutex_valid
int sys_mutex_valid(sys_mutex_t *mutex)
{
    return (*mutex == NULL) ? 0 : 1;
}
#endif

// @brief 将互斥量配置为无效
#ifndef sys_mutex_set_invalid
void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    *mutex = NULL;
}
#endif


// @brief 创建消息邮箱
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    char tname[16];
    static unsigned short counter = 0;
    htd_snprintf(tname, sizeof(tname), "lwip_mbox_%d", counter);
    counter ++;

    sys_mbox_t tmpmbox = htd_mq_create(tname, sizeof(void*), size);
    if (tmpmbox != NULL) {
        *mbox = tmpmbox;
        return ERR_OK;
    }
    return ERR_MEM;
}

// @brief 销毁消息邮箱
void sys_mbox_free(sys_mbox_t *mbox)
{
    htd_mq_destroy(*mbox);
    *mbox = NULL;
}

// @brief 发送消息，会等待
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    htd_mq_send_wait(*mbox, &msg, sizeof(void*), 0xffffffff);
}

// @brief 尝试发送，不等待
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if (0 == htd_mq_send(*mbox, &msg, sizeof(void*))) {
        return ERR_OK;
    }
    return ERR_MEM;
}

#if (LWIP_VERSION_MAJOR * 100 + LWIP_VERSION_MINOR) >= 201 /* >= v2.1.0 */
// @brief 从中断中发送消息
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    return htd_mq_send_fisr(*mbox, msg, sizeof(void*));
}
#endif /* (LWIP_VERSION_MAJOR * 100 + LWIP_VERSION_MINOR) >= 201 */

// @brief 接收消息
uint32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, uint32_t timeout)
{
    uint32_t start_ms = htd_time_ms_get();
    if (timeout == 0) {
        timeout = 0xffffffff;
    }
    
    int32_t ret = htd_mq_recv(*mbox, msg, sizeof(void*), timeout);
    if (ret != 0) {
        return SYS_ARCH_TIMEOUT;
    }
    
    // 计算耗时时间，单位毫秒
    uint32_t diff_ms = htd_time_ms_diff(htd_time_ms_get(), start_ms);
    if (diff_ms == 0) {
        diff_ms = 1;
    }
    return diff_ms;
}

// @brief 尝试接收消息，不等待
uint32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    int32_t ret = htd_mq_recv(*mbox, msg, sizeof(void*), 0);
    if (ret != 0) {
        return SYS_MBOX_EMPTY;
    }
    return ERR_OK;
}

#ifndef sys_mbox_valid
// @brief 消息邮箱是否有效
int sys_mbox_valid(sys_mbox_t *mbox)
{
    return (*mbox == NULL) ? 0 : 1;
}
#endif

#ifndef sys_mbox_set_invalid
// @brief 设置消息邮箱为无效
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    *mbox = NULL;
}
#endif

// @brief 进入保护区
sys_prot_t sys_arch_protect(void)
{
    htd_critical_enter();
    return 0;
}

// @brief 退出保护区
void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    htd_critical_exit();
}


// @brief 初始化完成回调
static void tcpip_init_done_callback(void *arg)
{
    htd_sem_give((htd_sem_t*)arg);
}

// @brief 系统初始化，外部调用
int32_t htd_lwip_init(void)
{
    static bool init_ok = false;
    if (init_ok) {
        print_debug("lwip system already init.\n");
        return 0;
    }
    
    /* set default netif to NULL */
    netif_default = NULL;
    
    htd_sem_t done_sem;
    int32_t rc = htd_sem_init(&done_sem, "done", 0, 1);
    if (rc != 0) {
        LWIP_ASSERT("Failed to create semaphore", 0);
        return -1;
    }
    
    // 初始化 tcpip
    tcpip_init(tcpip_init_done_callback, (void *)&done_sem);
    // 等待初始化完毕
    if (htd_sem_take(&done_sem, 10*1000) == false) {
        htd_sem_deinit(&done_sem);
        return -1;
    }
    htd_sem_deinit(&done_sem);
    print_debug("lwIP-%d.%d.%d initialized!", LWIP_VERSION_MAJOR, LWIP_VERSION_MINOR, LWIP_VERSION_REVISION);
    init_ok = true;
    return 0;
}
#endif /*NO_SYS == 0*/


