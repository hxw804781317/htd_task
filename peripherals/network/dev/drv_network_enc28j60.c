#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "net.enc28j60"
#include "htd_common.h"
#include "htd_network_dev.h"
#include "htd_device.h"
#include "drv_network_enc28j60.h"
#include "htd_network_thread.h"



//////////////////////////////////////////////////////////////////////
static uint16_t m_next_pkt_ptr = 0;
static uint8_t erxfcon = 0;
static net_dev_indicate_handle_t m_network_indicate_handle = NULL;
static spim_handle_t* m_enc28j60_bus = NULL;
const static char m_enc28j60_default_macaddr[] = {0xb8, 0xea, 0x96, 0x82, 0xba, 0xce};


// @brief 读操作
static uint8_t enc28j60_op_read(uint8_t op, uint8_t address)
{
    uint8_t wbuff[4], rbuff[4];
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 0);
    wbuff[0] = op | (address & ADDR_MASK);
    htd_spim_transfer(m_enc28j60_bus, wbuff, 1, rbuff, 1);
    if (address & 0x80) {
        htd_spim_transfer(m_enc28j60_bus, NULL, 0, rbuff, 1);
    }
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 1);
    return rbuff[0];
}

// @brief 写操作
static void enc28j60_op_write(uint8_t op, uint8_t address, uint8_t data)
{
    uint8_t wbuff[4];
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 0);
    memset(wbuff, 0, sizeof(wbuff));
    wbuff[0] = op | (address & ADDR_MASK);
    wbuff[1] = data;
    htd_spim_transfer(m_enc28j60_bus, wbuff, 2, NULL, 0);
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 1);
}

// @brief 读数据
static void enc28j60_buff_read(uint8_t* p_buff, uint16_t buff_len)
{
    if (m_enc28j60_bus == NULL) {
        return;
    }

    uint8_t wbuff[4];
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 0);
    memset(wbuff, 0, sizeof(wbuff));
    wbuff[0] = ENC28J60_READ_BUF_MEM;
    htd_spim_transfer(m_enc28j60_bus, wbuff, 1, p_buff, buff_len);
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 1);
}

// @brief 写数据
static void enc28j60_buff_write(const uint8_t* p_buff, uint16_t buff_len)
{
    if (m_enc28j60_bus == NULL) {
        return;
    }

    uint8_t wbuff[4];
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 0); 
    memset(wbuff, 0, sizeof(wbuff));
    wbuff[0] = ENC28J60_WRITE_BUF_MEM;
    htd_spim_transfer(m_enc28j60_bus, wbuff, 1, NULL, 0);
    // while(!(SPSR & (1<<SPIF)));
    htd_spim_transfer(m_enc28j60_bus, p_buff, buff_len, NULL, 0);
    //htd_delay_us(10);
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 1);
}

// @brief 设置操作区块
static void enc28j60_bank_set(uint8_t address)
{
    static uint8_t m_enc28j60_bank = 0xff;
    if ((address & BANK_MASK) != m_enc28j60_bank) {
        m_enc28j60_bank = address & BANK_MASK;
        enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
        enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON1, m_enc28j60_bank >> 5);
    }
}



//////////////////////////////////////////////////////////////////////
// @brief 读地址
static uint8_t enc28j60_reg_read(uint8_t reg)
{
    enc28j60_bank_set(reg);
    return enc28j60_op_read(ENC28J60_READ_CTRL_REG, reg);
}

// @brief 写地址
static void enc28j60_reg_write(uint8_t reg, uint8_t data)
{
    enc28j60_bank_set(reg); 
    enc28j60_op_write(ENC28J60_WRITE_CTRL_REG, reg, data);
}

// @brief 读寄存器两个字节
static uint16_t enc28j60_reg_word_read(uint8_t reg)
{
    uint8_t l8 = enc28j60_reg_read(reg);         /// 再读低八位
    uint8_t h8 = enc28j60_reg_read(reg+1);         /// 再读高八位
    return ((h8 << 8) | l8);
}

// @brief 写寄存器两个字节
static void enc28j60_reg_word_write(uint8_t reg, uint16_t word)
{
#if 0
    uint8_t l8 = word & 0xff;
    uint8_t h8 = (word >> 8) & 0xff;
    enc28j60_reg_write(reg, l8);
    enc28j60_reg_write(reg+1, h8);
#else
    enc28j60_bank_set(reg); 
    
    uint8_t wbuff[4];
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 0);
    memset(wbuff, 0, sizeof(wbuff));
    wbuff[0] = ENC28J60_WRITE_CTRL_REG | (reg & ADDR_MASK);
    wbuff[1] = word & 0xff;
    wbuff[2] = (word >> 8) & 0xff;
    htd_spim_transfer(m_enc28j60_bus, wbuff, 3, NULL, 0);
    htd_pin_write(NETWORK_DEV_SPI_PIN_CS, 1);
#endif
}

// @brief 写 phy 寄存器数据
static void enc28j60_phy_write(uint8_t address, uint16_t data)
{
    // 向MIREGADR写入PHY寄存器地址
    enc28j60_reg_write(MIREGADR, address);
    // 写入数据
    enc28j60_reg_word_write(MIWRL, data);
    // 等待完成
    uint32_t retry = 0;
    while (enc28j60_reg_read(MISTAT) & MISTAT_BUSY && retry < 10000) {
        htd_delay_us(1);
        retry++;
    }
    ASSERT(retry < 10000);
}

// @brief 读 phy 寄存器数据
static uint16_t enc28j60_phy_read(uint8_t address)
{
    //uint8_t micmd_regval = enc28j60_reg_read(MICMD);
    enc28j60_reg_write(MIREGADR, address);
    // 开始读PHY寄存器
    //enc28j60_reg_write(MICMD, micmd_regval|MICMD_MIIRD);
    enc28j60_reg_write(MICMD, MICMD_MIIRD);
    //等待读PHY完成, 大概需要10.24us
    uint32_t retry = 0;
    htd_delay_us(5);
    while ((enc28j60_reg_read(MISTAT) & MISTAT_BUSY) && retry < 10000) {
        retry++;
        htd_delay_us(1);
    }
    ASSERT(retry < 10000);
    // 读PHY完成
    //enc28j60_reg_write(MICMD, micmd_regval & (~MICMD_MIIRD));
    enc28j60_reg_write(MICMD, 0x00);
    // 返回结果
    return enc28j60_reg_word_read(MIRDL);
} 

// @brief 判断网线是否已经连接
static bool enc28j60_check_link_status(void)
{
    uint16_t phstat2 = enc28j60_phy_read(PHSTAT2);
    return (phstat2 & PHSTAT2_LSTAT) ? 1 : 0;
}


//////////////////////////////////////////////////////////////////////
// @brief 发送数据包
int32_t drv_enc28j60_packet_write(const uint8_t* p_pkt, uint32_t pkt_len)
{
    ASSERT(pkt_len <= MAX_FRAMELEN);
    // 正在发送数据，等待发送完成
    while (enc28j60_reg_read(ECON1) & 0X08);
    
    // 设置发送缓冲区地址写指针入口
    enc28j60_reg_word_write(EWRPTL, TXSTART_INIT);
    // 设置TXND指针，以对应给定的数据包大小 
    enc28j60_reg_word_write(ETXNDL, TXSTART_INIT+pkt_len);
    // 写每包控制字节（0x00表示使用macon3的设置）
    enc28j60_op_write(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
    // 复制数据包到发送缓冲区
    enc28j60_buff_write(p_pkt, pkt_len);
    // 发送数据到网络
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
    // 复位发送逻辑的问题。参见Rev. B4 Silicon Errata point 12.
    if ((enc28j60_reg_read(EIR) & EIR_TXERIF)) {
        enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
    }
    //print_hex(">>S", p_pkt, pkt_len);
    return pkt_len;
}

// @brief 接收数据包
int32_t drv_enc28j60_packet_read(uint8_t* p_pkt, uint32_t max_len)
{
    // 是否收到数据包
    if (enc28j60_reg_read(EPKTCNT) == 0) {
        //print_warning("no packet.");
        return 0;
    }

    // 接收忙，等待接收完成
    while ((enc28j60_reg_read(ESTAT) & 0X04));

    // 设置接收缓冲器读指针
    enc28j60_reg_word_write(ERDPTL, m_next_pkt_ptr);

    uint8_t buff[6];
    enc28j60_buff_read(buff, sizeof(buff));
    m_next_pkt_ptr = (buff[1] << 8) | buff[0];
    uint16_t pkt_len = (buff[3] << 8) | buff[2];
    uint16_t rxstat = (buff[5] << 8) | buff[4];
    print_debug("m_next_pkt_ptr=0x%04x, pkt_len=0x%04x, rxstat=0x%04x", m_next_pkt_ptr, pkt_len, rxstat);
    
    // 限制接收长度 
    pkt_len -= 4; //以太帧最小46字节 减去4字节的FCS校验和 加上帧头14字节 共64字节
    if (pkt_len > max_len) {
        pkt_len = max_len;
    }
    
    // 检查CRC和符号错误
    // ERXFCON.CRCEN 已经设置, 则不需要检查
    if ((erxfcon & ERXFCON_CRCEN) == 0 && (rxstat & 0x80) == 0) {
        pkt_len = 0;
    } else {
        enc28j60_buff_read(p_pkt, pkt_len);     // 从接收缓冲器中复制数据包
    }
    
    // RX读指针移动到下一个接收到的数据包的开始位置, 并释放我们刚才读出过的内存
    //enc28j60_reg_word_write(ERXRDPTL, m_next_pkt_ptr - 1);
    enc28j60_reg_word_write(ERXRDPTL, m_next_pkt_ptr);
    // 递减数据包计数器标志我们已经得到了这个包 
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
    return pkt_len;
}

// @brief 读取版本号
uint8_t drv_enc28j60_revid_get(void)
{
    uint8_t revision = enc28j60_reg_read(EREVID);
    revision &= 0x1f;
    return revision;
}

// @brief 判断 ENC28J60 是否为全双工状态，否为半双工
bool drv_enc28j60_is_full_duplex(void)
{
    uint16_t temp = enc28j60_phy_read(PHCON1)&0X0100;
    return (temp>>8) != 0 ? 1 : 0;
}

// @brief 读取mac地址
void drv_enc28j60_mac_addr_get(uint8_t* p_mac)
{
    //FIXME: 这里应该可以连读
    *p_mac++ = enc28j60_reg_read(MAADR5);
    *p_mac++ = enc28j60_reg_read(MAADR4);
    *p_mac++ = enc28j60_reg_read(MAADR3);
    *p_mac++ = enc28j60_reg_read(MAADR2);
    *p_mac++ = enc28j60_reg_read(MAADR1);
    *p_mac++ = enc28j60_reg_read(MAADR0);
}

// @brief 使能广播功能
void drv_enc28j60_broadcast_enable(void) 
{
    erxfcon |= ERXFCON_BCEN;
    enc28j60_reg_write(ERXFCON, erxfcon);
}

// @brief 关闭广播功能
void drv_enc28j60_broadcast_disable(void) 
{
    erxfcon &= ~ERXFCON_BCEN;
    enc28j60_reg_write(ERXFCON, erxfcon);
}

// @brief 使能组播功能
void drv_enc28j60_multicast_enable(void) 
{
    erxfcon |= ERXFCON_MCEN;
    enc28j60_reg_write(ERXFCON, erxfcon);
}

// @brief 关闭组播功能
void drv_enc28j60_multicast_disable(void) 
{
    erxfcon &= ~ERXFCON_MCEN;
    enc28j60_reg_write(ERXFCON, erxfcon);
}

// @brief 脉冲输出 @ECOCON_FREQ_3125KHZ
void drv_enc28j60_clkout_set(uint8_t clock_freq)
{
    enc28j60_reg_write(ECOCON, clock_freq & 0x7);
}

// @brief 关闭
void drv_enc28j60_power_down(void) 
{
    enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
    while (enc28j60_reg_read(ESTAT) & ESTAT_RXBUSY);
    while (enc28j60_reg_read(ECON1) & ECON1_TXRTS);
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
}

// @brief 启动
void drv_enc28j60_power_up(void) 
{
    enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
    while (! (enc28j60_reg_read(ESTAT) & ESTAT_CLKRDY));
}

// @brief 中断处理函数
void drv_enc28j60_int_handler(void* arg)
{
    uint8_t status;
    uint8_t packetnum;
    uint16_t temp;

    //关闭ENC28J60的全局中断
    enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, EIE, EIE_INTIE);
    // 读取以太网中断标志寄存器
    status = enc28j60_reg_read(EIR);
    if (status) {
        enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, EIR, status);
    }

    print_debug("EIR=0x%02x", status);
    if (status & EIR_PKTIF) {
        // 接收到数据,处理数据
        if (m_network_indicate_handle) {
            m_network_indicate_handle(NET_DEV_INDICATE_DATA_RECV, NULL, 0);
        }
    }

    // 以太网发送中断
    //if (status & EIR_TXIF) {
        // 清除ENC28J60的接收中断标志位
        //enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXIF); 
    //}
    
    // 网线插拔判断
    if (status & EIR_LINKIF) {
        enc28j60_phy_read(PHIR);
        uint8_t link_status = enc28j60_check_link_status();
        if (m_network_indicate_handle) {
            m_network_indicate_handle(NET_DEV_INDICATE_PHY_LINK, &link_status, sizeof(uint8_t));
        }
        print_warning("link status change: %d", link_status);
    }
    
    // 接收错误中断标志位
    if (status & EIR_RXERIF) {
        //enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, EIR, EIR_RXERIF);   
        packetnum = enc28j60_reg_read(EPKTCNT);
        temp = enc28j60_reg_word_read(ERXRDPTL);
        temp++;
        enc28j60_reg_word_write(ERXRDPTL, temp);
        enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
        print_error("recv error. recv packet number: %d", packetnum);
    }
    
    // 发送错误中断标志位
    if (status & EIR_TXERIF) {
        //enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF);
        enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ESTAT, ESTAT_LATECOL|ESTAT_TXABRT);
        print_error("send error.");
    }
    // 打开ENC28J60的全局中断
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE);
}

#if (defined(HTD_NETWORK_INT_PIN) && HTD_NETWORK_INT_PIN > 0)
// @brief 中断处理函数
static void enc28j60_int_handler(void)
{
    network_thread_load_function_fisr(drv_enc28j60_int_handler, NULL);
}
HTD_PIN_INT_FUNC_REGISTER(HTD_NETWORK_INT_PIN, enc28j60_int_handler);
#endif /*(defined(HTD_NETWORK_INT_PIN) && HTD_NETWORK_INT_PIN > 0)*/

// @brief 中断控制引脚
static void enc28j60_int_pin_enable(bool is_enable)
{
#if (defined(HTD_NETWORK_INT_PIN) && HTD_NETWORK_INT_PIN > 0)
    htd_pin_int_set(HTD_NETWORK_INT_PIN, HTD_PIN_INT_FALLING, is_enable);
#endif /*(defined(HTD_NETWORK_INT_PIN) && HTD_NETWORK_INT_PIN > 0)*/
}

// @brief 控制上电引脚
static void enc28j60_power_pin_enable(bool is_enable)
{
#if (defined(HTD_NETWORK_POWER_PIN) && (HTD_NETWORK_POWER_PIN > 0))
    htd_pin_output_pp_set(HTD_NETWORK_POWER_PIN);
    #if defined(HTD_NETWORK_POWER_PIN_ACTIVE_HIGH) && HTD_NETWORK_POWER_PIN_ACTIVE_HIGH == 0
        is_enable = !is_enable;
    #endif
    htd_pin_write(HTD_NETWORK_POWER_PIN, is_enable);
    htd_delay_ms(5);
#endif /*(defined(HTD_NETWORK_POWER_PIN) && (HTD_NETWORK_POWER_PIN > 0))*/
}

// @brief 硬件复位
static bool enc28j60_reset(void)
{
    // RESET 引脚
#if (defined(HTD_NETWORK_RESET_PIN) && (HTD_NETWORK_RESET_PIN > 0))
    htd_pin_output_pp_set(HTD_NETWORK_RESET_PIN);
    htd_pin_write(HTD_NETWORK_RESET_PIN, 0);
    htd_delay_ms(10);
    htd_pin_write(HTD_NETWORK_RESET_PIN, 1);
    htd_delay_ms(10);
#endif /*(defined(HTD_NETWORK_RESET_PIN) && (HTD_NETWORK_RESET_PIN > 0))*/

    //软件复位
    enc28j60_op_write(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    htd_delay_us(10);
    
    //等待时钟稳定
    uint8_t stat;
    uint32_t start_ms = htd_time_ms_get();
    while (1) {
        stat = enc28j60_reg_read(ESTAT);
        if (stat & ESTAT_CLKRDY) {
            break;
        }
        if (htd_time_ms_diff(htd_time_ms_get(), start_ms) > 500) {
            print_error("waite status timeout. stat=0x%02x", stat);
            return false;
        }
        htd_delay_ms(1);
    }

    // 读取版本号
    uint8_t revid = drv_enc28j60_revid_get();
    if (revid == 0 || revid == 0xff) {
        print_error("revid=0x%02x", revid);
        return false;
    }
    print_debug("revid=0x%02x", revid);
    return true;
}

// @brief 初始化SPIM
static spim_handle_t* enc28j60_hw_init(void)
{
    // 初始化 CS 引脚
    htd_pin_output_pp_set(NETWORK_DEV_SPI_PIN_CS);

    // 初始化 SPIM
    const static spim_user_config_t spi_config = { 
        .pin_cs = PIN_NUM_INVALID,
        .pin_sck = NETWORK_DEV_SPI_PIN_SCK,
        .pin_mosi = NETWORK_DEV_SPI_PIN_MOSI,
        .pin_miso = NETWORK_DEV_SPI_PIN_MISO,
        .cs_active_level = 0,
        .is_1line = 0,
        .is_bit_order_msb = 1,
        .data_width = 8,
        .mode = SPIM_MODE_0,
        .freq_hz = 8*1000*1000,
        .xfer_finish = NULL,
    };
    spim_handle_t *p_handle = htd_spim_open(HTD_NETWORK_BUS_NAME, &spi_config);
    ASSERT(p_handle);

    // POWER 引脚上电
    enc28j60_power_pin_enable(true);
    
    // FIXME: enc28j60 时钟输出引脚
    // 使能中断引脚
    enc28j60_int_pin_enable(true);
    return p_handle;
}

// @brief 信息显示
static void drv_enc28j60_dump(void)
{
    print_warning("EIE=0x%02x", enc28j60_reg_read(EIE)); 
    print_warning("EIR=0x%02x", enc28j60_reg_read(EIR)); 
    print_warning("ESTAT=0x%02x", enc28j60_reg_read(ESTAT)); 
    print_warning("ECON1=0x%02x", enc28j60_reg_read(ECON1)); 
    print_warning("ECON2=0x%02x", enc28j60_reg_read(ECON2)); 
    print_warning("ERXFCON=0x%02x, erxfcon=0x%02x", enc28j60_reg_read(ERXFCON), erxfcon);
    print_warning("PHSTAT1=0x%04x", enc28j60_phy_read(PHSTAT1));
    print_warning("PHSTAT2=0x%04x", enc28j60_phy_read(PHSTAT2));

    print_warning("ERXSTL=0x%04x", enc28j60_reg_word_read(ERXSTL));
    print_warning("ERXNDL=0x%04x", enc28j60_reg_word_read(ERXNDL));
    print_warning("ERXRDPTL=0x%04x", enc28j60_reg_word_read(ERXRDPTL));
    print_warning("ETXSTL=0x%04x", enc28j60_reg_word_read(ETXSTL));
    print_warning("ETXNDL=0x%04x", enc28j60_reg_word_read(ETXNDL));
}

// @brief 初始化enc28j60
int32_t drv_enc28j60_open(const char* p_macaddr)
{
    if (m_enc28j60_bus != NULL) {
        return 0;
    }
    
    // 硬件初始化
    m_enc28j60_bus = enc28j60_hw_init();

    // 硬件复位
    int32_t i = 0;
    for (i=0; i<3; ++i) {
        if (enc28j60_reset()) {
            break;
        }
    }
    if (i >= 3) {
        print_error("enc28j60 init failed. bus_name=%s", m_enc28j60_bus->p_dev->p_name);
        enc28j60_int_pin_enable(false);
        htd_spim_close(m_enc28j60_bus);
        m_enc28j60_bus = NULL;
        enc28j60_power_pin_enable(false);
        return -1;
    }
    
    //接收缓冲器由一个硬件管理的循环FIFO 缓冲器构成。
    //寄存器对ERXSTH:ERXSTL 和ERXNDH:ERXNDL 作
    //为指针，定义缓冲器的容量和其在存储器中的位置。
    //ERXST和ERXND指向的字节均包含在FIFO缓冲器内。
    //当从以太网接口接收数据字节时，这些字节被顺序写入
    //接收缓冲器。 但是当写入由ERXND 指向的存储单元
    //后，硬件会自动将接收的下一字节写入由ERXST 指向
    //的存储单元。 因此接收硬件将不会写入FIFO 以外的单元。
    m_next_pkt_ptr = RXSTART_INIT;
    // 设置接收起始字节 
    enc28j60_reg_word_write(ERXSTL, RXSTART_INIT);
    // 设置接收结束字节
    enc28j60_reg_word_write(ERXNDL, RXSTOP_INIT);
    // 设置接收读指针字节, 必须指向接收缓冲区的结束位置（官方建议设为偶数边界）
    enc28j60_reg_word_write(ERXRDPTL, RXSTART_INIT);
    // 设置发送结束字节
    enc28j60_reg_word_write(ETXSTL, TXSTART_INIT);
    // 设置发送结束字节
    enc28j60_reg_word_write(ETXNDL, TXSTOP_INIT);

    //UCEN：单播过滤器使能位
    //  当ANDOR = 1 时：
    //    1 = 目标地址与本地MAC地址不匹配的数据包将被丢弃
    //    0 = 禁止过滤器
    //  当ANDOR = 0 时：
    //    1 = 目标地址与本地MAC地址匹配的数据包会被接受
    //    0 = 禁止过滤器
    //CRCEN：后过滤器CRC 校验使能位
    //  1 = 所有CRC 无效的数据包都将被丢弃
    //  0 = 不考虑CRC 是否有效
    //PMEN：格式匹配过滤器使能位
    //  当ANDOR = 1 时：
    //    1 = 数据包必须符合格式匹配条件，否则将被丢弃
    //    0 = 禁止过滤器
    //  当ANDOR = 0 时：
    //    1 = 符合格式匹配条件的数据包将被接受
    //    0 = 禁止过滤器
    erxfcon = ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN;
    erxfcon |= ERXFCON_BCEN;
    enc28j60_reg_write(ERXFCON, erxfcon);
    enc28j60_reg_word_write(EPMM0, 0x303f);
    enc28j60_reg_word_write(EPMCSL, 0xf7f9);
    
    //bit0 MARXEN：MAC 接收使能位
    //  1 = 允许MAC 接收数据包
    //  0 = 禁止数据包接收
    //bit3 TXPAUS：暂停控制帧发送使能位
    //  1 = 允许MAC 发送暂停控制帧（用于全双工模式下的流量控制）
    //  0 = 禁止暂停帧发送
    //bit 2 RXPAUS：暂停控制帧接收使能位
    //  1 = 当接收到暂停控制帧时，禁止发送（正常操作）
    //  0 = 忽略接收到的暂停控制帧
    enc28j60_reg_write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    //将 MACON2 中的 MARST 位清零，使 MAC 退出复位状态。
    enc28j60_reg_write(MACON2, 0x00);
    //bit7-5 PADCFG2:PACDFG0：自动填充和CRC 配置位
    //  111 = 用0 填充所有短帧至64 字节长，并追加一个有效的CRC
    //  110 = 不自动填充短帧
    //  101 = MAC 自动检测具有8100h 类型字段的VLAN 协议帧，并自动填充到64 字节长。如果不
    //        是VLAN 帧，则填充至60 字节长。填充后还要追加一个有效的CRC
    //  100 = 不自动填充短帧
    //  011 = 用0 填充所有短帧至64 字节长，并追加一个有效的CRC
    //  010 = 不自动填充短帧
    //  001 = 用0 填充所有短帧至60 字节长，并追加一个有效的CRC
    //  000 = 不自动填充短帧
    //bit4 TXCRCEN：发送CRC 使能位
    //  1 = 不管PADCFG如何，MAC都会在发送帧的末尾追加一个有效的CRC。 如果PADCFG规定要
    //      追加有效的CRC，则必须将TXCRCEN 置1。
    //  0 = MAC不会追加CRC。 检查最后4 个字节，如果不是有效的CRC 则报告给发送状态向量。
    //bit0 FULDPX：MAC 全双工使能位
    //  1 = MAC工作在全双工模式下。 PHCON1.PDPXMD 位必须置1。
    //  0 = MAC工作在半双工模式下。 PHCON1.PDPXMD 位必须清零。
    enc28j60_reg_write(MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);
    //enc28j60_op_write(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);

    // 配置非背对背包间间隔寄存器的低字节 MAIPGL。
    // 大多数应用使用 12h 编程该寄存器。
    // 如果使用半双工模式，应编程非背对背包间间隔
    // 寄存器的高字节 MAIPGH。 大多数应用使用 0Ch 编程该寄存器。
    enc28j60_reg_word_write(MAIPGL, 0x0c12);
    // 配置背对背包间间隔寄存器MABBIPG。当使用
    // 全双工模式时，大多数应用使用15h 编程该寄存
    // 器，而使用半双工模式时则使用12h 进行编程。
    enc28j60_reg_write(MABBIPG, 0x15);
    // 最大帧长度 1518
    enc28j60_reg_word_write(MAMXFLL, MAX_FRAMELEN);
    
    // 设置MAC地址
    if (p_macaddr == NULL) {
        p_macaddr = m_enc28j60_default_macaddr;
    }
    enc28j60_reg_write(MAADR0, p_macaddr[5]);
    enc28j60_reg_write(MAADR1, p_macaddr[4]);
    enc28j60_reg_write(MAADR2, p_macaddr[3]);
    enc28j60_reg_write(MAADR3, p_macaddr[2]);
    enc28j60_reg_write(MAADR4, p_macaddr[1]);
    enc28j60_reg_write(MAADR5, p_macaddr[0]);
    
    // 配置PHY为全双工  LEDB为拉电流
    enc28j60_phy_write(PHCON1, PHCON1_PDPXMD);
    // HDLDIS：PHY 半双工环回禁止位
    // 当PHCON1.PDPXMD = 1 或PHCON1.PLOOPBK = 1 时：
    //   此位可被忽略。
    // 当PHCON1.PDPXMD = 0 且PHCON1.PLOOPBK = 0 时：
    //   1 = 要发送的数据仅通过双绞线接口发出
    //   0 = 要发送的数据会环回到MAC 并通过双绞线接口发出
    enc28j60_phy_write(PHCON2, PHCON2_HDLDIS);

    // 允许链路中断
    enc28j60_phy_write(PHIE, PHIE_PLNKIE|PHIE_PGEIE);
    
    //EIE： 以太网中断允许寄存器
    //bit 7 INTIE： 全局INT 中断允许位
    //  1 = 允许中断事件驱动INT 引脚
    //  0 = 禁止所有INT 引脚的活动（引脚始终被驱动为高电平）
    //bit 6 PKTIE： 接收数据包待处理中断允许位
    //  1 = 允许接收数据包待处理中断
    //  0 = 禁止接收数据包待处理中断
    enc28j60_reg_write(EIE, EIE_INTIE|EIE_PKTIE|EIE_LINKIE|EIE_TXIE|EIE_TXERIE|EIE_RXERIE);
    //enc28j60_reg_write(EIE, EIE_INTIE|EIE_PKTIE);

    // ECON1 寄存器
    // 寄存器3-1 所示为ECON1 寄存器，它用于控制
    // ENC28J60 的主要功能。 ECON1 中包含接收使能、发
    // 送请求、DMA 控制和存储区选择位。
    // enable packet reception
    //bit 2 RXEN：接收使能位
    //1 = 通过当前过滤器的数据包将被写入接收缓冲器
    //0 = 忽略所有接收的数据包
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
    //PHLCON：PHY 模块LED 控制寄存器	
    enc28j60_phy_write(PHLCON, 0x0476);
    // 清除所有中断标记位
    enc28j60_reg_write(EIR, 0);
    enc28j60_phy_read(PHIR); 

    // 如果接线了，查看多久才能检测到
    //uint32_t start_ms = htd_time_ms_get();
    //while (enc28j60_check_link_status() == false) {
    //    htd_delay_ms(1);
    //}
    //print_debug("waite ms: %d", htd_time_ms_diff(htd_time_ms_get(), start_ms));
    
    // 判断网线是否已经连接
    print_debug("ethernet is link: %d", enc28j60_check_link_status());
    print_debug("ENC28J60 Duplex:%s\r\n", drv_enc28j60_is_full_duplex() ? "Full Duplex" : "Half Duplex");

    uint8_t macaddr[6];
    drv_enc28j60_mac_addr_get(macaddr);
    if (memcmp(macaddr, p_macaddr, sizeof(macaddr))) {
        print_error("mac address set failed. %02x%02x%02x%02x%02x%02x", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    }
    drv_enc28j60_dump();
    // 如果接线了，等待稳定
    htd_delay_ms(80);
    return 0;
}

// @brief 关闭
int32_t drv_enc28j60_close(void)
{
    if (m_enc28j60_bus == NULL) {
        return 0;
    }
    
    // 禁用中断引脚
    enc28j60_int_pin_enable(false);
    // 软件掉电
    enc28j60_op_write(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
    while (enc28j60_reg_read(ESTAT) & ESTAT_RXBUSY);
    while (enc28j60_reg_read(ECON1) & ECON1_TXRTS);
    enc28j60_op_write(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
    
    // SPI 关闭
    htd_spim_close(m_enc28j60_bus);
    m_enc28j60_bus = NULL;
    // 掉电引脚控制
    enc28j60_power_pin_enable(false);
    print_warning("close");
    return 0;
}

// @brief 控制句柄
static int32_t drv_enc28j60_ctrl_handle(net_dev_cmd_t cmd, void* p_param, uint32_t param_len)
{
    switch (cmd) {
        case NET_DEV_CMD_INDICATE_HANDLE_SET: {     // 设置通告句柄
            m_network_indicate_handle = (net_dev_indicate_handle_t)p_param;
            break;
        }
        case NET_DEV_CMD_CLOSE: {                   // 关闭
            return drv_enc28j60_close();
        }
        case NET_DEV_CMD_LED1_ONOFF: {              // 控制 LED 闪烁
            uint8_t onoff = UINT8_VAL(p_param);
            if (onoff) {
                enc28j60_phy_write(PHLCON, 0x3880);
            } else {
                enc28j60_phy_write(PHLCON, 0x3990);
            }
            break;
        }
        case NET_DEV_CMD_MAC_ADDR_GET: {            // 读取 mac 地址
            drv_enc28j60_mac_addr_get((uint8_t*)p_param);
            return 6;
        }
        case NET_DEV_CMD_DUMP: {                    // 显示信息
            drv_enc28j60_dump();
            return 0;
        }
        default: {
            return -1;
        }
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t network_dev_enc28j60_register(void)
{
    const static dev_network_t network_dev = {
        .p_name = "enc28j60",
        .write_handle = drv_enc28j60_packet_write,
        .read_handle = drv_enc28j60_packet_read,
        .ctrl_handle = drv_enc28j60_ctrl_handle,
        .open_handle = drv_enc28j60_open,
    };
    return htd_network_dev_register(&network_dev);
}
DEVICE_INIT_FUNC_REGISTER(network_dev_enc28j60_register);


