#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "net.lwip"
#include <htd_common.h>
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/tcpip.h>
#include <lwip/tcp.h>
#include <lwip/timeouts.h>
#include <lwip/etharp.h>
#include <netif/ethernet.h>
#include "htd_network_dev.h"
#include "htd_device.h"
#include "htd_network_thread.h"
#include "htd_lwip.h"


#if (defined(NO_SYS) && (NO_SYS == 1))
//////////////////////////////////////////////////////////////////////
static struct netif m_lwip_netif;


// @brief 网卡发送
static err_t low_level_output(struct netif *p_netif, struct pbuf *p)
{
    uint32_t buff_len = 0;
    uint8_t *buff = htd_malloc(p->tot_len);
    ASSERT(buff);
    
    // 拷贝 pbuf 数据
    buff_len = pbuf_copy_partial(p, buff, p->tot_len, 0);
    if (buff_len) {
        htd_network_dev_write(buff, buff_len);
        //print_hex(">>S", buff, buff_len);
    } else {
        print_error("pbuf copy partial failed. tot_len=%d", p->tot_len);
    }
    
    htd_free(buff);
    return ERR_OK;
}

// @brief 底层网卡读取数据
static void low_level_input(void* arg)
{
    struct netif *p_netif = (struct netif *)arg;
    uint32_t buff_len = 0;
    uint8_t* buff = htd_malloc(MAX_FRAMELEN + 4);
    ASSERT(buff);

    // 从enc28j60读取数据
    buff_len = htd_network_dev_read(buff, MAX_FRAMELEN);
    if (buff_len == 0) {
        htd_free(buff);
        return;
    }
    //print_hex("<<R", buff, buff_len);
    
    // 将数据转成 pbuf
    struct pbuf *p_pbuf = pbuf_alloc(PBUF_RAW, buff_len, PBUF_POOL);
    if (NULL == p_pbuf) {
        htd_free(buff);
        print_error("pbuf alloc failed.");
        return;
    }
    pbuf_take(p_pbuf, buff, buff_len);
    // 释放原有数据内存
    htd_free(buff);
    
    err_t err = p_netif->input(p_pbuf, p_netif); 
    if (err != ERR_OK) {
        pbuf_free(p_pbuf);
        print_error("ethernetif_input: IP input error. err=%d", err);
    } 
    
    // 接着读取, 尽量把底层数据全部读取掉
    network_thread_load_function(low_level_input, p_netif);
}

// @brief 网卡事件通告
static void network_evt_indicate_handle(net_dev_indicate_t indicate, void* p_data, uint32_t data_len)
{
    if (indicate == NET_DEV_INDICATE_DATA_RECV) {
        low_level_input(&m_lwip_netif);
    } else if (indicate == NET_DEV_INDICATE_PHY_LINK) {
        uint8_t is_link = UINT8_VAL(p_data);
        if (is_link) {
            netif_set_link_up(&m_lwip_netif);
            #if LWIP_DHCP
                dhcp_start(&m_lwip_netif);
            #endif /*LWIP_DHCP*/
        } else {
            netif_set_link_down(&m_lwip_netif);
            #if LWIP_DHCP
                dhcp_stop(&m_lwip_netif);
                dhcp_cleanup(&m_lwip_netif);
            #endif /*LWIP_DHCP*/
        }
    }
}

// @brief 初始化网卡
static err_t ethernetif_init(struct netif *p_netif)
{
    ASSERT(p_netif);

    print_debug("ethernetif init.");
    int32_t ret = htd_network_dev_open(NULL);
    if (ret != 0) {
        print_error("network dev open failed. ret=%d", ret);
        return ERR_IF;
    }
    htd_network_dev_ctrl(NET_DEV_CMD_INDICATE_HANDLE_SET, network_evt_indicate_handle, sizeof(void*));
    
#if LWIP_NETIF_HOSTNAME
    p_netif->hostname = htd_network_dev_name_get(NULL); 
#endif 
    p_netif->name[0] = 'e';
    p_netif->name[1] = 'n';
    p_netif->output = etharp_output;        //IP层发送数据包函数
    p_netif->linkoutput = low_level_output; //ARP模块发送数据包函数
    // 设置MAC地址长度,为6个字节
    p_netif->hwaddr_len = ETHARP_HWADDR_LEN; 
    htd_network_dev_ctrl(NET_DEV_CMD_MAC_ADDR_GET, p_netif->hwaddr, ETHARP_HWADDR_LEN);
    p_netif->mtu = 1500;    //最大允许传输单元,允许该网卡广播和ARP功能
    p_netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP|NETIF_FLAG_IGMP|NETIF_FLAG_ETHERNET;
    print_debug("ethernet mac: %02x%02x%02x%02x%02x%02x", 
        p_netif->hwaddr[0], p_netif->hwaddr[1], p_netif->hwaddr[2], p_netif->hwaddr[3], p_netif->hwaddr[4], p_netif->hwaddr[5]);
    return ERR_OK;
}

// @brief 状态回调
void netif_status_callback(struct netif *netif)
{
    print_warning("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

// @brief 
static void lwip_nosys_loop_handle(void* arg)
{
    sys_check_timeouts();
    uint32_t inter_ms = sys_timeouts_sleeptime();
    network_thread_load_timer_function(false, inter_ms, lwip_nosys_loop_handle, NULL);
    //print_debug("inter_ms=%d", inter_ms);
}

// @brief 启动句柄
static void lwip_start_handle(void* arg)
{
    //初始化LWIP内核
    lwip_init();

#if LWIP_DHCP
    //开启DHCP服务
    dhcp_start(&m_lwip_netif);
#endif /*LWIP_DHCP*/

    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
#if LWIP_DHCP
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
    IP4_ADDR(&gw, 0, 0, 0, 0);
#else
    IP4_ADDR(&ipaddr, 192, 168, 8, 10);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 8, 1); 
#endif
    struct netif *p_netif = netif_add(&m_lwip_netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);//向网卡列表中添加一个网口
    if (p_netif == NULL) {
        print_error("netif add failed.");
        return;
    } 
    
    // 设置状态回调函数
    netif_set_status_callback(p_netif, netif_status_callback);
    // 设置netif为默认网口
    netif_set_default(p_netif); 
    //打开netif网口
    netif_set_up(p_netif);
    print_debug("lwip init finish.");
    //
    network_thread_load_timer_function(false, 50, lwip_nosys_loop_handle, NULL);
}

// @brief 初始化
int32_t htd_lwip_init(void)
{
    network_thread_load_function(lwip_start_handle, NULL);
    return 0;
}

// @brief 去初始化
int32_t htd_lwip_uninit(void)
{
    // 关闭硬件设备
    htd_network_dev_close();
    
#if LWIP_DHCP
    dhcp_stop(&m_lwip_netif);
    dhcp_cleanup(&m_lwip_netif);
#endif /*LWIP_DHCP*/

    netif_remove(&m_lwip_netif);
    return 0;
}


//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)
#include "htd_console_process.h"


// @brief 调试命令
static int lwip_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s init/uninit");
        return 0;
    }
    if (memcmp(argv[1], "init", 5) == 0) {
        htd_lwip_init();
    } else if (memcmp(argv[1], "uninit", 7) == 0) {
        htd_lwip_uninit();
    }
    return 0;
}
CONSOLE_CMD_EXPORT(lwip_cmd, lwip);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)*/
#endif /*(defined(NO_SYS) && (NO_SYS == 1))*/

