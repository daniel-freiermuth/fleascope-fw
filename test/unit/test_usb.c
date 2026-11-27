// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for the control endpoint in src/platform/usb.c: GET_DESCRIPTOR
// of the string descriptors whose template starts with '@' (product: the
// hostname, params_get_name()) or '#' (serial number: usb_serial), which
// usb_isr() rewrites when the host reads them. USB 2.0 9.4.3 and 9.6.7: the
// reply to a read of wLength bytes is the first wLength bytes of the
// descriptor, and bLength (byte 0) is the whole descriptor's length, so a host
// may read 2 bytes and then read bLength bytes (macOS always does).
//
// usb.c is compiled into this test with the PIC32 USB registers it uses
// simulated as plain variables. The test plays the host and the USB engine:
// it completes the buffer descriptors usb.c arms on endpoint 0 and calls
// usb_isr() after each, as the hardware's token-done interrupt would. The
// driver turns buffer pointers into 32-bit physical addresses and back, so the
// Makefile links this test as a position-dependent executable, whose static
// data lies below 2 GiB.

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

// ---- what usb.c uses from the firmware's main.h, util.h and timer.h

#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define cassert(predicate)  do { _Static_assert(predicate, #predicate); } while (0)
#define debug_only(x)  do { (x); } while (0)
#define TF_LITTLE(x)  (x)
#define __ISR(v, ipl)
#define KVA_TO_PA(v)  ((uint32)(v))
#define PA_TO_KVA0(pa)  ((void *)(uintptr)(pa))

static volatile int32 ticks;

// ---- PIC32MK USB module, as far as usb.c drives it

#define _U1IR_URSTIF_MASK     0x01
#define _U1IR_TRNIF_MASK      0x08
#define _U1IR_IDLEIF_MASK     0x10
#define _U1IE_URSTIE_MASK     0x01
#define _U1IE_TRNIE_MASK      0x08
#define _U1IE_IDLEIE_MASK     0x10
#define _U1CON_USBEN_MASK     0x01
#define _U1CON_PPBRST_MASK    0x02
#define _U1CON_TOKBUSY_MASK   0x20
#define _U1OTGCON_OTGEN_MASK  0x04
#define _U1OTGCON_DPPULUP_MASK  0x80
#define _U1STAT_PPBI_MASK     0x04
#define _U1STAT_DIR_MASK      0x08
#define _U1EP0_EPHSHK_MASK    0x01
#define _U1EP0_EPTXEN_MASK    0x04
#define _U1EP0_EPRXEN_MASK    0x08
#define _U1EP0_EPCONDIS_MASK  0x10
#define _IFS1_USB1IF_MASK     0x08

static volatile uint32 U1IR, U1IE, U1CON, U1OTGCON, U1ADDR, U1STAT;
static volatile uint32 U1EP0, U1EP1, U1EP2, U1EP3;
static volatile uint32 U1BDTP1, U1BDTP2, U1BDTP3;
static volatile uint32 IFS1CLR;
static struct { uint32 USBPWR; } U1PWRCbits;
static struct { uint32 USB1IE; } IEC1bits;
static struct { uint32 USB1IP, USB1IS; } IPC8bits;
static const uint32 DEVSN0 = 0x01234567u, DEVSN1 = 0x89abcdefu, DEVSN2 = 0x0u, DEVSN3 = 0x76543210u;

// ---- the rest of the firmware usb.c calls

bool cdcacm_active;

void
led_unknown_progress(void)
{
}

static char name[32];

const char *
params_get_name(void)
{
    return name;
}

static void
bus_reset(void)
{
}

// usb.c converts pointers to 32-bit addresses (see the top of this file)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpointer-to-int-cast\"")
_Pragma("GCC diagnostic ignored \"-Wint-to-pointer-cast\"")
#include "usb.c"  // src/platform/usb.c
_Pragma("GCC diagnostic pop")

#define PACKET_SIZE  64

static const byte device_descriptor_template[] = {
    18, DEVICE_DESCRIPTOR, 0x00, 0x02, 0x02, 0x00, 0x00, PACKET_SIZE,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x01,
};

static const byte configuration_descriptor_template[] = {
    9, CONFIGURATION_DESCRIPTOR, 25, 0, 1, 1, 0, 0x80, 50,
    9, INTERFACE_DESCRIPTOR, 0, 0, 1, 0xff, 0, 0, 0,
    7, ENDPOINT_DESCRIPTOR, 0x82, BULK_ATTRIBUTES, PACKET_SIZE, 0, 0,
};

// laid out like cdcacm.c's, with its '@' and '#' templates
static const byte string_descriptor_template[] = {
    4, STRING_DESCRIPTOR, 0x09, 0x04,

    8, STRING_DESCRIPTOR, 'a', 0, 'b', 0, 'c', 0,

    24, STRING_DESCRIPTOR,
    '@', 0, 'F', 0, 'l', 0, 'e', 0, 'a', 0, '-', 0, 'S', 0, 'c', 0, 'o', 0, 'p', 0, 'e', 0,

    52, STRING_DESCRIPTOR,
    '#', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0, '1', 0, '1', 0, '1', 0, '2', 0, '2', 0, '2', 0, '2', 0,
    '3', 0, '3', 0, '3', 0, '3', 0, '4', 0, '4', 0, '4', 0, '4', 0, '5', 0, '5', 0, '5', 0, '5', 0,
};

#define PRODUCT  2
#define SERIAL   3

// ---- the host and the USB engine

// the endpoint 0 buffer descriptor usb.c has handed to the engine for tx
static volatile struct bdt *
armed(int tx)
{
    volatile struct bdt *bdt;

    bdt = MYBDT(0, tx, endpoints[0].bdtodd[tx]);
    TEST_ASSERT_TRUE_MESSAGE(bdt->flags & BD_FLAGS_OWN, "no packet armed on endpoint 0");
    return bdt;
}

// the engine completes the armed packet in direction tx with token pid
static void
complete(int tx, int pid, int bc)
{
    int odd;
    volatile struct bdt *bdt;

    odd = endpoints[0].bdtodd[tx];
    bdt = armed(tx);
    bdt->flags = BD_FLAGS_BC_ENC(bc)|(pid << 2);
    U1STAT = (uint32)((tx ? _U1STAT_DIR_MASK : 0)|(odd ? _U1STAT_PPBI_MASK : 0));
    U1IR = _U1IR_TRNIF_MASK;
    usb_isr();
}

// GET_DESCRIPTOR(STRING, index) of wLength bytes; returns the bytes received
static int
get_string(int index, int wlength, byte *reply)
{
    int n;
    int bc;
    volatile struct bdt *bdt;

    memset(reply, 0xAA, 256);

    // SETUP
    bdt = armed(0);
    TEST_ASSERT_EQUAL_PTR(&setup_buffer, bdt->buffer);
    setup_buffer = (struct setup){ 0x80, REQUEST_GET_DESCRIPTOR, (uint16)(STRING_DESCRIPTOR << 8 | index), 0x0409, (uint16)wlength };
    complete(0, TOKEN_SETUP, SETUP_SIZE);

    // IN data stage, until a short packet or wLength bytes
    n = 0;
    for (;;) {
        bdt = armed(1);
        bc = BD_FLAGS_BC_DEC(bdt->flags);
        TEST_ASSERT_TRUE_MESSAGE(n+bc <= wlength, "device sent more than wLength");
        memcpy(reply+n, bdt->buffer, (size_t)bc);
        n += bc;
        complete(1, TOKEN_IN, bc);
        if (bc < PACKET_SIZE || n == wlength) {
            break;
        }
    }

    // OUT status stage
    bdt = armed(0);
    TEST_ASSERT_EQUAL_INT(0, BD_FLAGS_BC_DEC(bdt->flags));
    complete(0, TOKEN_OUT, 0);
    return n;
}

// the UTF-16LE string descriptor of s
static int
expected_string(const char *s, byte *descriptor)
{
    int n;

    n = 2;
    for (; *s; s++) {
        descriptor[n++] = (byte)*s;
        descriptor[n++] = 0;
    }
    descriptor[0] = (byte)n;
    descriptor[1] = STRING_DESCRIPTOR;
    return n;
}

void
setUp(void)
{
    usb_register(bus_reset, NULL, NULL);
    usb_device_descriptor(device_descriptor_template, sizeof(device_descriptor_template));
    usb_configuration_descriptor(configuration_descriptor_template, sizeof(configuration_descriptor_template));
    usb_string_descriptor(string_descriptor_template, sizeof(string_descriptor_template));
    usb_initialize();

    // bus reset
    U1IR = _U1IR_URSTIF_MASK;
    usb_isr();

    strcpy(name, "Flea-Scope");
}

void
tearDown(void)
{
}

// ---- tests

// a 2-byte read reports the length of the patched descriptor; reading that
// many bytes then returns the whole hostname
static void
test_product_length_probe_then_read(void)
{
    byte reply[256];
    byte expected[256];
    int length;

    strcpy(name, "a-twenty-char-name-x");
    length = expected_string(name, expected);
    TEST_ASSERT_EQUAL_INT(42, length);

    TEST_ASSERT_EQUAL_INT(2, get_string(PRODUCT, 2, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, 2);

    TEST_ASSERT_EQUAL_INT(length, get_string(PRODUCT, reply[0], reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, length);
}

// the longest hostname (31 characters) fills exactly one packet, which a
// zero-length packet ends
static void
test_product_longest_name(void)
{
    byte reply[256];
    byte expected[256];
    int length;

    strcpy(name, "abcdefghijklmnopqrstuvwxyz01234");
    length = expected_string(name, expected);

    TEST_ASSERT_EQUAL_INT(length, get_string(PRODUCT, 255, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, length);

    TEST_ASSERT_EQUAL_INT(2, get_string(PRODUCT, 2, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, 2);
}

// a 1-byte read is the first byte of the patched descriptor
static void
test_product_one_byte(void)
{
    byte reply[256];

    TEST_ASSERT_EQUAL_INT(1, get_string(PRODUCT, 1, reply));
    TEST_ASSERT_EQUAL_HEX8(2+2*strlen("Flea-Scope"), reply[0]);
}

// the serial number replaces the '#' template, also in a 2-byte read
static void
test_serial_length_probe_then_read(void)
{
    byte reply[256];
    byte expected[256];
    int length;

    length = expected_string(usb_serial, expected);
    TEST_ASSERT_EQUAL_INT(50, length);

    TEST_ASSERT_EQUAL_INT(2, get_string(SERIAL, 2, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, 2);

    TEST_ASSERT_EQUAL_INT(length, get_string(SERIAL, reply[0], reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, reply, length);
}

// a string without a template is returned as it is, even when the previous
// reply left '@' or '#' where its first character would be
static void
test_plain_string_is_not_patched(void)
{
    byte reply[256];

    strcpy(name, "#abc");
    TEST_ASSERT_EQUAL_INT(10, get_string(PRODUCT, 255, reply));
    TEST_ASSERT_EQUAL_HEX8('#', reply[2]);

    TEST_ASSERT_EQUAL_INT(2, get_string(1, 2, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(string_descriptor_template+4, reply, 2);
    TEST_ASSERT_EQUAL_INT(8, get_string(1, 255, reply));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(string_descriptor_template+4, reply, 8);
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_product_length_probe_then_read);
    RUN_TEST(test_product_longest_name);
    RUN_TEST(test_product_one_byte);
    RUN_TEST(test_serial_length_probe_then_read);
    RUN_TEST(test_plain_string_is_not_patched);
    return UNITY_END();
}
