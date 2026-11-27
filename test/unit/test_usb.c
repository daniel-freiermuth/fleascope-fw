// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for the endpoint 0 control transfers of usb_isr() in
// src/platform/usb.c: the data stage of a class or vendor request has to stay
// in endpoint 0's data_buffer, whatever wLength the host sends; the state of
// the interrupt and bulk endpoints follows that buffer in endpoints[].
//
// usb.c is compiled into this test with the USB controller simulated: the
// test plays the host and the serial interface engine, filling or draining
// the buffer descriptor the driver armed and raising the token-done
// interrupt. Like the hardware, it never transfers more bytes than the
// descriptor's byte count. usb.c is built as in the release firmware, where
// assert() is compiled out, so a broken bound shows as what it does on the
// device. The link is not position independent (Makefile): usb.c keeps buffer
// addresses in 32-bit ints, as on the PIC32.

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

#define PACKET_SIZE  64  // bMaxPacketSize0 and the bulk packet size

// ---- what usb.c uses from the firmware's main.h and the XC32 headers

#undef assert
#define assert(x)  // as in the release firmware
#define debug_only(x)  do { (x); } while (0)
#define cassert(predicate)  do { _Static_assert(predicate, #predicate); } while (0)
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define TF_LITTLE(x)  (x)
#define __ISR(v, ipl)
#define KVA_TO_PA(v)  ((uint32)(v))
#define PA_TO_KVA0(pa)  ((void *)(uintptr)(pa))

volatile int32 ticks;
bool cdcacm_active;

void
led_unknown_progress(void)
{
}

const char *
params_get_name(void)
{
    return "fleascope";
}

// ---- PIC32MK USB controller, as far as usb.c drives it (masks are plain
// ints, as in the XC32 device headers)

#define _U1IR_URSTIF_MASK    0x01
#define _U1IR_TRNIF_MASK     0x08
#define _U1IR_IDLEIF_MASK    0x10
#define _U1IE_URSTIE_MASK    0x01
#define _U1IE_TRNIE_MASK     0x08
#define _U1IE_IDLEIE_MASK    0x10
#define _U1STAT_PPBI_MASK    0x04
#define _U1STAT_DIR_MASK     0x08
#define _U1CON_USBEN_MASK    0x01
#define _U1CON_PPBRST_MASK   0x02
#define _U1CON_TOKBUSY_MASK  0x20
#define _U1OTGCON_OTGEN_MASK    0x04
#define _U1OTGCON_DPPULUP_MASK  0x80
#define _U1EP0_EPHSHK_MASK    0x01
#define _U1EP0_EPTXEN_MASK    0x04
#define _U1EP0_EPRXEN_MASK    0x08
#define _U1EP0_EPCONDIS_MASK  0x10
#define _IFS1_USB1IF_MASK     0x08

static volatile uint32 U1IR, U1IE, U1STAT, U1CON, U1OTGCON, U1ADDR;
static volatile uint32 U1EP0, U1EP1, U1EP2, U1EP3;
static volatile uint32 U1BDTP1, U1BDTP2, U1BDTP3;
static volatile uint32 IFS1CLR;
static volatile uint32 DEVSN0, DEVSN1, DEVSN2, DEVSN3;
static volatile struct { uint32 USBPWR : 1; } U1PWRCbits;
static volatile struct { uint32 USB1IE : 1; } IEC1bits;
static volatile struct { uint32 USB1IP : 3; uint32 USB1IS : 2; } IPC8bits;

// usb.c's casts between 32-bit addresses and pointers are exact here: the
// test is linked below 2 GiB
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "usb.c"  // src/platform/usb.c
#pragma GCC diagnostic pop

// ---- the device: cdcacm's descriptors, as far as usb.c parses them

static const byte device_descriptor_bytes[18] = {
    18, DEVICE_DESCRIPTOR, 0x00, 0x02, 0x02, 0x00, 0x00, PACKET_SIZE,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
};

static const byte configuration_descriptor_bytes[9+9+7+7+7] = {
    9, CONFIGURATION_DESCRIPTOR, sizeof(configuration_descriptor_bytes), 0x00, 1, 1, 0, 0x80, 250,
    9, INTERFACE_DESCRIPTOR, 0, 0, 3, 0x02, 0x02, 0x01, 0,
    7, ENDPOINT_DESCRIPTOR, 0x81, INTERRUPT_ATTRIBUTES, 0x08, 0x00, 0x10,
    7, ENDPOINT_DESCRIPTOR, 0x82, BULK_ATTRIBUTES, PACKET_SIZE, 0x00, 0x00,
    7, ENDPOINT_DESCRIPTOR, 0x03, BULK_ATTRIBUTES, PACKET_SIZE, 0x00, 0x00,
};

static const byte string_descriptor_bytes[] = {
    4, STRING_DESCRIPTOR, 0x09, 0x04,
};

// what the control callback was given, and what it answers an IN request with
static int control_calls;
static int control_length;
static byte control_data[512];

static void
reset_cb(void)
{
}

// gets endpoint 0's data_buffer and the length of the data stage in it, like
// cdcacm_control_transfer()
static int
control_cb(const struct setup *setup, byte *buffer, int length)
{
    control_calls++;
    control_length = length;
    TEST_ASSERT_TRUE(length >= 0 && (size_t)length <= sizeof(control_data));
    if (setup->requesttype & 0x80) {
        // like cdcacm's GET_ENCAPSULATED_RESPONSE: fill all the host asked for
        memset(buffer, 0, (size_t)length);
        return length;
    }
    if (buffer) {
        memcpy(control_data, buffer, (size_t)length);
    }
    return 0;
}

static int
bulk_cb(bool in, byte *buffer, int length)
{
    (void)in;
    (void)buffer;
    (void)length;
    return 0;
}

// ---- the host and the serial interface engine

// endpoints 1..3 (interrupt IN, bulk IN, bulk OUT) as the bus reset left them
static struct endpoint other_endpoints[ENDPOINTS-1];

// complete one token on endpoint 0 in the buffer descriptor the driver armed:
// SETUP and OUT write the host's packet, which must fit the descriptor's byte
// count (what the hardware does with a longer one is not modelled), IN reads
// it all. Returns the number of bytes transferred.
static int
token(int pid, const byte *data, int length, byte *in_data)
{
    int tx;
    int odd;
    int bc;
    byte *buffer;
    volatile struct bdt *bdt;

    tx = pid == TOKEN_IN;
    odd = endpoints[0].bdtodd[tx];
    bdt = MYBDT(0, tx, odd);
    TEST_ASSERT_TRUE_MESSAGE(bdt->flags & BD_FLAGS_OWN, "token to a buffer descriptor the driver did not arm");
    bc = BD_FLAGS_BC_DEC(bdt->flags);
    buffer = (byte *)PA_TO_KVA0((uintptr)bdt->buffer);
    if (tx) {
        if (bc) {
            memcpy(in_data, buffer, (size_t)bc);
        }
    } else {
        TEST_ASSERT_TRUE_MESSAGE(length <= bc, "host packet longer than the armed buffer descriptor");
        bc = length;
        memcpy(buffer, data, (size_t)bc);
    }
    bdt->flags = BD_FLAGS_BC_ENC(bc)|(pid << 2);

    U1STAT = (uint32)((0 << 4)|(tx ? _U1STAT_DIR_MASK : 0)|(odd ? _U1STAT_PPBI_MASK : 0));
    U1IR = _U1IR_TRNIF_MASK;
    usb_isr();
    return bc;
}

static void
setup_token(byte requesttype, byte request, uint16 length)
{
    struct setup setup;

    setup.requesttype = requesttype;
    setup.request = request;
    setup.value = 0;
    setup.index = 0;
    setup.length = length;
    TEST_ASSERT_EQUAL_INT(SETUP_SIZE, token(TOKEN_SETUP, (const byte *)&setup, SETUP_SIZE, NULL));
}

static void
assert_other_endpoints_unchanged(void)
{
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(other_endpoints, endpoints+1, sizeof(other_endpoints), "endpoints 1..3 overwritten");
}

void
setUp(void)
{
    control_calls = 0;
    control_length = -1;
    memset(control_data, 0, sizeof(control_data));

    usb_register(reset_cb, control_cb, bulk_cb);
    usb_device_descriptor(device_descriptor_bytes, sizeof(device_descriptor_bytes));
    usb_configuration_descriptor(configuration_descriptor_bytes, sizeof(configuration_descriptor_bytes));
    usb_string_descriptor(string_descriptor_bytes, sizeof(string_descriptor_bytes));
    usb_initialize();

    // bus reset: the driver arms endpoint 0 for a SETUP token
    U1IR = _U1IR_URSTIF_MASK;
    usb_isr();
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, endpoints[0].packetsize);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, endpoints[3].packetsize);
    memcpy(other_endpoints, endpoints+1, sizeof(other_endpoints));
}

void
tearDown(void)
{
}

// a class OUT request with a whole buffer of data: all of it reaches the
// callback, then the status stage
static void
test_class_out_data_up_to_the_buffer_reaches_the_callback(void)
{
    byte data[sizeof(endpoints[0].data_buffer)];
    byte status[1];
    int i;

    for (i = 0; i < (int)sizeof(data); i++) {
        data[i] = (byte)(i+1);
    }
    setup_token(0x21, 0x00, sizeof(data));
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, token(TOKEN_OUT, data, PACKET_SIZE, NULL));
    TEST_ASSERT_EQUAL_INT(0, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(data)-PACKET_SIZE, token(TOKEN_OUT, data+PACKET_SIZE, sizeof(data)-PACKET_SIZE, NULL));

    TEST_ASSERT_EQUAL_INT(1, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(data), control_length);
    TEST_ASSERT_EQUAL_MEMORY(data, control_data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(0, token(TOKEN_IN, NULL, 0, status));
    assert_other_endpoints_unchanged();
}

// a class OUT request with wLength 255: no packet lands past data_buffer, in
// the interrupt and bulk endpoints' state; the device takes all four packets,
// drops the data it cannot hold, completes the status stage and takes the
// next request
static void
test_class_out_longer_than_the_buffer_is_dropped(void)
{
    byte data[255];
    byte line_coding[7] = { 0x00, 0xC2, 0x01, 0x00, 0, 0, 8 };
    byte status[1];
    int offset;

    memset(data, 0xEE, sizeof(data));
    setup_token(0x21, 0x00, sizeof(data));  // SEND_ENCAPSULATED_COMMAND
    for (offset = 0; offset < (int)sizeof(data); offset += PACKET_SIZE) {
        token(TOKEN_OUT, data+offset, MIN(PACKET_SIZE, (int)sizeof(data)-offset), NULL);
        assert_other_endpoints_unchanged();
    }
    TEST_ASSERT_EQUAL_INT(0, control_calls);
    TEST_ASSERT_EQUAL_INT(0, token(TOKEN_IN, NULL, 0, status));

    setup_token(0x21, 0x20, sizeof(line_coding));  // SET_LINE_CODING
    token(TOKEN_OUT, line_coding, sizeof(line_coding), NULL);
    TEST_ASSERT_EQUAL_INT(1, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(line_coding), control_length);
    TEST_ASSERT_EQUAL_MEMORY(line_coding, control_data, sizeof(line_coding));
    TEST_ASSERT_EQUAL_INT(0, token(TOKEN_IN, NULL, 0, status));
    assert_other_endpoints_unchanged();
}

// a class IN request with wLength 255: the callback may fill only
// data_buffer, and the host gets no more than it holds
static void
test_class_in_longer_than_the_buffer_stays_in_it(void)
{
    byte in_data[PACKET_SIZE];
    int total;
    int n;

    setup_token(0xA1, 0x01, 255);  // GET_ENCAPSULATED_RESPONSE
    assert_other_endpoints_unchanged();
    TEST_ASSERT_EQUAL_INT(1, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(endpoints[0].data_buffer), control_length);

    total = 0;
    do {
        n = token(TOKEN_IN, NULL, 0, in_data);
        total += n;
    } while (n == PACKET_SIZE);
    TEST_ASSERT_EQUAL_INT(sizeof(endpoints[0].data_buffer), total);
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_class_out_data_up_to_the_buffer_reaches_the_callback);
    RUN_TEST(test_class_out_longer_than_the_buffer_is_dropped);
    RUN_TEST(test_class_in_longer_than_the_buffer_stays_in_it);
    return UNITY_END();
}
