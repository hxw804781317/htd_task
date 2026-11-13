#ifndef _HTD_SFW_AES128_H_
#define _HTD_SFW_AES128_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.
#ifndef CBC
  #define CBC 0
#endif

#ifndef ECB
  #define ECB 1
#endif


#if defined(ECB) && ECB
// @brief 加密数据，nopadding
void aes128_ecb_encrypt(uint8_t* input, const uint8_t* key, uint8_t *output);
// @brief 解密数据，zeropadding
void aes128_ecb_decrypt(uint8_t* input, const uint8_t* key, uint8_t *output);
#endif // #if defined(ECB) && ECB


#if defined(CBC) && CBC
void AES128_CBC_encrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void AES128_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
#endif // #if defined(CBC) && CBC


#ifdef __cplusplus
}
#endif
#endif //_HTD_SFW_AES128_H_
