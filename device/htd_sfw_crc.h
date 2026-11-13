#ifndef _HTD_SFW_CRC_H_
#define _HTD_SFW_CRC_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// @brief CRC-4/ITU         x4+x+1
// @param crcval 初始化值为 0x00
// Poly: 0x03
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc4_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC-5/EPC         x5+x3+1
// @param crcval 初始化值为 0x09
// Poly: 0x09
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc5_epc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC-5/ITU         x5+x4+x2+1
// @param crcval 初始化值为 0x00
// Poly: 0x15
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc5_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC-5/USB         x5+x2+1
// @param crcval 初始化值为 0x1F
// Poly: 0x05
// Refin: True
// Refout:True
// Xorout: 0x1F
uint8_t htd_sfw_crc5_usb(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC-6/ITU         x6+x+1
// @param crcval 初始化值为0
// Poly: 0x03
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc6_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC-7/MMC         x7+x3+1
// @param crcval 初始化值为0
// Poly: 0x09
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc7_mmc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);





//////////////////////////////////////////////////////////////////////
// @brief CRC8              x8+x2+x+1)
// @param crcval 初始化值为0
// Poly: 0x07
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc8(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC8_ITU 
// @param crcval 初始化值为 0
// Poly: 0x07
// Refin: False
// Refout: False
// Xorout: 0x55
// Alias: CRC-8/ATM
uint8_t htd_sfw_crc8_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief CRC8_ROHC
// @param crcval 初始化值为0xff
// Poly: 0x07
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc8_rohc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);
// @brief: CRC8_MAXIM         x8+x5+x4+1
// @param crcval 初始化值为 0
// Poly: 0x31
// Refin: True
// Refout: True
// Xorout: 0x00
// Alias: DOW-CRC,CRC-8/IBUTTON
uint8_t htd_sfw_crc8_maxim(const uint8_t *p_data, uint32_t data_len, uint8_t crcval);




//////////////////////////////////////////////////////////////////////
// @brief CRC-16/IBM          x16+x15+x2+1
// @param crcval 初始化值为 0
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0x0000
// Alias: CRC-16,CRC-16/ARC,CRC-16/LHA
uint16_t htd_sfw_crc16_ibm(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief CRC-16/MAXIM  x16+x15+x2+1
// @param crcval 初始化值为 0
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_maxim(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief CRC-16/USB          x16+x15+x2+1
// @param crcval 初始化值为 0xffff
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_usb(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief CRC-16/MODBUS x16+x15+x2+1
// @param crcval 初始化值为 0xffff
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0x0000
uint16_t htd_sfw_crc16_modbus(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief 直接计算 crc16/CCITT        x16+x12+x5+1
// @param crcval初始化值 0
// Poly: 0x1021
// Refin: True
// Refout: True
// Xorout: 0x0000
// Alias: CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT
uint16_t htd_sfw_crc16_ccitt(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief 直接计算 crc16/CCITT-FALSE，并返回结果; 
// @param crcval初始化值0xffff
// Poly: 0x1021
// Refin: False
// Refout: False
// Xorout: 0x0000
uint16_t htd_sfw_crc16_ccitt_false(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief 直接计算 crc16/X25          x16+x12+x5+1
// @param crcval 初始化值为 0xFFFF
// Poly: 0x1021
// Refin: True
// Refout: True
// Xorout: 0XFFFF
uint16_t htd_sfw_crc16_x25(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief 直接计算 CRC-16/XMODEM       x16+x12+x5+1
// @param crcval 初始化值为 0x0000
// Poly: 0x1021
// Refin: False
// Refout: False
// Xorout: 0x0000
// Alias:   CRC-16/ZMODEM,CRC-16/ACORN
uint16_t htd_sfw_crc16_xmodem(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);
// @brief 直接计算 CRC-16/DNP          x16+x13+x12+x11+x10+x8+x6+x5+x2+1
// @param crcval 初始化值为 0x0000
// Poly: 0x3D65
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_dnp(const uint8_t *p_data, uint32_t data_len, uint16_t crcval);





//////////////////////////////////////////////////////////////////////
// @brief 直接计算 CRC-32  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
// @param crcval 初始化值为 0xFFFFFFF
// Poly: 0x4C11DB7
// Refin: True
// Refout: True
// Xorout: 0xFFFFFFF
// Alias: CRC_32/ADCCP
uint32_t htd_sfw_crc32(const uint8_t *p_data, uint32_t data_len, uint32_t crcval);
// @brief 直接计算 CRC-32/MPEG-2  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
// @param crcval 初始化值为 0xFFFFFFF
// Poly: 0x4C11DB7
// Refin: False
// Refout: False
// Xorout: 0x0000000
uint32_t htd_sfw_crc32_mpeg2(const uint8_t *p_data, uint32_t data_len, uint32_t crcval);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_SFW_CRC_H_*/

