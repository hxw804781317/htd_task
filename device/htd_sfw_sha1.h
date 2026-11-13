/**
 * \file sha1.h
 *
 * \brief SHA-1 cryptographic hash function
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#ifndef _HTD_SFW_SHA1_H_
#define _HTD_SFW_SHA1_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/**
 * \brief          SHA-1 context structure
 */
typedef struct {
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[5];          /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */
}htd_sha1_ctx_t;


void htd_sfw_sha1_starts(htd_sha1_ctx_t *ctx);
/**
 * \brief          SHA-1 process buffer
 *
 * \param ctx      SHA-1 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void htd_sfw_sha1_update(htd_sha1_ctx_t *ctx, const unsigned char *input, size_t ilen);
/**
 * \brief          SHA-1 final digest
 *
 * \param ctx      SHA-1 context
 * \param output   SHA-1 checksum result
 */
void htd_sfw_sha1_finish(htd_sha1_ctx_t *ctx, unsigned char output[20]);




void hmac_sha1(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[20]);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_SFW_SHA1_H_*/

