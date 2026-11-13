#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.sha256"
#include "htd_common.h"
#include "htd_sfw_sha256.h"


//////////////////////////////////////////////////////////////////////
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32 - (b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))


static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


/**@brief Function for calculating the hash of a 64-byte section of data.
 *
 * @param[in,out] p_ctx   Hash instance.
 * @param[in]     p_data  Aray with data to be hashed. Assumed to be 64 bytes long.
 */
static void sha256_transform(htd_sha256_ctx_t *p_ctx, const uint8_t * p_data)
{
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = (p_data[j] << 24) | (p_data[j + 1] << 16) | (p_data[j + 2] << 8) | (p_data[j + 3]);
    }
    for ( ; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    a = p_ctx->state[0];
    b = p_ctx->state[1];
    c = p_ctx->state[2];
    d = p_ctx->state[3];
    e = p_ctx->state[4];
    f = p_ctx->state[5];
    g = p_ctx->state[6];
    h = p_ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    p_ctx->state[0] += a;
    p_ctx->state[1] += b;
    p_ctx->state[2] += c;
    p_ctx->state[3] += d;
    p_ctx->state[4] += e;
    p_ctx->state[5] += f;
    p_ctx->state[6] += g;
    p_ctx->state[7] += h;
}

// @brief 初始化
void htd_sfw_sha256_init(htd_sha256_ctx_t *p_ctx)
{
    ASSERT(p_ctx);
    p_ctx->datalen = 0;
    p_ctx->bitlen = 0;
    p_ctx->state[0] = 0x6a09e667;
    p_ctx->state[1] = 0xbb67ae85;
    p_ctx->state[2] = 0x3c6ef372;
    p_ctx->state[3] = 0xa54ff53a;
    p_ctx->state[4] = 0x510e527f;
    p_ctx->state[5] = 0x9b05688c;
    p_ctx->state[6] = 0x1f83d9ab;
    p_ctx->state[7] = 0x5be0cd19;
}

// @brief 更新 sha256 数据
void htd_sfw_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_ctx);
    ASSERT(p_data);
    ASSERT(data_len);

    while (data_len--) {
        p_ctx->data[p_ctx->datalen] = *p_data++;
        p_ctx->datalen++;
        if (p_ctx->datalen == 64) {
            sha256_transform(p_ctx, p_ctx->data);
            p_ctx->bitlen += 512;
            p_ctx->datalen = 0;
        }
    }
}

// @brief 获取 sha256 结果
void htd_sfw_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32])
{
    uint32_t i;

    ASSERT(p_ctx);
    ASSERT(output);

    i = p_ctx->datalen;
    // Pad whatever data is left in the buffer.
    if (p_ctx->datalen < 56) {
        p_ctx->data[i++] = 0x80;
        while (i < 56) {
            p_ctx->data[i++] = 0x00;
        }
    } else {
        p_ctx->data[i++] = 0x80;
        while (i < 64) {
            p_ctx->data[i++] = 0x00;
        }
        sha256_transform(p_ctx, p_ctx->data);
        memset(p_ctx->data, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    p_ctx->bitlen += (uint64_t)p_ctx->datalen * 8;
    p_ctx->data[63] = p_ctx->bitlen;
    p_ctx->data[62] = p_ctx->bitlen >> 8;
    p_ctx->data[61] = p_ctx->bitlen >> 16;
    p_ctx->data[60] = p_ctx->bitlen >> 24;
    p_ctx->data[59] = p_ctx->bitlen >> 32;
    p_ctx->data[58] = p_ctx->bitlen >> 40;
    p_ctx->data[57] = p_ctx->bitlen >> 48;
    p_ctx->data[56] = p_ctx->bitlen >> 56;
    sha256_transform(p_ctx, p_ctx->data);

#if 0
    for (i = 0; i < 4; ++i) {
        output[i]      = (p_ctx->state[7] >> (i * 8)) & 0x000000ff;
        output[i + 4]  = (p_ctx->state[6] >> (i * 8)) & 0x000000ff;
        output[i + 8]  = (p_ctx->state[5] >> (i * 8)) & 0x000000ff;
        output[i + 12] = (p_ctx->state[4] >> (i * 8)) & 0x000000ff;
        output[i + 16] = (p_ctx->state[3] >> (i * 8)) & 0x000000ff;
        output[i + 20] = (p_ctx->state[2] >> (i * 8)) & 0x000000ff;
        output[i + 24] = (p_ctx->state[1] >> (i * 8)) & 0x000000ff;
        output[i + 28] = (p_ctx->state[0] >> (i * 8)) & 0x000000ff;
    }
#else
    for (i = 0; i < 4; ++i) {
        output[i]      = (p_ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        output[i + 4]  = (p_ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        output[i + 8]  = (p_ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        output[i + 12] = (p_ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        output[i + 16] = (p_ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        output[i + 20] = (p_ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        output[i + 24] = (p_ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        output[i + 28] = (p_ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
#endif
}

