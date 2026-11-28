// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for flash_upgrade() in src/platform/flash.c: the `upgrade`
// command's Intel HEX parser, the only way to update a device without a
// programmer, and flash_upgrade_ram(), which installs the staged image.
// docs/commands.md (`upgrade`) is the contract for its messages and for when
// it installs.
//
// flash.c is compiled into this test with the PIC32 registers it uses
// simulated: the NVM controller erases a page to 0xFF and programs a word, or
// a quad word in boot flash, by clearing bits; programming a word that is not
// erased, a boot flash page still write-protected in NVMBWP, an address
// outside program and boot flash, or any operation with interrupts enabled or
// DMA running fails the test. Program flash is mapped at the firmware's own
// addresses, FLASH_START (KSEG0) and its KSEG1 alias, and boot flash at
// FLASH2_START, so the parser's address arithmetic (KVA_TO_PA, PA_TO_KVA1,
// 32-bit vaddr/taddr) runs unchanged. The pasted lines come from a fake
// terminal_poll(). run_upgrade() ends the run at delay(), first called after
// `paste done!`, before the image would be installed; run_install() lets it
// go on to the software reset that ends flash_upgrade_ram().

#define _GNU_SOURCE  // memfd_create(), MAP_FIXED_NOREPLACE

#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "unity.h"

#include "main.h"  // test/host/main.h
#include "commands.h"

#define PROGRAM_PA  0x1D000000u  // FLASH_START as a physical address
#define BOOT_PA     0x1FC00000u  // FLASH2_START as a physical address
#define PROGRAM_END  (PROGRAM_PA+FLASH_BYTES/2-FLASH2_BYTES)  // first address the image may not use
#define STAGING     (FLASH_BYTES/2)  // offset of the staging half in program flash
#define BOOT_STAGING  (FLASH_BYTES-FLASH2_BYTES)  // offset of staged boot flash data

byte *const host_flash = (byte *)(uintptr)0x9D000000u;  // config.h's FLASH_START
static byte *const host_boot = (byte *)(uintptr)0x9FC00000u;  // config.h's FLASH2_START

// ---- what flash.c uses from the firmware's main.h and the XC32 headers

#define __longramfunc__
#define nomips16
#define assert_ram(x)  assert(x)
#define KVA_TO_PA(v)  ((uint32)(uintptr)(v) & 0x1FFFFFFFu)
#define PA_TO_KVA1(pa)  ((void *)(uintptr)((pa) | 0xA0000000u))
#define printf  host_printf

byte *end_of_static = (byte *)(uintptr)0x9D001000u;

static char output[2048];

static int
__attribute__((format(__printf__, 1, 2)))
host_printf(const char *format, ...)
{
    size_t used;
    int n;
    va_list ap;

    used = strlen(output);
    va_start(ap, format);
    n = vsnprintf(output+used, sizeof(output)-used, format, ap);
    va_end(ap);
    TEST_ASSERT_TRUE_MESSAGE(n >= 0 && (size_t)n < sizeof(output)-used, "output buffer full");
    return n;
}

// ---- PIC32MK NVM controller, as far as flash.c drives it

#define _NVMCON_WR_MASK         0x00008000u
#define _NVMCON_WREN_MASK       0x00004000u
#define _NVMCON_WRERR_MASK      0x00002000u
#define _NVMCON_LVDERR_MASK     0x00001000u
#define _NVMCON_NVMOP_MASK      0x0000000Fu
#define _NVMCON_NVMOP_POSITION  0

#define _NVMBWP_LBWP0_MASK      0x00000100u  // LBWP0..4: boot flash pages 0..4

static uint32 nvmcon;
static uint32 nvmcon_set;
static uint32 nvmcon_clr;
static uint32 nvmbwp;  // after reset all 5 boot flash pages are write-protected
static uint32 nvmbwp_clr;
static int nvm_programmed;  // words programmed since setUp(), 4 per quad word
static int nvm_quads;  // quad words programmed since setUp()
static int nvm_erased;  // pages erased since setUp()
static int nvm_boot_erased;  // of which in boot flash

// NVMKEY and SYSKEY: a write of 0xAA996655 immediately followed by one of
// 0x556699AA unlocks; any other write locks again. A write is seen at the
// next access to the key or to the register it guards.
struct unlock {
    uint32 written;
    bool pending;
    int keys;  // 0, 1 after 0xAA996655, 2 when unlocked
};

static void
unlock_settle(struct unlock *key)
{
    if (key->pending) {
        if (key->written == 0xAA996655u) {
            key->keys = 1;
        } else if (key->written == 0x556699AAu && key->keys == 1) {
            key->keys = 2;
        } else {
            key->keys = 0;
        }
        key->pending = false;
    }
}

static uint32 *
unlock_register(struct unlock *key)
{
    unlock_settle(key);
    key->pending = true;
    return &key->written;
}

static void
unlock_reset(struct unlock *key)
{
    key->written = 0;
    key->pending = false;
    key->keys = 0;
}

// true if key was unlocked; the unlock is used up
static bool
unlock_consume(struct unlock *key)
{
    bool unlocked;

    unlock_settle(key);
    unlocked = key->keys == 2;
    key->keys = 0;
    return unlocked;
}

static struct unlock nvmkey;
#define NVMKEY  (*unlock_register(&nvmkey))

static volatile uint32 NVMADDR;
static volatile uint32 NVMDATA0, NVMDATA1, NVMDATA2, NVMDATA3;

// ---- interrupt level and DMA, which every NVM operation needs held off

#define _DMACON_SUSPEND_MASK  0x00001000u

static int spl_level;
static uint32 dmacon;
static uint32 dmacon_set;
static uint32 dmacon_clr;
static struct {
    uint32 SUSPEND;
} DMACONbits = { 1 };

// DMACONSET/DMACONCLR writes are applied at the next DMA register access
static void
dma_settle(void)
{
    dmacon = (dmacon & ~dmacon_clr) | dmacon_set;
    dmacon_set = dmacon_clr = 0;
}

static uint32 *
dma_register(uint32 *reg)
{
    dma_settle();
    return reg;
}

#define DMACONSET  (*dma_register(&dmacon_set))
#define DMACONCLR  (*dma_register(&dmacon_clr))

static void
assert_quiesced(const char *what)
{
    dma_settle();
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, spl_level, what);
    TEST_ASSERT_TRUE_MESSAGE(dmacon & _DMACON_SUSPEND_MASK, what);
}

// NVMCONSET/NVMCONCLR/NVMBWPCLR writes are applied at the next NVM register
// access
static void
nvm_settle(void)
{
    nvmcon = (nvmcon & ~nvmcon_clr) | nvmcon_set;
    nvmcon_set = nvmcon_clr = 0;
    nvmbwp &= ~nvmbwp_clr;
    nvmbwp_clr = 0;
    unlock_settle(&nvmkey);
}

static uint32 *
nvm_register(uint32 *reg)
{
    nvm_settle();
    return reg;
}

static uint32 *
nvm_bwpclr(void)
{
    nvm_settle();
    TEST_ASSERT_TRUE_MESSAGE(unlock_consume(&nvmkey), "NVMBWPCLR without unlock sequence");
    return &nvmbwp_clr;
}

// the simulated flash word at physical address pa, in program or boot flash
static uint32 *
nvm_word(uint32 pa)
{
    if (pa >= PROGRAM_PA && pa < PROGRAM_PA+FLASH_BYTES) {
        return (uint32 *)(void *)(host_flash+(pa-PROGRAM_PA));
    }
    TEST_ASSERT_TRUE_MESSAGE(pa >= BOOT_PA && pa < BOOT_PA+FLASH2_BYTES, "NVM operation outside program and boot flash");
    TEST_ASSERT_FALSE_MESSAGE(nvmbwp & (_NVMBWP_LBWP0_MASK<<((pa-BOOT_PA)/FLASH_PAGE_SIZE)), "boot flash page write-protected");
    return (uint32 *)(void *)(host_boot+(pa-BOOT_PA));
}

// reading NVMCON after WR is set runs the operation and clears WR
static uint32
nvm_read(void)
{
    uint32 pa;
    uint32 *word;
    uint32 i;

    nvm_settle();
    if (nvmcon & _NVMCON_WR_MASK) {
        TEST_ASSERT_TRUE_MESSAGE(nvmcon & _NVMCON_WREN_MASK, "WR without WREN");
        TEST_ASSERT_TRUE_MESSAGE(unlock_consume(&nvmkey), "WR without unlock sequence");
        assert_quiesced("NVM operation with interrupts or DMA running");
        pa = NVMADDR;
        word = nvm_word(pa);
        switch (nvmcon & _NVMCON_NVMOP_MASK) {
            case 0x4:  // page erase
                TEST_ASSERT_EQUAL_UINT32(0, pa%FLASH_PAGE_SIZE);
                memset(word, 0xFF, FLASH_PAGE_SIZE);
                nvm_erased++;
                nvm_boot_erased += pa >= BOOT_PA;
                break;
            case 0x1:  // word program, program flash only
                TEST_ASSERT_TRUE_MESSAGE(pa < BOOT_PA, "boot flash takes only quad word programs");
                TEST_ASSERT_EQUAL_UINT32(0, pa%4);
                TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xFFFFFFFF, *word, "programming a word that is not erased");
                *word &= NVMDATA0;
                nvm_programmed++;
                break;
            case 0x2:  // quad word program
                TEST_ASSERT_EQUAL_UINT32(0, pa%16);
                for (i = 0; i < 4; i++) {
                    TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xFFFFFFFF, word[i], "programming a word that is not erased");
                }
                word[0] &= NVMDATA0;
                word[1] &= NVMDATA1;
                word[2] &= NVMDATA2;
                word[3] &= NVMDATA3;
                nvm_programmed += 4;
                nvm_quads++;
                break;
            default:
                TEST_FAIL_MESSAGE("unexpected NVMOP");
        }
        nvmcon &= ~_NVMCON_WR_MASK;
    }
    return nvmcon;
}

#define NVMCONSET  (*nvm_register(&nvmcon_set))
#define NVMCONCLR  (*nvm_register(&nvmcon_clr))
#define NVMCON     nvm_read()
#define NVMBWPCLR  (*nvm_bwpclr())

// ---- software reset: reading RSWRST after SWRST is set ends the run

#define _RSWRST_SWRST_MASK  0x00000001u

static struct unlock syskey;
#define SYSKEY  (*unlock_register(&syskey))
static uint32 rswrstset;
static jmp_buf reset;

static uint32 *
rswrstset_register(void)
{
    TEST_ASSERT_TRUE_MESSAGE(unlock_consume(&syskey), "RSWRSTSET without SYSKEY unlock");
    return &rswrstset;
}

static void
__attribute__((noreturn))
rswrst_read(void)
{
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(_RSWRST_SWRST_MASK, rswrstset, "RSWRST read before SWRST set");
    assert_quiesced("reset with interrupts or DMA running");
    longjmp(reset, 1);
}

#define RSWRSTSET  (*rswrstset_register())
#define RSWRST     (rswrst_read(), 0u)

// ---- terminal, util and timer fakes

char *volatile main_command;
bool terminal_echo;

static char upgrade_command[] = "upgrade";
static char script[48][80];  // the pasted lines
static int script_len;
static int script_pos;
static int acks;

void
terminal_poll(void)
{
    TEST_ASSERT_LESS_THAN_INT_MESSAGE(script_len, script_pos, "upgrade read past the end of the paste");
    main_command = script[script_pos++];
}

void
terminal_command_ack(bool edit)
{
    TEST_ASSERT_FALSE(edit);
    acks++;
}

// as src/platform/util.c's, which cannot be built for the host
static int
gethex(const char *p)
{
    char c;

    c = *p;
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else {
        return -1;
    }
}

static int
get2hex(char **p)
{
    int v1, v0;

    v1 = gethex(*p);
    if (v1 == -1) {
        return -1;
    }
    v0 = gethex(*p+1);
    if (v0 == -1) {
        return -1;
    }

    (*p) += 2;
    return v1*16+v0;
}

int
splx(int level)
{
    int old;

    old = spl_level;
    spl_level = level;
    return old;
}

static jmp_buf installing;
static bool let_install;  // run_install(): delay() returns
static int delays;

// flash_upgrade() calls delay() first after `paste done!`, committed to
// install; run_upgrade() stops there
void
delay(int32 ms)
{
    TEST_ASSERT_EQUAL_INT(100, ms);
    delays++;
    if (! let_install) {
        longjmp(installing, 1);
    }
}

// flash.c's casts between 32-bit addresses and pointers are exact here: the
// simulated flash lies below 4 GiB, at the addresses the firmware uses
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#include "flash.c"  // src/platform/flash.c
#pragma GCC diagnostic pop

// ---- building the paste

static char *
next_line(void)
{
    TEST_ASSERT_LESS_THAN_INT(LENGTHOF(script), script_len);
    return script[script_len++];
}

static void
paste(const char *text)
{
    snprintf(next_line(), sizeof(script[0]), "%s", text);
}

// a record with a correct checksum
static void
paste_record(uint32 type, uint32 addr, const byte *bytes, uint32 n)
{
    char *line;
    size_t len;
    uint32 sum;
    uint32 i;

    line = next_line();
    sum = n + (addr>>8) + (addr&0xFF) + type;
    len = (size_t)snprintf(line, sizeof(script[0]), ":%02X%04X%02X", n, addr, type);
    for (i = 0; i < n; i++) {
        len += (size_t)snprintf(line+len, sizeof(script[0])-len, "%02X", (uint)bytes[i]);
        sum += bytes[i];
    }
    snprintf(line+len, sizeof(script[0])-len, "%02X", (0x100-(sum&0xFF))&0xFF);
}

// a :04 record: the upper 16 bits of the following records' addresses
static void
paste_extend(uint32 upper)
{
    byte bytes[2] = { (byte)(upper>>8), (byte)upper };

    paste_record(4, 0, bytes, sizeof(bytes));
}

// a data record of whole words at the lower 16 address bits addr
static void
paste_words(uint32 addr, const uint32 *words, uint32 nwords)
{
    byte bytes[16];
    uint32 i;

    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(bytes)/4, nwords);
    for (i = 0; i < nwords*4; i++) {
        bytes[i] = (byte)(words[i/4]>>(8*(i%4)));  // little endian
    }
    paste_record(0, addr, bytes, nwords*4);
}

static void
paste_word(uint32 addr, uint32 word)
{
    paste_words(addr, &word, 1);
}

static void
paste_eof(void)
{
    paste(":00000001FF");
}

// runs `upgrade` on the paste; true if it went on to install the image
static bool
run_upgrade(void)
{
    main_command = upgrade_command;
    terminal_echo = true;
    if (setjmp(installing)) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(script_len, script_pos, "lines left after the end-of-file record");
        TEST_ASSERT_FALSE(terminal_echo);
        return true;
    }
    flash_upgrade();
    TEST_ASSERT_TRUE_MESSAGE(terminal_echo, "echo not back on after a failed upgrade");
    TEST_ASSERT_NULL_MESSAGE(main_command, "failing line not acknowledged");
    TEST_ASSERT_EQUAL_INT(script_pos+1, acks);  // `upgrade` and every line read
    return false;
}

// runs `upgrade` on the paste through the install; true if it ended in the
// software reset
static bool
run_install(void)
{
    main_command = upgrade_command;
    terminal_echo = true;
    let_install = true;
    if (setjmp(reset)) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(script_len, script_pos, "lines left after the end-of-file record");
        return true;
    }
    flash_upgrade();
    return false;
}

static uint32
flash_word(uint32 offset)
{
    uint32 word;

    memcpy(&word, host_flash+offset, sizeof(word));
    return word;
}

#define HEADER   "paste HEX upgrade file now...\n"
#define INSTALL  "\npaste done!\nprogramming flash for upgrade...\nwait for heartbeat LED to blink!\n"
#define FAILED   "upgrade failed\n"

void
setUp(void)
{
    // whatever a previous image left in program flash and boot flash
    memset(host_flash, 0x00, FLASH_BYTES);
    memset(host_boot, 0x00, FLASH2_BYTES);
    memset(script, 0, sizeof(script));
    script_len = script_pos = 0;
    acks = 0;
    output[0] = '\0';
    end_of_static = (byte *)(uintptr)0x9D001000u;
    nvm_programmed = nvm_quads = nvm_erased = nvm_boot_erased = 0;
    nvmcon = nvmcon_set = nvmcon_clr = 0;
    nvmbwp = 0x1F1F;
    nvmbwp_clr = 0;
    unlock_reset(&nvmkey);
    spl_level = 0;
    dmacon = dmacon_set = dmacon_clr = 0;
    unlock_reset(&syskey);
    rswrstset = 0;
    let_install = false;
    delays = 0;
}

void
tearDown(void)
{
}

// program words go to the staging half at their offset, boot flash words to
// its last FLASH2_BYTES, up to the last word each may use; a `.` per 4 records
static void
test_stages_program_and_boot_words_and_installs(void)
{
    static const uint32 first[] = { 0x03020100, 0x07060504, 0x0B0A0908, 0x0F0E0D0C };

    paste_extend(PROGRAM_PA>>16);
    paste_words(0x0000, first, LENGTHOF(first));
    paste_extend((PROGRAM_END-4)>>16);
    paste_word((PROGRAM_END-4)&0xFFFF, 0xCAFEF00D);
    paste_extend(BOOT_PA>>16);
    paste_word(0x0000, 0x0B000000);
    paste_word(FLASH2_BYTES-4, 0x12345678);
    paste_eof();

    TEST_ASSERT_TRUE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER ".." INSTALL, output);

    TEST_ASSERT_EQUAL_HEX32(0x03020100, flash_word(STAGING+0));
    TEST_ASSERT_EQUAL_HEX32(0x0F0E0D0C, flash_word(STAGING+12));
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00D, flash_word(STAGING+(PROGRAM_END-4-PROGRAM_PA)));
    TEST_ASSERT_EQUAL_HEX32(0x0B000000, flash_word(BOOT_STAGING));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, flash_word(FLASH_BYTES-4));
    TEST_ASSERT_EQUAL_INT(7, nvm_programmed);
    // the rest of the staging half is erased, the running image untouched
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, flash_word(STAGING+16));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, flash_word(BOOT_STAGING+4));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, flash_word(0));
}

// the staged image replaces the running one: program flash up to
// PROGRAM_END word for word from the staging half and erased above it, boot
// flash in quad words from the last FLASH2_BYTES of staging once NVMBWP is
// cleared, then the staging half is erased for the next upgrade and the MCU
// reset; the simulation fails any word programmed twice without an erase
static void
test_install_replaces_program_and_boot_flash_with_staged_image(void)
{
    static const uint32 first[] = { 0x03020100, 0x07060504, 0x0B0A0908, 0x0F0E0D0C };
    static const uint32 boot[] = { 0x0B000000, 0x11111111, 0x22222222, 0x33333333 };
    static byte expected[FLASH_BYTES/2];

    paste_extend(PROGRAM_PA>>16);
    paste_words(0x0000, first, LENGTHOF(first));
    paste_extend((PROGRAM_END-4)>>16);
    paste_word((PROGRAM_END-4)&0xFFFF, 0xCAFEF00D);
    paste_extend(BOOT_PA>>16);
    paste_words(0x0000, boot, LENGTHOF(boot));
    paste_word(FLASH2_BYTES-4, 0x12345678);
    paste_eof();

    TEST_ASSERT_TRUE(run_install());
    TEST_ASSERT_EQUAL_STRING(HEADER ".." INSTALL, output);
    TEST_ASSERT_EQUAL_INT(2, delays);

    // program flash: the staged words, everything else erased
    memset(expected, 0xFF, sizeof(expected));
    memcpy(expected, first, sizeof(first));
    memcpy(expected+(PROGRAM_END-4-PROGRAM_PA), &(uint32){ 0xCAFEF00D }, 4);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, host_flash, FLASH_BYTES/2);

    // boot flash: the staged boot words, everything else erased
    memset(expected, 0xFF, FLASH2_BYTES);
    memcpy(expected, boot, sizeof(boot));
    memcpy(expected+FLASH2_BYTES-4, &(uint32){ 0x12345678 }, 4);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, host_boot, FLASH2_BYTES);
    TEST_ASSERT_EQUAL_HEX32(0, nvmbwp);

    // the staging half is erased
    memset(expected, 0xFF, sizeof(expected));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, host_flash+STAGING, FLASH_BYTES/2);

    // staging erase and 10 staged words, then the install: program flash
    // erased and every word below PROGRAM_END programmed, boot flash erased
    // and programmed a quad word at a time, the staging half erased
    TEST_ASSERT_EQUAL_INT(3*(FLASH_BYTES/2/FLASH_PAGE_SIZE) + FLASH2_BYTES/FLASH_PAGE_SIZE, nvm_erased);
    TEST_ASSERT_EQUAL_INT(FLASH2_BYTES/FLASH_PAGE_SIZE, nvm_boot_erased);
    TEST_ASSERT_EQUAL_INT(FLASH2_BYTES/16, nvm_quads);
    TEST_ASSERT_EQUAL_INT(10 + (FLASH_BYTES/2-FLASH2_BYTES)/4 + FLASH2_BYTES/4, nvm_programmed);
}

// flash_upgrade() refuses before touching flash when the running image
// reaches past PROGRAM_END, where it could not be staged; at PROGRAM_END it
// goes on
static void
test_code_exceeding_half_of_flash_is_refused(void)
{
    end_of_static = (byte *)(FLASH_START+FLASH_BYTES/2-FLASH2_BYTES+1);
    paste_extend(PROGRAM_PA>>16);
    paste_eof();
    main_command = upgrade_command;
    terminal_echo = true;

    flash_upgrade();
    TEST_ASSERT_EQUAL_STRING("code exceeds half of flash\n", output);
    TEST_ASSERT_EQUAL_INT(0, script_pos);
    TEST_ASSERT_EQUAL_INT(0, nvm_erased);
    TEST_ASSERT_EQUAL_INT(0, nvm_programmed);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, flash_word(STAGING));
    TEST_ASSERT_TRUE(terminal_echo);

    setUp();
    end_of_static = (byte *)(FLASH_START+FLASH_BYTES/2-FLASH2_BYTES);
    paste_extend(PROGRAM_PA>>16);
    paste_eof();

    TEST_ASSERT_TRUE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER INSTALL, output);
}

// regression for 08f25e6: before the first :04 record the upper address bits
// are 0, so the record is at physical 0x0000xxxx and rejected
static void
test_data_before_first_extend_is_bad_address(void)
{
    paste_word(0x0010, 0xDEADBEEF);
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0010, 0x11111111);
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad address 0x10\n." FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0x11111111, flash_word(STAGING+0x10));
    TEST_ASSERT_EQUAL_INT(1, nvm_programmed);
}

// the image must leave the staging half free: PROGRAM_END is the first
// address rejected, and only the first address error is reported
static void
test_program_flash_ends_below_staging_reserve(void)
{
    static const uint32 words[] = { 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC };

    paste_extend((PROGRAM_END-4)>>16);
    paste_words((PROGRAM_END-4)&0xFFFF, words, LENGTHOF(words));
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad address 0x1d03b000\n" FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0xAAAAAAAA, flash_word(STAGING+(PROGRAM_END-4-PROGRAM_PA)));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, flash_word(STAGING+(PROGRAM_END-PROGRAM_PA)));
    TEST_ASSERT_EQUAL_INT(1, nvm_programmed);
}

static void
test_boot_flash_ends_at_flash2_bytes(void)
{
    static const uint32 words[] = { 0x12345678, 0x9ABCDEF0 };

    paste_extend(BOOT_PA>>16);
    paste_words(FLASH2_BYTES-4, words, LENGTHOF(words));
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad address 0x1fc05000\n" FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0x12345678, flash_word(FLASH_BYTES-4));
    TEST_ASSERT_EQUAL_INT(1, nvm_programmed);
}

// a duplicate keeps the first word, is reported once, and the paste goes on
// to the end-of-file record before the upgrade fails
static void
test_duplicate_program_address_fails_after_eof(void)
{
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0100, 0x11111111);
    paste_word(0x0100, 0x22222222);
    paste_word(0x0104, 0x33333333);
    paste_word(0x0104, 0x44444444);
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nduplicate address 0x1d000100\n." FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0x11111111, flash_word(STAGING+0x100));
    TEST_ASSERT_EQUAL_HEX32(0x33333333, flash_word(STAGING+0x104));
    TEST_ASSERT_EQUAL_INT(2, nvm_programmed);
}

static void
test_duplicate_boot_address_fails_after_eof(void)
{
    paste_extend(BOOT_PA>>16);
    paste_word(0x0000, 0x33333333);
    paste_word(0x0000, 0x44444444);
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nduplicate address 0x1fc00000\n." FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0x33333333, flash_word(BOOT_STAGING));
    TEST_ASSERT_EQUAL_INT(1, nvm_programmed);
}

// after a bad address, later duplicates in program or boot flash are not
// reported; the upgrade fails all the same
static void
test_only_the_first_address_error_is_reported(void)
{
    paste_extend((PROGRAM_PA>>16)+4);  // 0x1d040000: the staging half
    paste_word(0x0000, 0x55555555);
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0000, 0x11111111);
    paste_word(0x0000, 0x22222222);
    paste_extend(BOOT_PA>>16);
    paste_word(0x0000, 0x33333333);
    paste_word(0x0000, 0x44444444);
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad address 0x1d040000\n.." FAILED, output);
    TEST_ASSERT_EQUAL_HEX32(0x11111111, flash_word(STAGING));
    TEST_ASSERT_EQUAL_HEX32(0x33333333, flash_word(BOOT_STAGING));
    TEST_ASSERT_EQUAL_INT(2, nvm_programmed);
}

// a malformed record ends the paste at once
static void
test_malformed_records_end_the_upgrade(void)
{
    static const struct {
        const char *line;
        const char *message;
    } cases[] = {
        { "04000000FFFFFFFFFC", "bad record" },     // no ':'
        { ":G4000000FFFFFFFFFC", "bad count" },
        { ":0400Z000FFFFFFFFFC", "bad address" },
        { ":0400000ZFFFFFFFFFC", "bad type" },
        { ":04000000FFFFFFZZFC", "bad data" },
        { ":0300000011223397", "bad count" },       // not whole words
        { ":020000001122CB", "bad count" },         // half a word
        { ":00000002FE", "bad record" },            // unknown type
        { ":0100000100FE", "bad record" },          // end of file with data
        { ":040000041D000000DB", "bad record" },    // :04 with 4 bytes
        { ":00000004FC", "bad record" },            // :04 without its 2 bytes
        { ":020000041DZZ00", "bad address" },        // :04 upper bits
        { ":0000000005", "bad checksum 0x5" },      // empty data record
        { ":00000000", "bad checksum" },
    };
    char expected[256];
    uint32 i;
    int with_extend;

    for (i = 0; i < LENGTHOF(cases); i++) {
        for (with_extend = 0; with_extend < 2; with_extend++) {
            setUp();
            if (with_extend) {
                paste_extend(PROGRAM_PA>>16);
            }
            paste(cases[i].line);
            paste_eof();  // must not be read

            TEST_ASSERT_FALSE_MESSAGE(run_upgrade(), cases[i].line);
            TEST_ASSERT_EQUAL_INT_MESSAGE(script_len-1, script_pos, cases[i].line);
            snprintf(expected, sizeof(expected), HEADER "\n%s\n%s:01 record not found\n" FAILED,
                     cases[i].message, with_extend ? "" : ":04 record not found\n");
            TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, output, cases[i].line);
            TEST_ASSERT_EQUAL_INT_MESSAGE(0, nvm_programmed, cases[i].line);
        }
    }
}

static void
test_missing_records_fail(void)
{
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0000, 0x11111111);
    paste("x");  // what ends the paste here does not matter

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad record\n:01 record not found\n" FAILED, output);

    setUp();
    paste_word(0x0000, 0x11111111);
    paste_eof();

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad address 0x0\n:04 record not found\n" FAILED, output);
}

// documented quirk: the end-of-file record counts before its checksum is
// checked, so a bad or missing one still installs, unless an address error
// came earlier
static void
test_eof_with_bad_checksum_still_installs(void)
{
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0000, 0x11111111);
    paste(":00000001FE");

    TEST_ASSERT_TRUE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad checksum 0xff\n" INSTALL, output);

    setUp();
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0000, 0x11111111);
    paste(":00000001");

    TEST_ASSERT_TRUE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nbad checksum\n" INSTALL, output);

    setUp();
    paste_extend(PROGRAM_PA>>16);
    paste_word(0x0000, 0x11111111);
    paste_word(0x0000, 0x11111111);
    paste(":00000001");

    TEST_ASSERT_FALSE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER "\nduplicate address 0x1d000000\n\nbad checksum\n" FAILED, output);
}

// blank lines and leading whitespace are skipped and not counted for the `.`
static void
test_blank_lines_and_leading_whitespace_are_skipped(void)
{
    uint32 i;

    paste("");
    paste_extend(PROGRAM_PA>>16);
    paste("   ");
    for (i = 0; i < 6; i++) {
        paste_word(4*i, i);
        paste("\t");
    }
    paste_eof();
    memmove(script[3]+2, script[3], strlen(script[3])+1);
    memcpy(script[3], " \t", 2);  // the first data record, indented

    TEST_ASSERT_TRUE(run_upgrade());
    TEST_ASSERT_EQUAL_STRING(HEADER ".." INSTALL, output);
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL_HEX32(i, flash_word(STAGING+4*i));
    }
}

int
main(void)
{
    int fd;

    // program flash at FLASH_START (KSEG0) and its KSEG1 alias, as on the PIC32
    fd = memfd_create("program-flash", 0);
    if (fd < 0 || ftruncate(fd, FLASH_BYTES) != 0 ||
        mmap(host_flash, FLASH_BYTES, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED_NOREPLACE, fd, 0) != host_flash ||
        mmap((void *)(uintptr)0xBD000000u, FLASH_BYTES, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED_NOREPLACE, fd, 0) != (void *)(uintptr)0xBD000000u) {
        perror("mapping simulated flash");
        return 1;
    }
    // boot flash at FLASH2_START (KSEG0), where flash_upgrade_ram() writes it
    if (mmap(host_boot, FLASH2_BYTES, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0) != host_boot) {
        perror("mapping simulated boot flash");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_stages_program_and_boot_words_and_installs);
    RUN_TEST(test_install_replaces_program_and_boot_flash_with_staged_image);
    RUN_TEST(test_code_exceeding_half_of_flash_is_refused);
    RUN_TEST(test_data_before_first_extend_is_bad_address);
    RUN_TEST(test_program_flash_ends_below_staging_reserve);
    RUN_TEST(test_boot_flash_ends_at_flash2_bytes);
    RUN_TEST(test_duplicate_program_address_fails_after_eof);
    RUN_TEST(test_duplicate_boot_address_fails_after_eof);
    RUN_TEST(test_only_the_first_address_error_is_reported);
    RUN_TEST(test_malformed_records_end_the_upgrade);
    RUN_TEST(test_missing_records_fail);
    RUN_TEST(test_eof_with_bad_checksum_still_installs);
    RUN_TEST(test_blank_lines_and_leading_whitespace_are_skipped);
    return UNITY_END();
}
