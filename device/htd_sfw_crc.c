#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "sfw.crc"
#include "htd_sfw_crc.h"
#include "htd_device.h"


//////////////////////////////////////////////////////////////////////
// @brief  CRC-4/ITU           x4+x+1
// @param crcval 初始化值为 0x00
// Poly: 0x03
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc4_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x0C;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief CRC-5/EPC           x5+x3+1
// @param crcval 初始化值为 0x09
// Poly: 0x09
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc5_epc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 0x80) {
                crcval = (crcval << 1) ^ 0x48;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval >> 3;
}

// @brief CRC-5/ITU  x5+x4+x2+1
// @param crcval 初始化值为 0x00
// Poly: 0x15
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc5_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x15;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief CRC-5/USB x5+x2+1
// @param crcval 初始化值为 0x1F
// Poly: 0x05
// Refin: True
// Refout:True
// Xorout: 0x1F
uint8_t htd_sfw_crc5_usb(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    if (crcval != 0x1f) {
        crcval ^= 0x1f;
    }
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x14;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval ^ 0x1F;
}

// @brief CRC-6/ITU  x6+x+1
// @param crcval 初始化值为0
// Poly: 0x03
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc6_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x30;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief CRC-7/MMC  x7+x3+1
// @param crcval 初始化值为0
// Poly: 0x09
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc7_mmc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while(data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 0x80) {
                crcval = (crcval << 1) ^ 0x12; 
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval >> 1;
}


//////////////////////////////////////////////////////////////////////
// @brief CRC8(x8+x2+x+1)
// @param crcval 初始化值为0
// Poly: 0x07
// Refin: False
// Refout: False
// Xorout: 0x00
uint8_t htd_sfw_crc8(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 0x80) {
                crcval = (crcval<<1) ^ 0x07;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval;
}

// @brief CRC8_ITU 也叫CRC8_ATM, 
// @param crcval 初始化值为0
// Poly: 0x07
// Refin: False
// Refout: False
// Xorout: 0x55
// Alias: CRC-8/ATM
uint8_t htd_sfw_crc8_itu(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    if (crcval != 0) {
        crcval ^= 0x55;
    }
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 0x80) {
                crcval = (crcval << 1) ^ 0x07;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval ^ 0x55;
}

// @brief CRC8_ROHC
// @param crcval 初始化值为0xff
// Poly: 0x07
// Refin: True
// Refout: True
// Xorout: 0x00
uint8_t htd_sfw_crc8_rohc(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xE0; 
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief: CRC8_MAXIM         x8+x5+x4+1
// @param crcval 初始化值为 0
// Poly: 0x31
// Refin: True
// Refout: True
// Xorout: 0x00
// Alias: DOW-CRC,CRC-8/IBUTTON
uint8_t htd_sfw_crc8_maxim(const uint8_t *p_data, uint32_t data_len, uint8_t crcval)
{
#if 1
    const unsigned char crc8_table_info[]={
        0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
        0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
        0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
        0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
        0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
        0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
        0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
        0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
        0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
        0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
        0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
        0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
        0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
        0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
        0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
        0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
    };
    uint8_t index;
    while (data_len--) {
        index = (*p_data++) ^ crcval;
        crcval = crc8_table_info[index];
    }
    return (crcval);
#else
    int i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 0x01) {
                crcval = (crcval >> 1) ^ 0x8c;
            } else {
                crcval >>= 1;
            }
        }
    }
    return crcval; 
#endif    
}



//////////////////////////////////////////////////////////////////////
// @brief CRC-16/IBM          x16+x15+x2+1
// @param crcval 初始化值为 0
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0x0000
// Alias: CRC-16,CRC-16/ARC,CRC-16/LHA
uint16_t htd_sfw_crc16_ibm(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xA001; 
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief CRC-16/MAXIM  x16+x15+x2+1
// @param crcval 初始化值为 0
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_maxim(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    if (crcval != 0) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xA001; 
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return ~crcval; 
}

// @brief CRC-16/USB          x16+x15+x2+1
// @param crcval 初始化值为 0xffff
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_usb(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    if (crcval != 0xffff) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xA001;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return ~crcval;
}

// @brief CRC-16/MODBUS x16+x15+x2+1
// @param crcval 初始化值为 0xffff
// Poly: 0x8005
// Refin: True
// Refout: True
// Xorout: 0x0000
uint16_t htd_sfw_crc16_modbus(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
#if 0
    static const unsigned char CRC16Hi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
    };
    static const unsigned char CRC16Lo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 
        0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 
        0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10, 
        0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
        0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 
        0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 
        0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
        0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 
        0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 
        0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 
        0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40
    };
    unsigned char ucCRCHi;
    unsigned char ucCRCLo;
    unsigned char iIndex;

    ucCRCHi = (unsigned char)((crcval) >> 8);
    ucCRCLo = (unsigned char)(crcval);
    while (data_len--) {
        iIndex = ucCRCLo ^ (*(p_data++));
        ucCRCLo = (unsigned char)(ucCRCHi ^ CRC16Hi[iIndex]);
        ucCRCHi = CRC16Lo[iIndex];
    }
    return (uint16_t)(ucCRCHi << 8 | ucCRCLo);
#else
    uint32_t i;
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xA001;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
#endif
}

// @brief 直接计算 crc16/CCITT        x16+x12+x5+1
// @param crcval初始化值 0
// Poly: 0x1021
// Refin: True
// Refout: True
// Xorout: 0x0000
// Alias: CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT
uint16_t htd_sfw_crc16_ccitt(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint8_t i;
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x8408;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return crcval;
}

// @brief 直接计算 crc16/CCITT-FALSE，并返回结果; 
// @param crcval初始化值 0xffff
// Poly: 0x1021
// Refin: False
// Refout: False
// Xorout: 0x0000
uint16_t htd_sfw_crc16_ccitt_false(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    for (uint32_t i=0; i<data_len; i++) {
        crcval  = (uint8_t)(crcval >> 8) | (crcval << 8);
        crcval ^= p_data[i];
        crcval ^= (uint8_t)(crcval & 0xFF) >> 4;
        crcval ^= (crcval << 8) << 4;
        crcval ^= ((crcval & 0xFF) << 4) << 1;
    }
    return crcval;
}

// @brief 直接计算 crc16/X25          x16+x12+x5+1
// @param crcval 初始化值为 0xFFFF
// Poly: 0x1021
// Refin: True
// Refout: True
// Xorout: 0XFFFF
uint16_t htd_sfw_crc16_x25(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    if (crcval != 0xffff) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0x8408;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return ~crcval; 
}

// @brief 直接计算 CRC-16/XMODEM       x16+x12+x5+1
// @param crcval 初始化值为 0x0000
// Poly: 0x1021
// Refin: False
// Refout: False
// Xorout: 0x0000
// Alias: CRC-16/ZMODEM,CRC-16/ACORN
uint16_t htd_sfw_crc16_xmodem(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= (uint16_t)(*p_data++) << 8; 
        for (i=0; i<8; i++) {
            if (crcval & 0x8000) {
                crcval = (crcval << 1) ^ 0x1021;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval;
}

// @brief 直接计算 CRC-16/DNP      x16+x13+x12+x11+x10+x8+x6+x5+x2+1
// @param crcval 初始化值为 0x0000
// Poly: 0x3D65
// Refin: True
// Refout: True
// Xorout: 0xFFFF
uint16_t htd_sfw_crc16_dnp(const uint8_t *p_data, uint32_t data_len, uint16_t crcval)
{
    uint32_t i;
    if (crcval != 0) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval ^= *p_data++;
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xA6BC;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return ~crcval; 
}


//////////////////////////////////////////////////////////////////////
// @brief 直接计算 CRC-32  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
// @param crcval 初始化值为 0xFFFFFFF
// Poly: 0x4C11DB7
// Refin: True
// Refout: True
// Xorout: 0xFFFFFFF
// Alias:  CRC_32/ADCCP
uint32_t htd_sfw_crc32(const uint8_t *p_data, uint32_t data_len, uint32_t crcval)
{
#if 1
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };
    if (crcval != 0xffffffff) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval = (crcval >> 4) ^ rtable[(crcval ^ (p_data[0] >> 0)) & 0xf];
        crcval = (crcval >> 4) ^ rtable[(crcval ^ (p_data[0] >> 4)) & 0xf];
        p_data++;
    }
    return ~crcval;
#else
    uint32_t i;
    if (crcval != 0xffffffff) {
        crcval = ~crcval;
    }
    while (data_len--) {
        crcval ^= *p_data++; 
        for (i=0; i<8; i++) {
            if (crcval & 1) {
                crcval = (crcval >> 1) ^ 0xEDB88320;
            } else {
                crcval = (crcval >> 1);
            }
        }
    }
    return ~crcval;
#endif
}

// @brief 直接计算 CRC-32/MPEG-2  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
// @param crcval 初始化值为 0xFFFFFFF
// Poly: 0x4C11DB7
// Refin: False
// Refout: False
// Xorout: 0x0000000
uint32_t htd_sfw_crc32_mpeg2(const uint8_t *p_data, uint32_t data_len, uint32_t crcval)
{
    uint32_t i;
    while (data_len--) {
        crcval ^= (uint32_t)(*p_data++) << 24;
        for (i=0; i<8; i++) {
            if (crcval & 0x80000000) {
                crcval = (crcval << 1) ^ 0x04C11DB7;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval;
}



