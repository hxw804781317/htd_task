/**
 *  _____              _____ _     _   _ 
 * |  ___|            |_   _| |   | | | |
 * | |__  __ _ ___ _   _| | | |   | | | |
 * |  __|/ _` / __| | | | | | |   | | | |
 * | |__| (_| \__ \ |_| | | | |___\ \_/ /
 * \____/\__,_|___/\__, \_/ \_____/\___/ 
 *                  __/ |                
 *                 |___/                 
 *
 * This library implements the ASN1 Tag-Length-Value encoding standard. For
 * more information about this standard, see the ISO8825-1 Basic Encoding
 * Rules: https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 * 
 * Note: This library does not implement indefinite length TLV objects, as
 * described in section 8.1.3
 */

/**
 * MIT License
 *
 * Copyright (c) 2020 John Boyd <johnboydiv@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "easytlv.h"
#include <stdint.h>
#include "htd_task.h"

void easytlv_print_hex(const void* src, int len)
{
    if(!src || len < 0) {
        return;
    }

    const uint8_t* s = src;
    htd_print("(%i) ", len);
    while(len-- > 0) {
        htd_print("%02x", *s++);
    }
}

#define ETLV_LOG_HEX(...)   easytlv_print_hex(__VA_ARGS__)
#define ETLV_LOG(...)       htd_print("<etlv debug> " __VA_ARGS__)
#define ETLV_LOG_LINE()     htd_print("\n\r")

#define GET_MSBYTE(i) ((i >> 24) & 0xFF)

// Compute the minimum number of bytes required to represent a number
static inline uint8_t min_size(uint32_t d)
{
    uint8_t n = 0;
    while (d) {
        d >>= 8;
        n++;
    }
    return n;
}

// Left-shift a number, until it has no leading zeros
static inline uint32_t trim_leading_zeros(uint32_t i)
{
    while (i <= 0x00FFFFFF)
        i <<= 8;
    return i;
}

// Read the tag field from the source buffer and decode it
// Modifies src to point to next byte after tag bytes
// Returns tag, or negative error
static inline int decode_tag(uint32_t* tag, const uint8_t** src, int srcLen)
{
    if (!tag || !src || !*src) {
        return ETLV_ERR_BADARG;
    }
    if (srcLen <= 0) {
        return ETLV_ERR_NODATA;
    }

    const uint8_t* s = *src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;
    *src = 0; // Reset src to null

    if ((*s & 0x1F) > 30) { // Is the tag extended?
        // First subsequent octet must not be 0
        if (srcLen < 2 || *(s+1) == 0) {
            return ETLV_ERR_INVAL;
        }
        *tag = *s++;
        do {
            // Detect overflow
            if ((*tag << 8) < *tag) {
                return ETLV_ERR_OVERFLOW;
            }
            // Join each octet of the tag
            *tag = (*tag << 8) + (*s & 0xFF);
        } while (s < END && *s++ & 0x80); // Last octet will clear MSB

        if (s > END) {
            return ETLV_ERR_MSGSIZE;
        }
        *src = s;
    } else { // Tag is just one byte
        *src = s+1;
        *tag = *s;
    }
    return s-BEGIN;
}

// Encode the tag and Write it to the destination buffer
// Modifies dest to point to next byte after tag bytes
// Returns number of bytes written, or negative error
static inline int encode_tag(uint8_t** dest, int destLen, uint32_t tag)
{
    if (!dest || !*dest) {
        return ETLV_ERR_BADARG;
    }
    if (destLen <= 0) {
        return ETLV_ERR_NOMEM;
    }
    
    uint8_t* d = *dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + destLen;
    *dest = 0; // Reset dest to null

    if (/*tag < 0 ||*/ tag > 0xFF) { // Extended tag
        // Scan to first tag byte
        tag = trim_leading_zeros(tag);
        
        // Check first byte is valid extended byte
        if ((GET_MSBYTE(tag) & 0x1F) < 31) {
            return ETLV_ERR_INVAL;
        }
        // Write tag bytes
        while (tag > 0x00FFFFFF) {
            // Check for enough memory
            if (d >= END) {
                return ETLV_ERR_NOMEM;
            }
            // Copy next tag byte
            *d++ = GET_MSBYTE(tag);
            tag <<= 8;
        }
    } else { // Short tag
        if ((tag & 0x1F) > 30) {
            return ETLV_ERR_INVAL; // Invalid short tag
        }
        // Check for end of buffer
        if (d >= END) {
            return ETLV_ERR_NOMEM;
        }
        // Write tag byte
        *d++ = tag & 0xFF;
    }
    *dest = d;
    return d-BEGIN; 
}

// Read the length field from the source buffer and decode it
// Modifies src to point to next byte after length bytes
// Returns number of bytes read or negative error
static inline int decode_length(uint32_t* length, const uint8_t** src, int srcLen)
{
    if (!length || !src || !*src)
        return ETLV_ERR_BADARG;
    if (srcLen <= 0)
        return ETLV_ERR_NODATA;

    const uint8_t* s = *src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;
    *src = (void*) 0; // Reset src to null

    if (*s & 0x80) { // Is the length in long form?
        // The first byte must not be 0xFF
        if (*s == 0xFF)
            return ETLV_ERR_INVAL;

        const int N = *s++ & 0x7F; // Number of length bytes to use
        if (N > 4 || (N > 3 && s[3] & 0x80))
            return ETLV_ERR_OVERFLOW;

        int len = 0;
        for(int i = 0; i < N; i++)
            len = (len << 8) + s[i];
        *src = s+N;
        *length = len;
    } else { // Tag is just one byte
        *src = s+1;
        *length = *s;
    }

    if (s >= END)
        return ETLV_ERR_MSGSIZE;

    return s-BEGIN;
}

// Encode the length and Write it to the destination buffer
// Modifies dest to point to next byte after length bytes
// Returns number of bytes written, or negative error
static inline int encode_length(uint8_t** dest, int destLen, uint32_t length)
{
    if (!dest || !*dest || length <= 0)
        return ETLV_ERR_BADARG;
    if (destLen <= 0)
        return ETLV_ERR_NOMEM;

    uint8_t* d = *dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + destLen;
    *dest = 0; // Reset dest to null

    if (length > 0x7F) { // Long form
        // Determine how many length bytes
        int n = min_size(length);
        if (n > 0x7E) // This long cannot fit in TLV encoding
            return ETLV_ERR_OVERFLOW;
        if (d >= END)
            return ETLV_ERR_NOMEM;
        *d++ = (n | 0x80) & 0xFF; 

        // Write length bytes
        length = trim_leading_zeros(length);
        while (n-- > 0) {
            if (d >= END)
                return ETLV_ERR_NOMEM;

            *d++ = GET_MSBYTE(length);
            length <<= 8;
        }
    } else { // Short form
        if (d >= END)
            return ETLV_ERR_NOMEM;
        *d++ = length & 0xFF;
    }

    *dest = d;
    return d-BEGIN;
}

int etlv_parse(ETLVToken* t, int* nTok, const void* src, int srcLen)
{
    if (!t || !nTok || *nTok < 0 || !src || srcLen < 0)
        return ETLV_ERR_BADARG;

    const uint8_t* s = src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;

    ETLV_LOG("Parse input: ");
    ETLV_LOG_HEX(src, srcLen);
    ETLV_LOG_LINE();

    int n = 0;
    int err = 0;
    while (s < END) {
        // Check for memory
        if (n >= *nTok) {
            /* it's normal to read required number of tokens and doesn't reach buffer end */
            //n = ETLV_ERR_NOMEM;
            break;
        }

        // Decode the tag field
        err = decode_tag(&t[n].tag, &s, END-s);
        if (err < 0) {
            n = err;
            break;
        }
        ETLV_LOG("tag: %08X\n\r", t[n].tag);

        // Decode the length field
        err = decode_length(&t[n].len, &s, END-s);
        if (err < 0) {
            n = err;
            break;
        }
        ETLV_LOG("len: %u\n\r", t[n].len);

        // Save pointer to value field
        t[n].val = s;
        if (t[n].val == 0) {
            n = ETLV_ERR_UNKNOWN;
            break;
        }
        ETLV_LOG("val: ");
        ETLV_LOG_HEX(t[n].val, t[n].len);
        ETLV_LOG_LINE();

        // Point to next object
        s = (uint8_t *)t[n].val + t[n].len; 
        n++;
    }

    // Output the number of tokens found
    if (n >= 0)
        *nTok = n;

    // Handle errors
    if (n < 0) // Error during parse loop
        return n;
    if (s > END) // TLV data exceeds byte array provided
        return ETLV_ERR_MSGSIZE;

    // Return the total length of the TLV data
    return s-BEGIN;
}

int etlv_serialize(void* dest, int* len, const ETLVToken* t, int nTok)
{
    if (!dest || !len || *len < 0 || !t || nTok < 0)
        return ETLV_ERR_BADARG;

    uint8_t* d = dest;
    const uint8_t* const BEGIN = d;
    const uint8_t* const END = d + *len;

    ETLV_LOG("Serializing %d tokens into %i byte buffer\n\r", nTok, *len);

    *len = 0;
    int err = 0;
    for (int i = 0; i < nTok; i++) {
        // Write the tag field
        ETLV_LOG("tag: %08X\n\r", t[i].tag);
        err = encode_tag(&d, END-d, t[i].tag);
        if (err < 0)
            return err;
        if (*len + err < *len)
            return ETLV_ERR_OVERFLOW;

        // Write the length field
        ETLV_LOG("len: %u\n\r", t[i].len);
        err = encode_length(&d, END-d, t[i].len);
        if (err < 0)
            return err;
        if (*len + err < *len)
            return ETLV_ERR_OVERFLOW;

        // Copy the value
        ETLV_LOG("val: ");
        ETLV_LOG_HEX(t[i].val, t[i].len);
        ETLV_LOG_LINE();
        const uint8_t* v = t[i].val;
        const uint8_t* const VAL_END = v + t[i].len;
        while (v < VAL_END) {
            // Make sure not to overrun the buffer
            if (d >= END)
                return ETLV_ERR_NOMEM;

            *d++ = *v++;
        }
    }

    // Return total serialized length
    return *len = d-BEGIN;
}

int etlv_find(ETLVToken* t, uint32_t tag, const void* src, int srcLen)
{
    if (!t || !src || srcLen < 0)
        return ETLV_ERR_BADARG;

    const uint8_t* s = src;
    const uint8_t* const BEGIN = s;
    const uint8_t* const END = s + srcLen;

    int err;
    uint32_t found;
    uint32_t len;
    int offset = 0;
    while (s < END) {
        offset = s-BEGIN;
        err = decode_tag(&found, &s, END-s);
        if (err < 0)
            break;
        ETLV_LOG("Found tag: 0x%08X\n\r", found);
        err = decode_length(&len, &s, END-s);
        if (err < 0)
            break;
        if (found == tag)
            break;
        s += len;
    }

    if (err < 0)
        return err;

    if (s >= END)
        return ETLV_ERR_NOENT;

    // Output a token for the found tag
    t->tag = found;
    t->len = len;
    t->val = s;

    // Return the byte offset of the found token
    return offset;
}




//////////////////////////////////////////////////////////////////////
#if (1 && defined(RT_USING_FINSH))
#include <finsh.h>

// Example TLV data, encoding two 32 bit numbers:
const uint8_t testDataShort[] = {
    0x02, 0x04, 0x00, 0x00, 0x00, 0x2A, // Number 42
    0x02, 0x04, 0x00, 0x00, 0x01, 0x01, // Number 257
};

const uint8_t testDataLong[] = {
    0x1F, 0x88, 0x01,   // Extended tag
    0x82, 0x01, 0x01,   // Extended length: 257 bytes
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0x01,
    0x02,		// Short tag
    0x04,		// Short length: 4 bytes
    0x00, 0x00, 0x01, 0x01,
};

static inline int is_big_endian(void)
{
    int i=1;
    return ! *((char *)&i);
}

static inline uint32_t tlv_ntohl(uint32_t num)
{
    if (is_big_endian()) {
        return num;
    } else {    
        return  ((num>>24)&0xff) |      // move byte 3 to byte 0
                ((num<<8)&0xff0000) |   // move byte 1 to byte 2
                ((num>>8)&0xff00) |     // move byte 2 to byte 1
                ((num<<24)&0xff000000); // byte 0 to byte 3
    }
}

int easytlv_test_cmd(int argc, void** argv)
{
    uint8_t tlvRaw[sizeof(testDataLong)];
    uint32_t numChk;

    htd_print("\n\n\r-------------------- EasyTLV Test --------------------\n");

    // ---- TEST ON SHORT DATA ----
    htd_print("Parse test (SHORT DATA)\n\r");

    // Parse the two tokens
    ETLVToken t[2];
    int nTok = sizeof(t)/sizeof(t[0]);
    int err = etlv_parse(t, &nTok, testDataShort, sizeof(testDataShort));
    htd_print(" - result: %i\n\r", err);
    ASSERT(err >= 0);

    // Cast the numbers to uint32_t's and compare
    numChk = tlv_ntohl(*(uint32_t *)t[0].val);
    htd_print("[0]tag=%d, len=%d, val=%d\r\n", t[0].tag, t[0].len, numChk);
    numChk = tlv_ntohl(*(uint32_t *)t[1].val);
    htd_print("[1]tag=%d, len=%d, val=%d\r\n", t[1].tag, t[1].len, numChk);
    htd_print(" - TEST PASS\n\r");


    htd_print("Serialization test (SHORT DATA)\n\r");
    htd_print(" - re-serializing same data as before\n\r");
    // Re-serialize the parsed data and compare to original
    int tlvBufSz = sizeof(tlvRaw);
    err = etlv_serialize(tlvRaw, &tlvBufSz, t, nTok);
    htd_print(" - result: %i\n\r", err);
    ASSERT(err >= 0);
    ASSERT(0 == memcmp(tlvRaw, testDataShort, tlvBufSz));
    htd_print(" - TEST PASS\n\r");

    // ---- TEST ON LONG DATA ----
    htd_print("Parse test (LONG DATA)\n\r");

    // Parse the two tokens
    nTok = sizeof(t)/sizeof(t[0]);
    err = etlv_parse(t, &nTok, testDataLong, sizeof(testDataLong));
    htd_print(" - result: %i\n\r", err);
    ASSERT(err >= 0);

    // Check the parsed tokens
    ASSERT(t[0].tag == 0x001F8801);
    ASSERT(t[0].len == 257);
    ASSERT(t[0].val == testDataLong + 3 + 3);
    numChk = tlv_ntohl(*(uint32_t *)t[1].val);
    ASSERT(numChk == 257);
    htd_print(" - TEST PASS\n\r");

    htd_print("Serialization test (LONG DATA)\n\r");
    htd_print(" - re-serializing same data as before\n\r");

    // Re-serialize the parsed data and compare to original
    tlvBufSz = sizeof(tlvRaw);
    err = etlv_serialize(tlvRaw, &tlvBufSz, t, nTok);
    htd_print(" - result: %i\n\r", err);
    ASSERT(err >= 0);
    ASSERT(0 == memcmp(tlvRaw, testDataLong, tlvBufSz));
    htd_print(" - TEST PASS\n\r");

    // Search for both tags
    htd_print("Tag search test (LONG DATA -- first tag)\n\r");
    ETLVToken needle;
    err = etlv_find(&needle, 0x001F8801, testDataLong, sizeof(testDataLong));
    htd_print(" - result: %i\n\r", err);
    ASSERT(err == 0);
    ASSERT(0 == memcmp(&t[0], &needle, sizeof(needle)));
    htd_print(" - TEST PASS\n\r");
    htd_print("Tag search test (LONG DATA -- second tag)\n\r");
    err = etlv_find(&needle, 0x02, testDataLong, sizeof(testDataLong));
    htd_print(" - result: %i\n\r", err);
    ASSERT(err == 263);
    ASSERT(0 == memcmp(&t[1], &needle, sizeof(needle)));
    htd_print(" - TEST PASS\n\r");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(easytlv_test_cmd, easytlv_test, easytlv test);
#endif /**/

