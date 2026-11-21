// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for sources/params.c: the A/B parameter pages that hold
// hostname and calibration across reboots, upgrades and firmware versions.
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

static _Alignas(PARAMS_PAGE_SIZE) uint32_t host_flash_words[FLASH_BYTES/sizeof(uint32_t)];
byte *const host_flash = (byte *)host_flash_words;

static uint32_t *const page_a = host_flash_words+(FLASH_BYTES-2*PARAMS_PAGE_SIZE)/sizeof(uint32_t);
static uint32_t *const page_b = host_flash_words+(FLASH_BYTES-PARAMS_PAGE_SIZE)/sizeof(uint32_t);

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
flash_write_words(uint32 *addr, const uint32 *data, uint32 nwords)
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

// first boot after a reflash: StickOS cleared flash variables to 0, and the
// original web GUI treats only 0 as "not calibrated"
static void
test_first_boot_clears_user_words_to_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(-1, params_get_user(0));

    params_initialize();

    TEST_ASSERT_EQUAL_INT32(0, params_get_user(0));
    TEST_ASSERT_EQUAL_INT32(0, params_get_user(PARAMS_USER_BYTES-4));
    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_ANALOG));  // system words untouched
    TEST_ASSERT_EQUAL_STRING("Flea-Scope", params_get_name());
}

// later boots must not clear calibration again
static void
test_later_boots_keep_values(void)
{
    params_initialize();
    params_set_user(4, 2000);
    erases = writes = 0;

    params_initialize();

    TEST_ASSERT_EQUAL_INT(0, erases + writes);
    TEST_ASSERT_EQUAL_INT32(2000, params_get_user(4));
}

// each update goes to the other page with a higher generation and keeps
// every other word, so a power loss mid-update leaves the old page current
static void
test_updates_alternate_pages_and_keep_other_words(void)
{
    params_initialize();
    params_set_name("scope-1");
    params_set_user(0, 1050);
    params_set(PARAM_ANALOG, 3300);
    params_set_user(12, 2100);

    TEST_ASSERT_EQUAL_INT32(1050, params_get_user(0));
    TEST_ASSERT_EQUAL_INT32(2100, params_get_user(12));
    TEST_ASSERT_EQUAL_INT32(0, params_get_user(8));
    TEST_ASSERT_EQUAL_INT32(3300, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_STRING("scope-1", params_get_name());

    // five writes: generations 0..4, the last one in page A
    TEST_ASSERT_EQUAL_HEX32(4, generation(page_a));
    TEST_ASSERT_EQUAL_HEX32(3, generation(page_b));
}

static void
test_unchanged_value_is_not_rewritten(void)
{
    params_initialize();
    params_set_user(0, 1050);
    erases = writes = 0;

    params_set_user(0, 1050);
    params_set(PARAM_ANALOG, -1);

    TEST_ASSERT_EQUAL_INT(0, erases + writes);
}

// StickOS's on-flash layout as byte offsets in a page (stickos/vars.h:
// FLASH_OFFSET(var) = 4096-4-(512-var)*4), written out literally so that a
// change to params.h cannot move the expected location along with it
#define STICKOS_USER_0     0x000  // first flash variable word
#define STICKOS_USER_LAST  0x7f8  // last of 511 flash variable words
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
    put_word(page_a, STICKOS_USER_0, 111);
    put_word(page_b, STICKOS_GENERATION, 7);
    put_word(page_b, STICKOS_USER_0, 1050);
    put_word(page_b, STICKOS_USER_LAST, 2100);
    put_word(page_b, STICKOS_ANALOG, 3300);
    memcpy((byte *)page_b+STICKOS_NAME, "unit-2", 7);

    params_initialize();  // a written page exists: nothing to do

    TEST_ASSERT_EQUAL_INT(0, erases + writes);
    TEST_ASSERT_EQUAL_INT32(1050, params_get_user(STICKOS_USER_0));
    TEST_ASSERT_EQUAL_INT32(2100, params_get_user(STICKOS_USER_LAST));
    TEST_ASSERT_EQUAL_INT32(3300, params_get(PARAM_ANALOG));
    TEST_ASSERT_EQUAL_STRING("unit-2", params_get_name());
}

// what this firmware writes, StickOS (and older Flea-Scope firmware) reads
// at the same offsets
static void
test_writes_stickos_layout(void)
{
    params_initialize();
    params_set_user(STICKOS_USER_LAST, 2100);
    params_set(PARAM_ANALOG, 3300);
    params_set_name("unit-2");

    // three updates after the first-boot write: generation 3, in page B
    TEST_ASSERT_EQUAL_HEX32(3, word_at(page_b, STICKOS_GENERATION));
    TEST_ASSERT_EQUAL_HEX32(2100, word_at(page_b, STICKOS_USER_LAST));
    TEST_ASSERT_EQUAL_HEX32(3300, word_at(page_b, STICKOS_ANALOG));
    TEST_ASSERT_EQUAL_STRING("unit-2", (const char *)page_b+STICKOS_NAME);
    TEST_ASSERT_EQUAL_INT(STICKOS_USER_LAST+4, PARAMS_USER_BYTES);
}

// after upgrade (successful, or failed and handled in flash_upgrade()):
// variables are 0 again, the staged boot flash data is gone, and a partial
// recalibration does not leave -1 in the other words (review of round 209)
static void
test_after_upgrade_variables_restart_at_zero(void)
{
    params_initialize();
    params_set_user(0, 1050);
    params_set_user(4, 2000);

    simulate_upgrade();
    TEST_ASSERT_EQUAL_INT32(-1, params_get_user(0));

    params_initialize();
    params_set_user(0, 1000);

    TEST_ASSERT_EQUAL_INT32(1000, params_get_user(0));
    TEST_ASSERT_EQUAL_INT32(0, params_get_user(4));
    TEST_ASSERT_EQUAL_INT32(0, params_get_user(8));
    TEST_ASSERT_EQUAL_INT32(-1, params_get(PARAM_LAST-15));  // staged config word dropped
    TEST_ASSERT_EQUAL_STRING("Flea-Scope", params_get_name());
}

static void
test_name_is_truncated_and_terminated(void)
{
    char name[64];

    memset(name, 'x', sizeof(name)-1);
    name[sizeof(name)-1] = '\0';
    params_initialize();

    params_set_name(name);

    TEST_ASSERT_EQUAL_size_t(31, strlen(params_get_name()));
    params_set_name("");
    TEST_ASSERT_EQUAL_STRING("Flea-Scope", params_get_name());
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_boot_clears_user_words_to_zero);
    RUN_TEST(test_later_boots_keep_values);
    RUN_TEST(test_updates_alternate_pages_and_keep_other_words);
    RUN_TEST(test_unchanged_value_is_not_rewritten);
    RUN_TEST(test_reads_stickos_pages);
    RUN_TEST(test_writes_stickos_layout);
    RUN_TEST(test_after_upgrade_variables_restart_at_zero);
    RUN_TEST(test_name_is_truncated_and_terminated);
    return UNITY_END();
}
