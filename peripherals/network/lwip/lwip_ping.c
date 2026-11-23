#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "ping"
#include <lwip/opt.h>
#include <lwip/init.h>
#include <lwip/mem.h>
#include <lwip/icmp.h>
#include <lwip/netif.h>
#include <lwip/sys.h>
#include <lwip/inet.h>
#include <lwip/inet_chksum.h>
#include <lwip/ip.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/pbuf.h>
#include <lwip/raw.h>
#include <lwip/ip_addr.h>
#include <lwip/dns.h>
#include "htd_network_thread.h"
#include "htd_console_process.h"


#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1 == NO_SYS)
#define PING_ID             0xAFAF
#define PING_DATA_SIZE      32
#define PING_TIMEOUT_MS     1000 // 超时时间 1秒

typedef struct {
    char hostname[128];
    ip_addr_t target_ip;
    uint8_t seq;
    uint8_t is_pinging;
    uint8_t send_count;
    uint8_t lost_count;
    uint8_t send_total;
    uint32_t start_time_ms;
    struct raw_pcb *pcb;
} lwip_ping_t;
static lwip_ping_t* m_ping_cmd = NULL;


// @brief 发送 ICMP-Echo 请求
void ping_send(lwip_ping_t* p_ping, uint32_t ping_data_len) 
{
    ASSERT(ping_data_len > 0);
    if (p_ping == NULL || p_ping->is_pinging) {
        return;
    }

    if (p_ping->is_pinging) {
        uint32_t timeout_ms = sys_now() - p_ping->start_time_ms;
        if (timeout_ms >= 3000) {
            p_ping->is_pinging = 0;
            p_ping->lost_count++;
            print_debug("Request timed out.");
        } else {
            return;
        }
    }
    
    // 分配内存：ICMP头部 + 数据负载
    struct pbuf * p_pbuf = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr) + ping_data_len, PBUF_POOL);
    if (p_pbuf == NULL) {
        return;
    }

    // 填充 ICMP 头部
    struct icmp_echo_hdr * iecho = (struct icmp_echo_hdr*)p_pbuf->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->id = PP_HTONS(PING_ID);
    p_ping->seq++;
    iecho->seqno = PP_HTONS(p_ping->seq);
    iecho->chksum = 0;
    // 填充数据部分
    for (int i=0; i<ping_data_len; i++) {
        ((uint8_t *)iecho + sizeof(struct icmp_echo_hdr))[i] = (uint8_t)i;
    }
    iecho->chksum = inet_chksum(iecho, p_pbuf->tot_len);
    p_ping->start_time_ms = sys_now();
    p_ping->is_pinging = 1;
    
    // 发送数据包
    raw_sendto(p_ping->pcb, p_pbuf, &p_ping->target_ip);
    pbuf_free(p_pbuf);
    
    //print_debug("Ping request sent to %s, seq=%d", ipaddr_ntoa(&p_ping->target_ip), p_ping->seq);
}

// @brief 网卡发送
static void lwip_print_packet(struct pbuf *p_pbuf)
{
    uint32_t buff_len = 0;
    uint8_t *buff = htd_malloc(p_pbuf->tot_len);
    ASSERT(buff);
    buff_len = pbuf_copy_partial(p_pbuf, buff, p_pbuf->tot_len, 0);
    if (buff_len) {
        print_hex("ping recv", buff, buff_len);
    } 
    htd_free(buff);
}

// @brief 接收回调函数
static uint8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p_pbuf, const ip_addr_t *addr)
{
    // 确保数据包长度至少包含一个标准的 IP 头部
    lwip_ping_t* p_ping = m_ping_cmd;
    if (p_ping == NULL || p_pbuf->tot_len < IP_HLEN) {
        return 1;
    }

    struct ip_hdr *iphdr = (struct ip_hdr*)p_pbuf->payload;
    uint16_t ip_hdr_len = IPH_HL_BYTES(iphdr);
    if (p_pbuf->tot_len < ip_hdr_len + sizeof(struct icmp_echo_hdr)) {
        return 1;
    }

    // 获取 ICMP 头部, 校验：是否是 Echo Reply，且 ID 匹配
    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr*)((uint8_t*)p_pbuf->payload + ip_hdr_len);
    if (PP_NTOHS(iecho->id) == PING_ID && ICMPH_TYPE(iecho) == ICMP_ER) {
        uint32_t rtt = sys_now() - p_ping->start_time_ms;
        print_warning("reply from %s: bytes=%d, time_ms=%lu, TTL=%d, seq=%d", 
               ipaddr_ntoa(addr), p_pbuf->tot_len, rtt, IPH_TTL(iphdr), p_ping->seq);
        p_ping->is_pinging = 0; // 收到回复，标记空闲
        return 0;
    }
    
    print_warning("error. id=0x%04x, type=0x%02x", PP_NTOHS(iecho->id), ICMPH_TYPE(iecho));
    lwip_print_packet(p_pbuf);
    return 1;
}

// @brief 实现ping命令功能
static void ping_loop_handle(void* arg)
{
    lwip_ping_t* p_ping = m_ping_cmd;
    if (p_ping == NULL) {
        print_warning("ping stop");
        return;
    }

    if (p_ping->pcb == NULL) {
        print_debug("Pinging %s[%s] with 32 bytes of data:", p_ping->hostname, ipaddr_ntoa(&p_ping->target_ip));
        p_ping->pcb = raw_new(IP_PROTO_ICMP);
        if (p_ping->pcb) {
            raw_recv(p_ping->pcb, ping_recv, NULL);
        }
    }

    if (p_ping->send_count >= p_ping->send_total) {
        if (p_ping->pcb) {
            raw_remove(p_ping->pcb);
            p_ping->pcb = NULL;
        }
        htd_free(p_ping);
        m_ping_cmd = NULL;
        return;
    }

    p_ping->send_count++;
    ping_send(p_ping, 32);
    network_thread_load_timer_function(false, PING_TIMEOUT_MS, ping_loop_handle, arg);
}

// @brief DNS 回调
static void ping_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (m_ping_cmd == NULL) {
        return;
    }
    if (ipaddr != NULL) {
        // 解析成功，将 ip_addr_t 转换为字符串，然后启动 Ping
        m_ping_cmd->target_ip = *ipaddr;
        if (false == network_thread_load_function(ping_loop_handle, (void*)m_ping_cmd)) {
            print_error("ping start failed.");
            htd_free(m_ping_cmd);
            m_ping_cmd = NULL;
        }
    } else {
        print_debug("Ping request could not find host %s. Please check the name and try again.", m_ping_cmd->hostname);
    }
}

// @brief 调试命令
static int ping_cmd(int argc, char **argv)
{
    if (argc != 3) {
        print_warning("Usage: ping <host/address> <send count>");
        return 0;
    }
    
    if (m_ping_cmd) {
        print_error("is pinging. Please waite...");
        return 0;
    }
    
    uint32_t send_total = atoi(argv[2]);
    if (send_total == 0 || send_total > 255) {
        send_total = 10;
    }

    m_ping_cmd = (lwip_ping_t*)htd_malloc(sizeof(lwip_ping_t));
    ASSERT(m_ping_cmd);
    memset(m_ping_cmd, 0, sizeof(lwip_ping_t));
    strncpy(m_ping_cmd->hostname, argv[1], sizeof(m_ping_cmd->hostname));
    m_ping_cmd->send_total = send_total;

    int32_t ret = dns_gethostbyname(m_ping_cmd->hostname, &m_ping_cmd->target_ip, ping_dns_found_callback, NULL);
    if (ret == ERR_OK) {
        ping_dns_found_callback(m_ping_cmd->hostname, &m_ping_cmd->target_ip, NULL);
    } else if (ret == ERR_INPROGRESS) {
        print_debug("DNS request, wait...");
    } else {
        print_debug("Ping request could not find host %s. Please check the name and try again.", m_ping_cmd->hostname);
        htd_free(m_ping_cmd);
        m_ping_cmd = NULL;
    }
    return 0;
}
CONSOLE_CMD_EXPORT(ping_cmd, ping);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)*/

