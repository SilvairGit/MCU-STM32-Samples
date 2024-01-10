#include "Checksum.h"

#include <stdbool.h>
#include <string.h>

#include "Assert.h"
#include "Utils.h"


#define CRC16_POLYNOMIAL 0x8005u
#define CRC32_POLYNOMIAL 0xEDB88320u

#define SHA256_CHUNK_SIZE 64
#define SHA256_TOTAL_LEN_LEN 8


static const uint32_t SHA256_K[] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
                                    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                                    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
                                    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static const uint32_t SHA256_H[] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};


struct SHA256State
{
    const uint8_t *p;
    size_t         len;
    size_t         total_len;
    bool           single_one_delivered;
    bool           total_len_delivered;
};


static uint16_t        CalcCRC16(uint8_t data, uint16_t crc);
static void            CalcSHA256(uint32_t hash[32], const void *input, size_t len);
static bool            CalcSHA256Chunk(uint8_t chunk[SHA256_CHUNK_SIZE], struct SHA256State *p_state);
static inline uint32_t CalcSHA256RightRotation(uint32_t value, unsigned int count);


uint16_t Checksum_CalcCRC16(uint8_t *p_data, size_t len, uint16_t init_val)
{
    uint16_t crc = init_val;
    size_t   i;
    for (i = 0; i < len; i++)
    {
        crc = CalcCRC16(p_data[i], crc);
    }

    return crc;
}

uint32_t Checksum_CalcCRC32(uint8_t *p_data, size_t len, uint32_t init_val)
{
    uint32_t crc = init_val;
    size_t   i;
    for (i = 0; i < len; i++)
    {
        crc = crc ^ p_data[i];
        size_t j;
        for (j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (CRC32_POLYNOMIAL & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}

void Checksum_CalcSHA256(uint8_t *p_data, size_t len, uint8_t *p_sha256)
{
    ASSERT(p_sha256 != NULL);
    CalcSHA256((uint32_t *)p_sha256, p_data, len);
}


static uint16_t CalcCRC16(uint8_t data, uint16_t crc)
{
    size_t i;
    for (i = 0; i < 8; i++)
    {
        if (((crc & 0x8000) >> 8) ^ (data & 0x80))
        {
            crc = (crc << 1) ^ CRC16_POLYNOMIAL;
        }
        else
        {
            crc = (crc << 1);
        }
        data <<= 1;
    }

    return crc;
}

static void CalcSHA256(uint32_t hash[32], const void *p_input, size_t len)
{
    size_t i, j;

    memcpy(hash, SHA256_H, 32);

    uint8_t chunk[64];

    struct SHA256State state = {
        .p                    = (uint8_t *)p_input,
        .len                  = len,
        .total_len            = len,
        .single_one_delivered = 0,
        .total_len_delivered  = 0,
    };

    while (CalcSHA256Chunk(chunk, &state))
    {
        uint32_t ah[8];

        uint32_t       w[64];
        const uint8_t *p = chunk;

        memset(w, 0x00, sizeof w);
        for (i = 0; i < 16; i++)
        {
            w[i] = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
            p += 4;
        }

        for (i = 16; i < 64; i++)
        {
            const uint32_t s0 = CalcSHA256RightRotation(w[i - 15], 7) ^ CalcSHA256RightRotation(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = CalcSHA256RightRotation(w[i - 2], 17) ^ CalcSHA256RightRotation(w[i - 2], 19) ^ (w[i - 2] >> 10);

            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        for (i = 0; i < 8; i++)
        {
            ah[i] = hash[i];
        }

        for (i = 0; i < 64; i++)
        {
            const uint32_t s1 = CalcSHA256RightRotation(ah[4], 6) ^ CalcSHA256RightRotation(ah[4], 11) ^ CalcSHA256RightRotation(ah[4], 25);

            const uint32_t ch    = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            const uint32_t temp1 = ah[7] + s1 + ch + SHA256_K[i] + w[i];
            const uint32_t s0    = CalcSHA256RightRotation(ah[0], 2) ^ CalcSHA256RightRotation(ah[0], 13) ^ CalcSHA256RightRotation(ah[0], 22);

            const uint32_t maj   = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            const uint32_t temp2 = s0 + maj;

            ah[7] = ah[6];
            ah[6] = ah[5];
            ah[5] = ah[4];
            ah[4] = ah[3] + temp1;
            ah[3] = ah[2];
            ah[2] = ah[1];
            ah[1] = ah[0];
            ah[0] = temp1 + temp2;
        }

        for (i = 0; i < 8; i++)
        {
            hash[i] += ah[i];
        }
    }

    for (i = 0, j = 0; i < 8; i++)
    {
        uint32_t word          = hash[i];
        ((uint8_t *)hash)[j++] = (uint8_t)(word >> 24);
        ((uint8_t *)hash)[j++] = (uint8_t)(word >> 16);
        ((uint8_t *)hash)[j++] = (uint8_t)(word >> 8);
        ((uint8_t *)hash)[j++] = (uint8_t)word;
    }
}

static bool CalcSHA256Chunk(uint8_t chunk[SHA256_CHUNK_SIZE], struct SHA256State *p_state)
{
    ASSERT(p_state != NULL);

    size_t space_in_chunk;

    if (p_state->total_len_delivered)
    {
        return false;
    }

    if (p_state->len >= SHA256_CHUNK_SIZE)
    {
        memcpy(chunk, p_state->p, SHA256_CHUNK_SIZE);
        p_state->p += SHA256_CHUNK_SIZE;
        p_state->len -= SHA256_CHUNK_SIZE;
        return true;
    }

    memcpy(chunk, p_state->p, p_state->len);
    chunk += p_state->len;
    space_in_chunk = SHA256_CHUNK_SIZE - p_state->len;
    p_state->p += p_state->len;
    p_state->len = 0;

    if (!p_state->single_one_delivered)
    {
        *chunk++ = 0x80;
        space_in_chunk -= 1;
        p_state->single_one_delivered = 1;
    }

    if (space_in_chunk >= SHA256_TOTAL_LEN_LEN)
    {
        const size_t left = space_in_chunk - SHA256_TOTAL_LEN_LEN;
        size_t       len  = p_state->total_len;
        int          i;
        memset(chunk, 0x00, left);
        chunk += left;

        chunk[7] = (uint8_t)(len << 3);
        len >>= 5;
        for (i = 6; i >= 0; i--)
        {
            chunk[i] = (uint8_t)len;
            len >>= 8;
        }
        p_state->total_len_delivered = 1;
    }
    else
    {
        memset(chunk, 0x00, space_in_chunk);
    }

    return true;
}

static inline uint32_t CalcSHA256RightRotation(uint32_t value, unsigned int count)
{
    return ((value >> count) | (value << (32 - count)));
}
