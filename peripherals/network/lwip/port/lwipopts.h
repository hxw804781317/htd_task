#ifndef _LWIPOPTS_H_
#define _LWIPOPTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

#include <htd_common.h>
#include "htd_dev_rng.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#ifndef LWIP_RAND
    #define LWIP_RAND()                 htd_rng_val_get()
#endif /*LWIP_RAND*/
// 配置内存对齐长度，默认是4字节
#ifdef HTD_LWIP_MEM_ALIGNMENT
    #define MEM_ALIGNMENT               HTD_LWIP_MEM_ALIGNMENT
#else
    #define MEM_ALIGNMENT               4
#endif
// 未查找到DNS服务器IP时，返回的错误码
#ifndef ENSRNOTFOUND
    #define ENSRNOTFOUND                163     // 0xA3
#endif
// 是否使用 stddef.h 头文件，不包含为1，包含则为0
#define LWIP_NO_STDDEF_H                0
// 是否使用 stdint.h 头文件，不包含为1，包含则为0
#define LWIP_NO_STDINT_H                0
// 是否使用 inttypes.h 头文件，不包含为1，包含则为0
#define LWIP_NO_INTTYPES_H              0

#ifndef BSP_USING_RTTHREAD
    // 是否使用 unistd.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_UNISTD_H            1
    // 是否使用 limits.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_LIMITS_H            1
    // 是否使用 ctype.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_CTYPE_H             1
    // 用于控制 LwIP 是否定义自己的 struct timeval 结构体
    #define LWIP_TIMEVAL_PRIVATE        1
#else
    // 是否使用 unistd.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_UNISTD_H            0
    // 是否使用 limits.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_LIMITS_H            0
    // 是否使用 ctype.h 头文件，不包含为1，包含则为0
    #define LWIP_NO_CTYPE_H             0
    // 用于控制 LwIP 是否定义自己的 struct timeval 结构体
    #define LWIP_TIMEVAL_PRIVATE        0
#endif

// 是否使用硬件原生的字节序转换指令（如 htonl, htons）来代替 LwIP 自带的软件算法
#define LWIP_PLATFORM_BYTESWAP          0
#if LWIP_PLATFORM_BYTESWAP
    #if (defined ( __CC_ARM ) || defined(__CLANG_ARM))
        #define LWIP_PLATFORM_HTONS(x)  ((uint16_t)__REV16(x))
        #define LWIP_PLATFORM_HTONL(x)  ((uint32_t)__REV(x))
    #else
        #define LWIP_PLATFORM_HTONS(x)  (__builtin_bswap16(x))
        #define LWIP_PLATFORM_HTONL(x)  (__builtin_bswap32(x))
    #endif
#endif 







//////////////////////////////////////////////////////////////////////
//NO_SYS==1:不使用操作系统
#if defined(BSP_USING_NONOS)
    #define NO_SYS                      1
    // 用于保护内存堆（Heap）分配和释放操作的“轻量级锁”开关
    #define SYS_LIGHTWEIGHT_PROT        0
    // 定义系统定时器
    #define LWIP_TIMERS                 1
    // 配置系统定时器的数量
    //#define MEMP_NUM_SYS_TIMEOUT       (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + PPP_SUPPORT + (LWIP_IPV6 ? (1 + (2*LWIP_IPV6)) : 0))
    #define LWIP_DEBUG_TIMERNAMES       0
    #define NO_SYS_NO_TIMERS            0
#else
    #define NO_SYS                      0
    // 用于保护内存堆（Heap）分配和释放操作的“轻量级锁”开关
    #define SYS_LIGHTWEIGHT_PROT        1
    // 错误码
    #define LWIP_PROVIDE_ERRNO          1
#endif 

// 调试打印, 关掉直接注释掉
#define LWIP_DEBUG                      1
#define LWIP_DBG_TYPES_ON               (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT)
//#define DHCP_DEBUG                      LWIP_DBG_ON
//#define ETHARP_DEBUG                    LWIP_DBG_ON
//#define ARP_DEBUG                       LWIP_DBG_ON
//#define MEM_DEBUG                       LWIP_DBG_ON
//#define PBUF_DEBUG                      LWIP_DBG_ON


// 内存管理方式：0为静态内存, 1为动态内存
#define MEMP_MEM_MALLOC                 0
#if MEMP_MEM_MALLOC
    #define MEM_USE_POOLS               0
#endif

#define xMEM_CUSTOM_ALLOCATOR           1
#if defined(MEM_CUSTOM_ALLOCATOR) && MEM_CUSTOM_ALLOCATOR
    #define MEM_CUSTOM_FREE             htd_free
    #define MEM_CUSTOM_MALLOC           htd_malloc
    #define MEM_CUSTOM_CALLOC           htd_calloc
#endif

// 是否使用标准C库实现malloc/free, 为0表示否, 使用LWIP内置内存管理方式
#define MEM_LIBC_MALLOC                 0
#if MEM_LIBC_MALLOC == 0
    // 使用LWIP内置内存管理方式时, 分配的内存大小
    #define MEM_SIZE                    (1024*12)
#endif

// 内存越界检测方式，可以配置为0,1,2
#define MEMP_OVERFLOW_CHECK             0
// 检查空闲链表是否形成了死循环
#define MEMP_SANITY_CHECK               0
// 控制是否允许在中断服务函数(ISR)或其他线程中直接释放内存
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0

// 发送固定数据包包头数量(PBUF_ROM 或 PBUF_REF)，减少内存拷贝
// 比如web服务器，返回rom里面固定的数据，这时候就会用到一个包头数量，而不用把rom数据拷贝到内存中再发送
#define MEMP_NUM_PBUF                   16

// 控制接收的网络数据包缓冲个数，如果应用层没有及时处理，可能导致丢包
#define MEMP_NUM_NETBUF                 8

// PBUF_POOL 类型数据包缓冲区的最大数量, 同时能处理多少个网络数据包
// IP_REASS_MAX_PBUFS < PBUF_POOL_SIZE
#define PBUF_POOL_SIZE                  16
// 一次能接收多大的数据包
#define PBUF_POOL_BUFSIZE               1514


// RAW PCB（Protocol Control Block，协议控制块） 的最大数量, 设备最多能同时开启多少个“原始套接字”连接
#ifdef HTD_LWIP_RAW_PCB_NUM
    #define MEMP_NUM_RAW_PCB            HTD_LWIP_RAW_PCB_NUM
#endif
// UDP PCB（Protocol Control Block，协议控制块） 的最大数量, 设备最多能同时建立多少个 UDP 连接
#ifdef HTD_LWIP_UDP_PCB_NUM
    #define MEMP_NUM_UDP_PCB            HTD_LWIP_UDP_PCB_NUM
#endif









// IP 相关定义
//////////////////////////////////////////////////////////////////////
#define LWIP_IPV4                       1
#ifdef HTD_USING_LWIP_IPV6
    #define LWIP_IPV6                   1
#else
    #define LWIP_IPV6                   0
#endif /* HTD_USINGLWIP_IPV6 */

// 是否开启 IP 包转发功能
#define IP_FORWARD                      0
// 协议栈中开启网络地址与端口转换功能
// 连接在你设备局域网（LAN）侧的其他设备，能够通过你设备的广域网（WAN）接口共享上网, 该功能依赖 IP_FORWARD
#define IP_NAT                          IP_FORWARD
// 是否开启 IP 分片重组功能(接收); 能否接收并拼合“被切碎”的大数据包
#define IP_REASSEMBLY                   0
// 是否开启 IP 分片重组功能(发送); 能否发送并拼合“被切碎”的大数据包
#define IP_FRAG                         0

#if IP_REASSEMBLY
    // 单个拼接分片的最大数量
    #define IP_REASS_MAX_PBUFS          10
    // 同时能接受的拼接数量
    #define MEMP_NUM_REASSDATA          4
#endif




// SOCKET 相关定义
//////////////////////////////////////////////////////////////////////
// 使能 socket api 接口
#if NO_SYS
    // 如果没有使用操作系统，不能使用 socket 接口，否则无法编译通过
    #define LWIP_SOCKET                 0
#else
    #define LWIP_SOCKET                 1
#endif
// 网络连接对象的最大数量
#define MEMP_NUM_NETCONN                8
#if LWIP_SOCKET
    // 网络连接中间层，socket 接口依赖于 LWIP_NETCONN
    #define LWIP_NETCONN                1
    // 是否支持 socket select
    #define LWIP_SOCKET_SELECT          0
    // 是否支持 socket poll
    #define LWIP_SOCKET_POLL            0
#else
    #define LWIP_NETCONN                0
#endif

// 是否支持环回接口，开启它，你的设备才能访问 127.0.0.1 这个特殊IP；自己给自己发送数据
#define LWIP_HAVE_LOOPIF                0





// 应用协议相关定义
//////////////////////////////////////////////////////////////////////
// DNS，域名解析协议，依赖UDP
#ifdef HTD_LWIP_DNS
    #define LWIP_DNS                    1
#else
    #define LWIP_DNS                    0
#endif
// ARP，地址解析协议，必须开启
#ifndef LWIP_ARP
    #define LWIP_ARP                    1
    #define ARP_TABLE_SIZE              10
    #define ARP_QUEUEING                1
    #define ETHARP_SUPPORT_STATIC_ENTRIES 1
#endif
// DHCP 动态主机配置协议，依赖UDP
#ifdef HTD_LWIP_IP_ADDR_DHCP
    #define LWIP_DHCP                   1
    // 控制 DHCP 客户端在获取到 IP 地址后，是否进行 ARP 冲突检测, 起到防止 IP 地址冲突的作用
    #define DHCP_DOES_ARP_CHECK         1
    // 当 DHCP 自动获取 IP 失败时，设备会自动给自己分配一个 169.254.x.x 开头的 IP 地址，确保设备至少能进行局域网通信
    #define LWIP_AUTOIP                 0
    #define LWIP_DHCP_AUTOIP_COOP       (LWIP_AUTOIP)
#else
    #define LWIP_DHCP                   0
#endif


// IGMP 互联网组管理协议(即 IP 组播/多播)，这个宏必须开启; 依赖UDP
#ifdef HTD_LWIP_IGMP
    #define LWIP_IGMP                   1
#else
    #define LWIP_IGMP                   0
#endif


// ICMP，互联网控制报文协议，ping 协议底层实现
#ifdef HTD_LWIP_ICMP
    #define LWIP_ICMP                   1
    #define ICMP_TTL                    255
#else
    #define LWIP_ICMP                   0
#endif


// SNMP
#ifdef HTD_LWIP_SNMP
    #define LWIP_SNMP                   1
#else
    #define LWIP_SNMP                   0
#endif


// #define MEMP_NUM_TCPIP_MSG_API      16
// #define MEMP_NUM_TCPIP_MSG_INPKT    16


// [ 头部预留 (ETH_PAD_SIZE) ] [ 以太网头 (14字节) ] [ IP头 ] [ TCP/UDP头 ] [ 数据 ]
// 由于以太网头是14字节，不是4字节对齐，部分老旧的网卡芯片要求按4字节对齐，这时候就需要配置 ETH_PAD_SIZE
#define ETH_PAD_SIZE                    0






// RAW 相关定义
//////////////////////////////////////////////////////////////////////
#ifdef HTD_LWIP_RAW
    #define LWIP_RAW                    1
    #define DEFAULT_RAW_RECVMBOX_SIZE   1
    #define DEFAULT_ACCEPTMBOX_SIZE     10
#else
    #define LWIP_RAW                    0
#endif



// UDP 相关定义
//////////////////////////////////////////////////////////////////////
#if LWIP_IGMP || LWIP_DNS || LWIP_DHCP
    #define LWIP_UDP                    1
    #define LWIP_UDPLITE                0
    #define UDP_TTL                     255
    #define DEFAULT_UDP_RECVMBOX_SIZE   1
#else
    #define LWIP_UDP                    0
#endif




// TCP 相关定义
//////////////////////////////////////////////////////////////////////
#ifdef HTD_LWIP_TCP
    #define LWIP_TCP                    1
#else
    #define LWIP_TCP                    0
#endif

// TCP PCB（Protocol Control Block，协议控制块） 的最大数量, 设备最多能同时建立多少个 TCP 连接
#ifdef HTD_LWIP_TCP_PCB_NUM
    #define MEMP_NUM_TCP_PCB            HTD_LWIP_TCP_PCB_NUM
#endif
// TCP 的TTL
#define TCP_TTL                         255
// 是否开启乱序报文段排队功能；它决定了当网络包“不按顺序到达”时，LwIP 是选择“先存起来”还是“直接扔掉”。
#define TCP_QUEUE_OOSEQ                 1
// 用于定义 TCP 协议栈中最大报文段长度, 以太网的标准 MTU 通常是 1500 字节, 减去TCP头部20字节和IP头部20字节
#define TCP_MSS                         1460
// 宏用于定义 LwIP 中 TCP 发送缓冲区的大小; 它决定了你的设备在等待对方确认（ACK）之前，最多能“塞”进协议栈多少数据
#define TCP_SND_BUF                     (TCP_MSS * 2)
// TCP 发送队列中允许存在的最大报文段（Segment）数量, 限制了“待发送数据”能打包成多少个包裹，而不是限制字节的总数
// 这个宏通常不直接写死数字，而是根据 TCP_SND_BUF 和 TCP_MSS 动态计算出来的。
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
// TCP 接收窗口（Receive Window）的大小
#define TCP_WND                         (TCP_MSS * 2)
// TCP 段（Segment）内存池的最大数量
// MEMP_NUM_TCP_SEG ≥ TCP_SND_QUEUELEN + 乱序接收余量
#define MEMP_NUM_TCP_SEG                (TCP_SND_QUEUELEN + TCP_QUEUE_OOSEQ*8)
// TCP 发送低水位标记, 
// TCP_SNDLOWAT 必须小于 TCP_SND_BUF
// TCP_SNDLOWAT 通常设置为 TCP_MSS 的整数倍，且不应超过 TCP_SND_BUF 的 50%
#define TCP_SNDLOWAT                    TCP_MSS     // (TCP_SND_BUF/2)
// TCP 发送队列的“包数量”低水位标记
#define TCP_SNDQUEUELOWAT               (TCP_SND_QUEUELEN/4)
// TCP 数据段的最大重传次数; 一般最大配置为12
#define TCP_MAXRTX                      8
// 连接过程，三次握手的最大重传次数，比 TCP_MAXRTX 小
#define TCP_SYNMAXRTX                   (((TCP_MAXRTX/2) < 3) ? 3 : (TCP_MAXRTX/2))
/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 */
#ifndef LWIP_TCP_KEEPALIVE
    #define LWIP_TCP_KEEPALIVE          1
#endif


#if NO_SYS == 0
    // TCP/IP 消息邮箱大小
    #define TCPIP_MBOX_SIZE             8
    // TCP/IP 线程优先级
    #define TCPIP_THREAD_PRIO           10
    // TCP/IP 线程栈大小
    #define TCPIP_THREAD_STACKSIZE      4096
    // TCP/IP 线程名字
    #define TCPIP_THREAD_NAME           "tcpip"
    // TCP/IP 接收消息邮箱大小
    #define DEFAULT_TCP_RECVMBOX_SIZE   10
#endif



// 是否在软件中计算发送 IP 数据包的校验和
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

// 配置统计功能
#ifdef HTD_LWIP_STATS
    #define LWIP_STATS                  1
    #define LWIP_STATS_DISPLAY          1
#else
    #define LWIP_STATS                  0
#endif
#if LWIP_STATS
    #define LINK_STATS                  1
    #define IP_STATS                    1
    #define ICMP_STATS                  1
    #define IGMP_STATS                  1
    #define IPFRAG_STATS                1
    #define UDP_STATS                   1
    #define TCP_STATS                   1
    #define MEM_STATS                   1
    #define MEMP_STATS                  1
    #define PBUF_STATS                  1
    #define SYS_STATS                   1
    #define MIB2_STATS                  1
#endif /* LWIP_STATS */


#ifndef LWIP_NETIF_API
    #if NO_SYS
        #define LWIP_NETIF_API          0
    #else
        #define LWIP_NETIF_API          1
    #endif
#endif

// 支持网卡名字
#ifndef LWIP_NETIF_HOSTNAME
    #define LWIP_NETIF_HOSTNAME             1
#endif

#define LWIP_NETIF_STATUS_CALLBACK      1

// 未知协议包回调
//#ifdef LWIP_HOOK_UNKNOWN_ETH_PROTOCOL
//    #define LWIP_HOOK_UNKNOWN_ETH_PROTOCOL lwip_hook_unknown_eth_protocol
//#endif /* LWIP_HOOK_UNKNOWN_ETH_PROTOCOL */


// socket 相关
//////////////////////////////////////////////////////////////////////
#if NO_SYS == 0
    // 控制 LwIP 是否将标准的 POSIX 文件 I/O 函数名（如 read、write、close 等）映射为 LwIP 内部对应的 Socket 函数（如 lwip_read、lwip_write、lwip_close）
    #ifndef LWIP_POSIX_SOCKETS_IO_NAMES
        #define LWIP_POSIX_SOCKETS_IO_NAMES 0
    #endif
    // 使能 socket 接口
    #ifndef LWIP_COMPAT_SOCKETS
        #define LWIP_COMPAT_SOCKETS         1
    #endif
    // 使能发送超时参数
    #ifndef LWIP_SO_SNDTIMEO
        #define LWIP_SO_SNDTIMEO            1
    #endif
    // 使能接收超时参数
    #ifndef LWIP_SO_RCVTIMEO
        #define LWIP_SO_RCVTIMEO            1
    #endif
    // 打开“获取和设置 Socket 接收缓冲区大小”功能
    #ifndef LWIP_SO_RCVBUF
        #define LWIP_SO_RCVBUF              1
        #ifndef RECV_BUFSIZE_DEFAULT
            #define RECV_BUFSIZE_DEFAULT    8192
        #endif
    #endif
    // 是否支持套接字（Socket）的地址复用功能
    #ifndef SO_REUSE
        #define SO_REUSE                    0
    #endif
#endif /*NO_SYS == 0*/



// PPP 相关
//////////////////////////////////////////////////////////////////////
#if defined(HTD_LWIP_PPP)
    #define PPP_SUPPORT                 1
    #define NUM_PPP                     1
    // PPP Over Ethernet 插网线并且需要账号密码拨号上网的路由器或网关
    #ifdef HTD_LWIP_PPPOE
        #define PPPOE_SUPPORT           1
    #else
        #define PPPOE_SUPPORT           0
    #endif
    // PPP Over Serials 通过串口接 4G 模组上网的物联网设备
    #ifdef HTD_LWIP_PPPOS
        #define PPPOS_SUPPORT           1
    #else
        #define PPPOS_SUPPORT           0
    #endif
    #define PAP_SUPPORT                 1      /* Set > 0 for PAP. */
    #define CHAP_SUPPORT                1      /* Set > 0 for CHAP. */
    #define MSCHAP_SUPPORT              0      /* Set > 0 for MSCHAP (NOT FUNCTIONAL!) */
    #define CBCP_SUPPORT                0      /* Set > 0 for CBCP (NOT FUNCTIONAL!) */
    #define CCP_SUPPORT                 0      /* Set > 0 for CCP (NOT FUNCTIONAL!) */
    #define VJ_SUPPORT                  1      /* Set > 0 for VJ header compression. */
    #define MD5_SUPPORT
    
#else
    #define PPP_SUPPORT                 0
    #define PPPOS_SUPPORT               0
    #define PPPOE_SUPPORT               0
    #define SLIP_SUPPORT                0
#endif


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __LWIPOPTS_H__ */

