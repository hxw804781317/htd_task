#ifndef _DRV_NETWORK_ENC28J60_H_
#define _DRV_NETWORK_ENC28J60_H_

#include <stdint.h>
#include "htd_network_dev.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
#ifndef HTD_NETWORK_BUS_NAME
    #define HTD_NETWORK_BUS_NAME            "spim0"
#endif /*HTD_NETWORK_BUS_NAME*/
#ifndef NETWORK_DEV_SPI_PIN_CS
    #define NETWORK_DEV_SPI_PIN_CS          10
#endif /*NETWORK_DEV_SPI_PIN_CS*/
#ifndef NETWORK_DEV_SPI_PIN_MOSI
    #define NETWORK_DEV_SPI_PIN_MOSI        11
#endif /*NETWORK_DEV_SPI_PIN_MOSI*/
#ifndef NETWORK_DEV_SPI_PIN_MISO
    #define NETWORK_DEV_SPI_PIN_MISO        12
#endif /*NETWORK_DEV_SPI_PIN_MISO*/
#ifndef NETWORK_DEV_SPI_PIN_SCK
    #define NETWORK_DEV_SPI_PIN_SCK         13
#endif /*NETWORK_DEV_SPI_PIN_SCK*/
#ifndef NETWORK_DEV_PIN_CLK_OUT
    #define NETWORK_DEV_PIN_CLK_OUT         14
#endif /*NETWORK_DEV_PIN_CLK_OUT*/


#ifndef HTD_NETWORK_POWER_PIN
    #define HTD_NETWORK_POWER_PIN           -1
#endif /*HTD_NETWORK_POWER_PIN*/
#ifndef HTD_NETWORK_RESET_PIN
    #define HTD_NETWORK_RESET_PIN           2
#endif /*HTD_NETWORK_RESET_PIN*/
#ifndef HTD_NETWORK_INT_PIN
    #define HTD_NETWORK_INT_PIN             3
#endif /*HTD_NETWORK_INT_PIN*/



//////////////////////////////////////////////////////////////////////
#define ADDR_MASK                   0X1F
#define BANK_MASK                   0X60
#define SPRD_MASK                   0X80

//所有Bank共有的寄存器
#define EIE                         0x1B
#define EIR                         0x1C
    #define EIR_PKTIF               0x40                // 接收数据包待处理中断标志位
    #define EIR_DMAIF               0x20                // DMA中断标志位
    #define EIR_LINKIF              0x10                // 连接状态改变中断标志位, 读取PHIR寄存器可清零该位
    #define EIR_TXIF                0x08                // 发送中断标志位
    #define EIR_WOLIF               0x04                // WOL中断标志位
    #define EIR_TXERIF              0x02                // 发送错误中断标志位
    #define EIR_RXERIF              0x01                // 接收错误中断标志位
#define ESTAT                       0x1D
#define ECON2                       0x1E
#define ECON1                       0x1F

//Bank 0寄存器
#define ERDPTL                      (0x00|0x00)
#define ERDPTH                      (0x01|0x00)
#define EWRPTL                      (0x02|0x00)
#define EWRPTH                      (0x03|0x00)
#define ETXSTL                      (0x04|0x00)
#define ETXSTH                      (0x05|0x00)
#define ETXNDL                      (0x06|0x00)
#define ETXNDH                      (0x07|0x00)
#define ERXSTL                      (0x08|0x00)
#define ERXSTH                      (0x09|0x00)
#define ERXNDL                      (0x0A|0x00)
#define ERXNDH                      (0x0B|0x00)
#define ERXRDPTL                    (0x0C|0x00)
#define ERXRDPTH                    (0x0D|0x00)
#define ERXWRPTL                    (0x0E|0x00)
#define ERXWRPTH                    (0x0F|0x00)
#define EDMASTL                     (0x10|0x00)
#define EDMASTH                     (0x11|0x00)
#define EDMANDL                     (0x12|0x00)
#define EDMANDH                     (0x13|0x00)
#define EDMADSTL                    (0x14|0x00)
#define EDMADSTH                    (0x15|0x00)
#define EDMACSL                     (0x16|0x00)
#define EDMACSH                     (0x17|0x00)

//Bank 1寄存器
#define EHT0                        (0x00|0x20)
#define EHT1                        (0x01|0x20)
#define EHT2                        (0x02|0x20)
#define EHT3                        (0x03|0x20)
#define EHT4                        (0x04|0x20)
#define EHT5                        (0x05|0x20)
#define EHT6                        (0x06|0x20)
#define EHT7                        (0x07|0x20)
#define EPMM0                       (0x08|0x20)
#define EPMM1                       (0x09|0x20)
#define EPMM2                       (0x0A|0x20)
#define EPMM3                       (0x0B|0x20)
#define EPMM4                       (0x0C|0x20)
#define EPMM5                       (0x0D|0x20)
#define EPMM6                       (0x0E|0x20)
#define EPMM7                       (0x0F|0x20)
#define EPMCSL                      (0x10|0x20)
#define EPMCSH                      (0x11|0x20)
#define EPMOL                       (0x14|0x20)
#define EPMOH                       (0x15|0x20)
#define EWOLIE                      (0x16|0x20)
#define EWOLIR                      (0x17|0x20)
// 接收过滤器控制寄存器
#define ERXFCON                     (0x18|0x20)
    #define ERXFCON_UCEN            0x80        // 单播过滤器使能位, ANDDR为1时，目标地址与本地MAC地址不匹配的数据包将被丢弃
    #define ERXFCON_ANDOR           0x40        // AND/OR过滤器选择位
    #define ERXFCON_CRCEN           0x20        // 后过滤器CRC校验使能位, 所有CRC无效的数据包都将被丢弃
    #define ERXFCON_PMEN            0x10        // 格式匹配过滤器使能位
    #define ERXFCON_MPEN            0x08        // Magic Packet过滤器使能位
    #define ERXFCON_HTEN            0x04        // 哈希表过滤器使能位
    #define ERXFCON_MCEN            0x02        // 组播过滤器使能位
    #define ERXFCON_BCEN            0x01        // 广播过滤器使能位
#define EPKTCNT                     (0x19|0x20)

//Bank 2寄存器
#define MACON1                      (0x00|0x40|0x80)
#define MACON2                      (0x01|0x40|0x80)
#define MACON3                      (0x02|0x40|0x80)
#define MACON4                      (0x03|0x40|0x80)
#define MABBIPG                     (0x04|0x40|0x80)
#define MAIPGL                      (0x06|0x40|0x80)
#define MAIPGH                      (0x07|0x40|0x80)
#define MACLCON1                    (0x08|0x40|0x80)
#define MACLCON2                    (0x09|0x40|0x80)
#define MAMXFLL                     (0x0A|0x40|0x80)
#define MAMXFLH                     (0x0B|0x40|0x80)
#define MAPHSUP                     (0x0D|0x40|0x80)
#define MICON                       (0x11|0x40|0x80)
#define MICMD                       (0x12|0x40|0x80)
#define MIREGADR                    (0x14|0x40|0x80)
#define MIWRL                       (0x16|0x40|0x80)
#define MIWRH                       (0x17|0x40|0x80)
#define MIRDL                       (0x18|0x40|0x80)
#define MIRDH                       (0x19|0x40|0x80)

//Bank 3 寄存器
#define MAADR1                      (0x00|0x60|0x80)
#define MAADR0                      (0x01|0x60|0x80)
#define MAADR3                      (0x02|0x60|0x80)
#define MAADR2                      (0x03|0x60|0x80)
#define MAADR5                      (0x04|0x60|0x80)
#define MAADR4                      (0x05|0x60|0x80)
#define EBSTSD                      (0x06|0x60)
#define EBSTCON                     (0x07|0x60)
#define EBSTCSL                     (0x08|0x60)
#define EBSTCSH                     (0x09|0x60)
#define MISTAT                      (0x0A|0x60|0x80)
#define EREVID                      (0x12|0x60)
#define ECOCON                      (0x15|0x60)
#define EFLOCON                     (0x17|0x60)
#define EPAUSL                      (0x18|0x60)
#define EPAUSH                      (0x19|0x60)

//PHY寄存器
#define PHCON1                      0x00
#define PHSTAT1                     0x01
#define PHHID1                      0x02
#define PHHID2                      0x03
#define PHCON2                      0x10
#define PHSTAT2                     0x11
// PHY中断允许寄存器
#define PHIE                        0x12
    #define PHIE_PLNKIE             (1 << 4)            // PHY 连接状态改变中断允许位
    #define PHIE_PGEIE              (1 << 1)            // PHY 全局中断允许位
// PHY中断请求（标志）寄存器
#define PHIR                        0x13
    #define PHIR_PLNKIF             (1 << 4)            // 上次读取PHIR以后PHY的连接状态发生了改变；读取时复位为 0
    #define PHIR_PGEIF              (1 << 1)            // 上次读取PHIR以后发生了一个或多个被允许的PHY中断；读取时复位为 0
#define PHLCON                      0x14

//ENC28J60 EIE寄存器位定义
#define EIE_INTIE                   0x80
#define EIE_PKTIE                   0x40
#define EIE_DMAIE                   0x20
#define EIE_LINKIE                  0x10
#define EIE_TXIE                    0x08
#define EIE_WOLIE                   0x04
#define EIE_TXERIE                  0x02
#define EIE_RXERIE                  0x01


//ENC28J60 ESTAT寄存器位定义
#define ESTAT_INT                   0x80
#define ESTAT_LATECOL               0x10
#define ESTAT_RXBUSY                0x04
#define ESTAT_TXABRT                0x02
#define ESTAT_CLKRDY                0x01
//NC28J60 ECON2寄存器位定义
#define ECON2_AUTOINC               0x80
#define ECON2_PKTDEC                0x40
#define ECON2_PWRSV                 0x20
#define ECON2_VRPS                  0x08
//NC28J60 ECON1寄存器位定义
#define ECON1_TXRST                 0x80
#define ECON1_RXRST                 0x40
#define ECON1_DMAST                 0x20
#define ECON1_CSUMEN                0x10
#define ECON1_TXRTS                 0x08
#define ECON1_RXEN                  0x04
#define ECON1_BSEL1                 0x02
#define ECON1_BSEL0                 0x01
//ENC28J60 MACON1寄存器位定义
#define MACON1_LOOPBK               0x10
#define MACON1_TXPAUS               0x08
#define MACON1_RXPAUS               0x04
#define MACON1_PASSALL              0x02
#define MACON1_MARXEN               0x01
//ENC28J60 MACON2寄存器位定义
#define MACON2_MARST                0x80
#define MACON2_RNDRST               0x40
#define MACON2_MARXRST              0x08
#define MACON2_RFUNRST              0x04
#define MACON2_MATXRST              0x02
#define MACON2_TFUNRST              0x01
//ENC28J60 MACON3寄存器位定义
#define MACON3_PADCFG2              0x80
#define MACON3_PADCFG1              0x40
#define MACON3_PADCFG0              0x20
#define MACON3_TXCRCEN              0x10
#define MACON3_PHDRLEN              0x08
#define MACON3_HFRMLEN              0x04
#define MACON3_FRMLNEN              0x02
#define MACON3_FULDPX               0x01
// ENC28J60 MACON4 寄存器位定义
#define MACON4_DEFER                (1<<6)
#define MACON4_BPEN                 (1<<5)
#define MACON4_NOBKOFF              (1<<4)
//ENC28J60 MICMD 寄存器位定义
#define MICMD_MIISCAN               0x02
#define MICMD_MIIRD                 0x01
//ENC28J60 MISTAT 寄存器位定义
#define MISTAT_NVALID               0x04
#define MISTAT_SCAN                 0x02
#define MISTAT_BUSY                 0x01


//ENC28J60 PHY PHCON1 寄存器位定义
#define PHCON1_PRST                 0x8000
#define PHCON1_PLOOPBK              0x4000
#define PHCON1_PPWRSV               0x0800
#define PHCON1_PDPXMD               0x0100
//ENC28J60 PHY PHSTAT1 寄存器位定义
#define PHSTAT1_PFDPX               0x1000
#define PHSTAT1_PHDPX               0x0800
#define PHSTAT1_LLSTAT              0x0004
#define PHSTAT1_JBSTAT              0x0002
// ENC28J60 PHY PHSTAT2 寄存器位定义
#define PHSTAT2_TXSTAT              (1 << 13)
#define PHSTAT2_RXSTAT              (1 << 12)
#define PHSTAT2_COLSTAT             (1 << 11)
#define PHSTAT2_LSTAT               (1 << 10)
#define PHSTAT2_DPXSTAT             (1 << 9)
#define PHSTAT2_PLRITY              (1 << 5)
// ENC28J60 PHY PHCON2 寄存器位定义
#define PHCON2_FRCLINK              0x4000
#define PHCON2_TXDIS                0x2000
#define PHCON2_JABBER               0x0400
#define PHCON2_HDLDIS               0x0100

//ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN             0x08
#define PKTCTRL_PPADEN              0x04
#define PKTCTRL_PCRCEN              0x02
#define PKTCTRL_POVERRIDE           0x01


/* ENC28J60 Transmit Status Vector */
#define TSV_TXBYTECNT               0
#define TSV_TXCOLLISIONCNT          16
#define TSV_TXCRCERROR              20
#define TSV_TXLENCHKERROR           21
#define TSV_TXLENOUTOFRANGE         22
#define TSV_TXDONE                  23
#define TSV_TXMULTICAST             24
#define TSV_TXBROADCAST             25
#define TSV_TXPACKETDEFER           26
#define TSV_TXEXDEFER               27
#define TSV_TXEXCOLLISION           28
#define TSV_TXLATECOLLISION         29
#define TSV_TXGIANT                 30
#define TSV_TXUNDERRUN              31
#define TSV_TOTBYTETXONWIRE         32
#define TSV_TXCONTROLFRAME          48
#define TSV_TXPAUSEFRAME            49
#define TSV_BACKPRESSUREAPP         50
#define TSV_TXVLANTAGFRAME          51
#define TSV_SIZE                    7
#define TSV_BYTEOF(x)               ((x) / 8)
#define TSV_BITMASK(x)              (1 << ((x) % 8))
#define TSV_GETBIT(x, y)            (((x)[TSV_BYTEOF(y)] & TSV_BITMASK(y)) ? 1 : 0)

/* ENC28J60 Receive Status Vector */
#define RSV_RXLONGEVDROPEV          16
#define RSV_CARRIEREV               18
#define RSV_CRCERROR                20
#define RSV_LENCHECKERR             21
#define RSV_LENOUTOFRANGE           22
#define RSV_RXOK                    23
#define RSV_RXMULTICAST             24
#define RSV_RXBROADCAST             25
#define RSV_DRIBBLENIBBLE           26
#define RSV_RXCONTROLFRAME          27
#define RSV_RXPAUSEFRAME            28
#define RSV_RXUNKNOWNOPCODE         29
#define RSV_RXTYPEVLAN              30
#define RSV_SIZE                    6
#define RSV_BITMASK(x)              (1 << ((x) - 16))
#define RSV_GETBIT(x, y)            (((x) & RSV_BITMASK(y)) ? 1 : 0)


//SPI运行代码
#define ENC28J60_READ_CTRL_REG      0x00
#define ENC28J60_READ_BUF_MEM       0x3A
#define ENC28J60_WRITE_CTRL_REG     0x40
#define ENC28J60_WRITE_BUF_MEM      0x7A    // 
#define ENC28J60_BIT_FIELD_SET      0x80    // 设置某个位
#define ENC28J60_BIT_FIELD_CLR      0xA0    // 清除某个位
#define ENC28J60_SOFT_RESET         0xFF


#define MAX_TX_PACKAGE_SIZE         (1536)
// 接收buffer起始位置必须为0
#define RXSTART_INIT                0x0000
// 接收buffer终止位置
#define RXSTOP_INIT                 0x19FF
// 发送buffer的起始位置
#define TXSTART_INIT                0x1A00
// 发送buffer的终止位置
#define TXSTOP_INIT                 0x1FFF
// 以太网最大帧长度
#ifndef MAX_FRAMELEN
    #define MAX_FRAMELEN            1518
#endif
#define MAX_ADDR_LEN                6



// @brief 初始化enc28j60
int32_t drv_enc28j60_open(const char* p_macaddr);
// @brief 关闭
int32_t drv_enc28j60_close(void);

// @brief 发送数据包
int32_t drv_enc28j60_packet_write(const uint8_t* p_pkt, uint32_t pkt_len);
// @brief 接收数据包
int32_t drv_enc28j60_packet_read(uint8_t* p_pkt, uint32_t max_len);
// @brief 读取版本号
uint8_t drv_enc28j60_revision_get(void);
// @brief 判断 ENC28J60 是否为全双工状态，否为半双工
bool drv_enc28j60_is_full_duplex(void);
// @brief 读取mac地址
void drv_enc28j60_mac_addr_get(uint8_t* p_mac);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DRV_NETWORK_ENC28J60_H_*/

