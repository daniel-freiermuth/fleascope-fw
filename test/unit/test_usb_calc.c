// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for src/platform/usb-calc.h (run: make unit-test).
//
// Covers the GET_DESCRIPTOR(STRING) table walk in usb_isr(): which indices
// name a descriptor and which are past the end of the table.

#include "unity.h"

#include "usb-calc.h"

#define STRING_DESCRIPTOR  3

// the layout of cdcacm_string_descriptor[] (src/platform/cdcacm.c): four
// descriptors of 4, 54, 24 and 52 bytes; only the length bytes matter
static unsigned char table[4+54+24+52];

void setUp(void)
{
    table[0] = 4;
    table[1] = STRING_DESCRIPTOR;
    table[4] = 54;
    table[5] = STRING_DESCRIPTOR;
    table[58] = 24;
    table[59] = STRING_DESCRIPTOR;
    table[82] = 52;
    table[83] = STRING_DESCRIPTOR;
}

void tearDown(void) {}

static void
test_every_descriptor_is_found(void)
{
    TEST_ASSERT_EQUAL_INT(0, usb_string_descriptor_offset(table, (int)sizeof(table), 0));
    TEST_ASSERT_EQUAL_INT(4, usb_string_descriptor_offset(table, (int)sizeof(table), 1));
    TEST_ASSERT_EQUAL_INT(58, usb_string_descriptor_offset(table, (int)sizeof(table), 2));
    TEST_ASSERT_EQUAL_INT(82, usb_string_descriptor_offset(table, (int)sizeof(table), 3));
}

// index == number of descriptors walks to exactly the end of the table;
// the byte there is not part of it
static void
test_index_one_past_the_last_is_not_found(void)
{
    TEST_ASSERT_EQUAL_INT(-1, usb_string_descriptor_offset(table, (int)sizeof(table), 4));
}

static void
test_indices_further_past_the_end_are_not_found(void)
{
    TEST_ASSERT_EQUAL_INT(-1, usb_string_descriptor_offset(table, (int)sizeof(table), 5));
    TEST_ASSERT_EQUAL_INT(-1, usb_string_descriptor_offset(table, (int)sizeof(table), 255));
}

static void
test_empty_table_has_no_descriptors(void)
{
    TEST_ASSERT_EQUAL_INT(-1, usb_string_descriptor_offset(table, 0, 0));
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_descriptor_is_found);
    RUN_TEST(test_index_one_past_the_last_is_not_found);
    RUN_TEST(test_indices_further_past_the_end_are_not_found);
    RUN_TEST(test_empty_table_has_no_descriptors);
    return UNITY_END();
}
