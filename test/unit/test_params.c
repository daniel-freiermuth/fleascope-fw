// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for sources/params.c: the A/B parameter pages that hold
// the hostname across reboots, upgrades and firmware versions.
//
// Flash is simulated like the PIC32's: erasing a page sets it to 0xFF, and
// programming can only clear bits, so programming a word twice is a bug.
// params.c keeps no state between calls except a scratch buffer, so a
// "reboot" needs no special handling.

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

#define WORDS  (PARAMS_PAGE_SIZE/sizeof(uint32_t))

_Alignas(PARAMS_PAGE_SIZE) byte host_flash[FLASH_BYTES];

static uint32_t *const page_a = (uint32_t *)(host_flash+FLASH_BYTES-2*PARAMS_PAGE_SIZE);
static uint32_t *const page_b = (uint32_t *)(host_flash+FLASH_BYTES-PARAMS_PAGE_SIZE);

static int erases;
static int writes;

void
flash_erase_pages(uint32 *addr, uint32 npages)
{
    TEST_ASSERT_EQUAL_UINT(0, ((byte *)addr-host_flash)%FLASH_PAGE_SIZE);
    memset(addr, 0xff, npages*FLASH_PAGE_SIZE);
    erases++;
}

void
flash_write_words(uint32 *addr, uint32 *data, uint32 nwords)
{
    uint32 i;

    for (i = 0; i < nwords; i++) {
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xffffffff, addr[i], "programming a word that is not erased");
        addr[i] &= data[i];
    }
    writes++;
}

static uint32_t
generation(const uint32_t *page)
{
    return page[WORDS-1];
}

// what upgrade's staging leaves in the parameter pages: both erased, then
// boot flash data (here: config words) staged into page A's system words
static void
simulate_upgrade(void)
{
    memset(page_a, 0xff, PARAMS_PAGE_SIZE);
    memset(page_b, 0xff, PARAMS_PAGE_SIZE);
    page_a[0xfc0/4] = 0x3f3fffff;
    page_a[0xfc4/4] = 0x7ffaa728;
}

void
setUp(void)
{
    memset(host_flash, 0xff, FLASH_BYTES);
    erases = writes = 0;
}

void
tearDown(void)
{
}

// each update goes to the other page with a higher generation and keeps
// every other word, so a power loss mid-update leaves the old page current
static void
test_updates_alternate_pages_and_keep_other_words(void)
{
    params_set_name("scope-1");
    params_set(PARAM_ANALOG, 3300);
    params_set(PARAM_SERVO, 50);

    TEST_ASSERT_EQUAL_INT32(3300, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_INT32(50, params_get(PARAM_SERVO));
    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_BAUD));
    TEST_ASSERT_EQUAL_STRING("scope-1", params_get_name());

    // three writes: generations 0..2, the last one in page A
    TEST_ASSERT_EQUAL_HEX32(2, generation(page_a));
    TEST_ASSERT_EQUAL_HEX32(1, generation(page_b));
}

static void
test_unchanged_value_is_not_rewritten(void)
{
    params_set(PARAM_ANALOG, 3300);
    erases = writes = 0;

    params_set(PARAM_ANALOG, 3300);
    params_set(PARAM_SERVO, -1);

    TEST_ASSERT_EQUAL_INT(0, erases + writes);
}

// StickOS's on-flash layout as byte offsets in a page (stickos/vars.h:
// FLASH_OFFSET(var) = 4096-4-(512-var)*4), written out literally so that a
// change to params.h cannot move the expected location along with it
#define STICKOS_ANALOG     0x804  // FLASH_ANALOG
#define STICKOS_NAME       0x82c  // FLASH_NAME_0..7, NUL-terminated
#define STICKOS_GENERATION 0xffc

static void
put_word(uint32_t *page, uint32_t offset, uint32_t value)
{
    memcpy((byte *)page+offset, &value, sizeof(value));
}

static uint32_t
word_at(const uint32_t *page, uint32_t offset)
{
    uint32_t value;

    memcpy(&value, (const byte *)page+offset, sizeof(value));
    return value;
}

// a page written by StickOS: the newer generation wins, and every value
// is found at StickOS's offset
static void
test_reads_stickos_pages(void)
{
    put_word(page_a, STICKOS_GENERATION, 6);
    put_word(page_a, STICKOS_ANALOG, 1111);
    put_word(page_b, STICKOS_GENERATION, 7);
    put_word(page_b, STICKOS_ANALOG, 3300);
    memcpy((byte *)page_b+STICKOS_NAME, "unit-2", 7);

    TEST_ASSERT_EQUAL_INT32(3300, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_STRING("unit-2", params_get_name());
}

// what this firmware writes, StickOS (and older Flea-Scope firmware) reads
// at the same offsets
static void
test_writes_stickos_layout(void)
{
    params_set(PARAM_ANALOG, 3300);
    params_set_name("unit-2");

    // two updates: generation 1, in page B
    TEST_ASSERT_EQUAL_HEX32(1, word_at(page_b, STICKOS_GENERATION));
    TEST_ASSERT_EQUAL_HEX32(3300, word_at(page_b, STICKOS_ANALOG));
    TEST_ASSERT_EQUAL_STRING("unit-2", (const char *)page_b+STICKOS_NAME);
}

// after upgrade (successful or failed): the boot flash data staged into
// page A is never read as parameters, and the first update drops it
static void
test_after_upgrade_staged_data_is_ignored(void)
{
    params_set(PARAM_ANALOG, 3300);
    params_set_name("unit-2");

    simulate_upgrade();

    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_LAST-15));  // staged config word
    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_STRING("Flea-Scope", params_get_name());

    params_set(PARAM_ANALOG, 3000);

    TEST_ASSERT_EQUAL_INT32(3000, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_LAST-15));
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, page_a[0xfc0/4]);
}

static void
test_name_is_truncated_and_terminated(void)
{
    char name[64];

    memset(name, 'x', sizeof(name)-1);
    name[sizeof(name)-1] = '\0';

    params_set_name(name);

    TEST_ASSERT_EQUAL_size_t(31, strlen(params_get_name()));
    params_set_name("");
    TEST_ASSERT_EQUAL_STRING("Flea-Scope", params_get_name());
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_updates_alternate_pages_and_keep_other_words);
    RUN_TEST(test_unchanged_value_is_not_rewritten);
    RUN_TEST(test_reads_stickos_pages);
    RUN_TEST(test_writes_stickos_layout);
    RUN_TEST(test_after_upgrade_staged_data_is_ignored);
    RUN_TEST(test_name_is_truncated_and_terminated);
    return UNITY_END();
}
