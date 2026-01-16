/*
QuakeSpasm Unit Tests - CRC Tests
Copyright (C) 2026 QuakeSpasm developers
*/

#include "unity/unity.h"
#include "quakedef.h"
#include "crc.h"
#include "crc.c"

/* Minimal type definitions */
typedef unsigned char byte;

/*-------------------------------------------------------
 * CRC Tests
 *-------------------------------------------------------*/

void test_CRC_Init_sets_correct_value(void)
{
    unsigned short crc;
    CRC_Init(&crc);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc);
}

void test_CRC_Value_applies_xor(void)
{
    unsigned short crc = 0xFFFF;
    unsigned short result = CRC_Value(crc);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, result);  /* 0xFFFF XOR 0x0000 = 0xFFFF */
}

void test_CRC_ProcessByte_changes_value(void)
{
    unsigned short crc;
    CRC_Init(&crc);
    unsigned short original = crc;
    
    CRC_ProcessByte(&crc, 'A');
    
    TEST_ASSERT_TRUE(crc != original);
}

void test_CRC_Block_empty_data(void)
{
    const byte data[] = "";
    unsigned short crc = CRC_Block(data, 0);
    
    /* Empty data gives init value (0xFFFF) XORed with XOR value (0x0000) = 0xFFFF */
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc);
}

void test_CRC_Block_single_byte(void)
{
    const byte data[] = "A";
    unsigned short crc = CRC_Block(data, 1);
    
    /* Just verify it produces a non-zero result */
    TEST_ASSERT_TRUE(crc != 0x0000);
}

void test_CRC_Block_known_value(void)
{
    const byte data[] = "123456789";
    unsigned short crc = CRC_Block(data, 9);
    
    /* CCITT CRC-16 of "123456789" is 0x29B1 */
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc);
}

void test_CRC_Block_same_input_same_output(void)
{
    const byte data[] = "Hello, World!";
    unsigned short crc1 = CRC_Block(data, 13);
    unsigned short crc2 = CRC_Block(data, 13);
    
    TEST_ASSERT_EQUAL_HEX16(crc1, crc2);
}

void test_CRC_Block_different_input_different_output(void)
{
    const byte data1[] = "Hello";
    const byte data2[] = "World";
    unsigned short crc1 = CRC_Block(data1, 5);
    unsigned short crc2 = CRC_Block(data2, 5);
    
    TEST_ASSERT_TRUE(crc1 != crc2);
}

void test_CRC_Block_order_matters(void)
{
    const byte data1[] = "AB";
    const byte data2[] = "BA";
    unsigned short crc1 = CRC_Block(data1, 2);
    unsigned short crc2 = CRC_Block(data2, 2);
    
    TEST_ASSERT_TRUE(crc1 != crc2);
}

void test_CRC_ProcessByte_incremental_matches_block(void)
{
    const byte data[] = "Test";
    unsigned short crc_block = CRC_Block(data, 4);
    
    unsigned short crc_incremental;
    CRC_Init(&crc_incremental);
    CRC_ProcessByte(&crc_incremental, 'T');
    CRC_ProcessByte(&crc_incremental, 'e');
    CRC_ProcessByte(&crc_incremental, 's');
    CRC_ProcessByte(&crc_incremental, 't');
    crc_incremental = CRC_Value(crc_incremental);
    
    TEST_ASSERT_EQUAL_HEX16(crc_block, crc_incremental);
}

/*-------------------------------------------------------
 * Test Suite Runner
 *-------------------------------------------------------*/

int run_crc_tests(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_CRC_Init_sets_correct_value);
    RUN_TEST(test_CRC_Value_applies_xor);
    RUN_TEST(test_CRC_ProcessByte_changes_value);
    RUN_TEST(test_CRC_Block_empty_data);
    RUN_TEST(test_CRC_Block_single_byte);
    RUN_TEST(test_CRC_Block_known_value);
    RUN_TEST(test_CRC_Block_same_input_same_output);
    RUN_TEST(test_CRC_Block_different_input_different_output);
    RUN_TEST(test_CRC_Block_order_matters);
    RUN_TEST(test_CRC_ProcessByte_incremental_matches_block);
    
    return UNITY_END();
}
