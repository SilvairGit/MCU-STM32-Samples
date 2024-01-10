#include "Checksum.h"
#include "unity.h"


void setUp(void)
{
}

void tearDown(void)
{
}

void test_Checksum_CalcCRC16_NullData(void)
{
    uint16_t checksum_init_val = 0xFFFF;
    uint8_t *p_data            = NULL;
    size_t   len               = 0;
    uint16_t crc_expected      = checksum_init_val;
    uint16_t crc_calculated    = Checksum_CalcCRC16(p_data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT16(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC16_EmptyData(void)
{
    uint16_t checksum_init_val = 0xFFFF;
    uint8_t  data[]            = {};
    size_t   len               = sizeof(data);
    uint16_t crc_expected      = checksum_init_val;
    uint16_t crc_calculated    = Checksum_CalcCRC16(data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT16(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC16_SingleDatBuf(void)
{
    uint16_t checksum_init_val = 0xFFFF;
    uint8_t  data[]            = {0x12, 0x34, 0x56, 0x78};
    size_t   len               = sizeof(data);
    uint16_t crc_expected      = 0x1EA7;
    uint16_t crc_calculated    = Checksum_CalcCRC16(data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT16(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC16_MultipleDataBuffs(void)
{
    uint16_t checksum_init_val = 0xFFFF;
    uint8_t  data[]            = {0x12, 0x34, 0x56, 0x78};
    size_t   len               = sizeof(data);
    uint16_t crc_expected      = 0x320F;

    uint16_t crc_calculated = Checksum_CalcCRC16(data, len, checksum_init_val);
    for (int i = 0; i < 10; i++)
    {
        data[0]++;
        crc_calculated = Checksum_CalcCRC16(data, len, crc_calculated);
    }

    TEST_ASSERT_EQUAL_UINT16(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC32_NullData(void)
{
    uint32_t checksum_init_val = 0xFFFFFFFF;
    uint8_t *p_data            = NULL;
    size_t   len               = 0;
    uint32_t crc_expected      = 0x00000000;
    uint32_t crc_calculated    = Checksum_CalcCRC32(p_data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT32(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC32_EmptyData(void)
{
    uint32_t checksum_init_val = 0xFFFFFFFF;
    uint8_t  data[]            = {};
    size_t   len               = sizeof(data);
    uint32_t crc_expected      = 0x00000000;
    uint32_t crc_calculated    = Checksum_CalcCRC32(data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT32(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC32_SingleDatBuf(void)
{
    uint32_t checksum_init_val = 0xFFFFFFFF;
    uint8_t  data[]            = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    size_t   len               = sizeof(data);
    uint32_t crc_expected      = 0xA85A34A3;
    uint32_t crc_calculated    = Checksum_CalcCRC32(data, len, checksum_init_val);
    TEST_ASSERT_EQUAL_UINT32(crc_expected, crc_calculated);
}

void test_Checksum_CalcCRC32_MultipleDataBuffs(void)
{
    uint32_t checksum_init_val = 0xFFFFFFFF;
    uint8_t  data[]            = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    size_t   len               = sizeof(data);
    uint32_t crc_expected      = 0x542273BE;

    uint32_t crc_calculated = Checksum_CalcCRC32(data, len, checksum_init_val);
    for (int i = 0; i < 10; i++)
    {
        data[0]++;
        crc_calculated = Checksum_CalcCRC32(data, len, crc_calculated);
    }

    TEST_ASSERT_EQUAL_UINT32(crc_expected, crc_calculated);
}

void test_Checksum_CalcSHA256_EmptyData(void)
{
    uint8_t data[]              = {};
    size_t  len                 = sizeof(data);
    uint8_t sha256_expected[32] = {0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14, 0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
                                   0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C, 0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55};
    uint8_t sha256_calculated[32];

    Checksum_CalcSHA256(data, len, sha256_calculated);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(sha256_expected, sha256_calculated, sizeof(sha256_expected));
}

void test_Checksum_CalcSHA256_SingleDataBuf(void)
{
    uint8_t data[]              = {0x12, 0x34, 0x56, 0x78};
    size_t  len                 = sizeof(data);
    uint8_t sha256_expected[32] = {
        0xB2, 0xED, 0x99, 0x21, 0x86, 0xA5, 0xCB, 0x19, 0xF6, 0x66, 0x8A, 0xAD, 0xE8, 0x21, 0xF5, 0x02,
        0xC1, 0xD0, 0x09, 0x70, 0xDF, 0xD0, 0xE3, 0x51, 0x28, 0xD5, 0x1B, 0xAC, 0x46, 0x49, 0x91, 0x6C,
    };
    uint8_t sha256_calculated[32];

    Checksum_CalcSHA256(data, len, sha256_calculated);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(sha256_expected, sha256_calculated, sizeof(sha256_expected));
}
