// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for the EP0 control-transfer state machine in usb_isr()
// (src/platform/usb.c), the only code between the host and every reply:
// splitting a data stage into packets, truncating it to wLength, ending it
// with a zero-length packet (0b2bb72), OUT data stages, SET_ADDRESS timing,
// data toggles, SET_CONFIGURATION and bus reset.
//
// usb.c is compiled into this test with the USB controller simulated. The
// test plays the controller and the host: a token uses the buffer descriptor
// the controller's own ping-pong pointer selects, must find it armed (OWN)
// by the driver, writes back the byte count and PID like the controller and
// runs usb_isr() once. An IN token on an unarmed descriptor is a NAK. The
// real cdcacm.c (from the host library) registers the descriptors and the
// class callbacks, so the replies are the ones the firmware sends.
//
// The buffer descriptors hold 32-bit addresses that usb.c casts from
// pointers through int; the Makefile links this test without PIE, so usb.c's
// statics lie below 2 GiB and those casts are exact. U1IR's write-1-to-clear
// is not modeled: each simulated interrupt sets it to the pending flags.

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

#define PACKET_SIZE  64  // EP0's, from cdcacm.c's device descriptor

// ---- what usb.c uses from the firmware's main.h, util.h, timer.h and the XC32 headers

#define __ISR(vector, ipl)
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define cassert(predicate)  do { _Static_assert(predicate, #predicate); } while (0)
#define debug_only(x)  do { (x); } while (0)
#define TF_LITTLE(x)  (x)
#define KVA_TO_PA(v)  ((uint32)(v))
#define PA_TO_KVA0(pa)  ((void *)(uintptr)(pa))

// PIC32MK0512GPK064 register bits, as p32mk0512gpk064.h (DFP 1.10.264) defines them
#define _IFS1_USB1IF_MASK        0x00000004
#define _U1OTGCON_OTGEN_MASK     0x00000004
#define _U1OTGCON_DPPULUP_MASK   0x00000080
#define _U1IR_URSTIF_MASK        0x00000001
#define _U1IR_TRNIF_MASK         0x00000008
#define _U1IR_IDLEIF_MASK        0x00000010
#define _U1IE_URSTIE_MASK        0x00000001
#define _U1IE_TRNIE_MASK         0x00000008
#define _U1IE_IDLEIE_MASK        0x00000010
#define _U1STAT_PPBI_MASK        0x00000004
#define _U1STAT_DIR_MASK         0x00000008
#define _U1CON_USBEN_MASK        0x00000001
#define _U1CON_PPBRST_MASK       0x00000002
#define _U1CON_TOKBUSY_MASK      0x00000020
#define _U1EP0_EPHSHK_MASK       0x00000001
#define _U1EP0_EPTXEN_MASK       0x00000004
#define _U1EP0_EPRXEN_MASK       0x00000008
#define _U1EP0_EPCONDIS_MASK     0x00000010

static volatile uint32 U1IR, U1IE, U1CON, U1OTGCON, U1ADDR, U1STAT;
static volatile uint32 U1EP0, U1EP1, U1EP2, U1EP3;
static volatile uint32 U1BDTP1, U1BDTP2, U1BDTP3;
static volatile uint32 IFS1CLR;
static volatile struct { uint32 USBPWR; } U1PWRCbits;
static volatile struct { uint32 USB1IE; } IEC1bits;
static volatile struct { uint32 USB1IP, USB1IS; } IPC8bits;
static const uint32 DEVSN0 = 0x0badf00du, DEVSN1 = 0x89abcdefu, DEVSN2 = 0, DEVSN3 = 0x01234567u;

static volatile int32 ticks;

// ---- fakes for what usb.c and cdcacm.c call

volatile int32 msecs;

static const char *hostname;

const char *
params_get_name(void)
{
    return hostname;
}

void
led_unknown_progress(void)
{
}

int
gpl(void)
{
    return 0;
}

int
splx(int level)
{
    (void)level;
    return 0;
}

void
delay(int32 ms)
{
    (void)ms;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#include "usb.c"  // src/platform/usb.c
#pragma GCC diagnostic pop

// ---- the controller and the host

static bool odd_bd[ENDPOINTS][2];  // the controller's ping-pong pointers, [endpoint][tx]

static volatile struct bdt *
current_bd(int endpoint, int tx)
{
    return MYBDT(endpoint, tx, odd_bd[endpoint][tx]);
}

static bool
armed(int endpoint, int tx)
{
    return current_bd(endpoint, tx)->flags & BD_FLAGS_OWN;
}

// the token on the current descriptor completed: write it back and interrupt
static void
complete(int endpoint, int tx, int pid, int bc)
{
    volatile struct bdt *bd;

    bd = current_bd(endpoint, tx);
    bd->flags = BD_FLAGS_BC_ENC(bc)|(bd->flags & BD_FLAGS_DATA)|(pid << 2);
    U1STAT = (uint32)(endpoint << 4)|(tx ? _U1STAT_DIR_MASK : 0)|(odd_bd[endpoint][tx] ? _U1STAT_PPBI_MASK : 0);
    odd_bd[endpoint][tx] = ! odd_bd[endpoint][tx];
    U1IR = _U1IR_TRNIF_MASK;
    usb_isr();
}

// what the device sent for one IN token
struct packet {
    int length;
    bool data1;
    byte bytes[PACKET_SIZE];
};

static void
host_in(int endpoint, struct packet *packet)
{
    volatile struct bdt *bd;

    TEST_ASSERT_TRUE_MESSAGE(armed(endpoint, 1), "IN token NAKed");
    bd = current_bd(endpoint, 1);
    packet->length = BD_FLAGS_BC_DEC(bd->flags);
    packet->data1 = bd->flags & BD_FLAGS_DATA;
    TEST_ASSERT_LESS_OR_EQUAL_INT(PACKET_SIZE, packet->length);
    if (packet->length) {
        memcpy(packet->bytes, bd->buffer, (size_t)packet->length);
    }
    complete(endpoint, 1, TOKEN_IN, packet->length);
}

// sends an OUT packet; returns the data toggle the device armed it with
static bool
host_out(int endpoint, const byte *bytes, int length)
{
    volatile struct bdt *bd;
    bool data1;

    TEST_ASSERT_TRUE_MESSAGE(armed(endpoint, 0), "OUT token NAKed");
    bd = current_bd(endpoint, 0);
    TEST_ASSERT_LESS_OR_EQUAL_INT(BD_FLAGS_BC_DEC(bd->flags), length);
    data1 = bd->flags & BD_FLAGS_DATA;
    if (length) {
        memcpy(bd->buffer, bytes, (size_t)length);
    }
    complete(endpoint, 0, TOKEN_OUT, length);
    return data1;
}

// EP0 is armed for the next SETUP: 8 bytes into setup_buffer, DATA0
static void
assert_setup_armed(void)
{
    volatile struct bdt *bd;

    TEST_ASSERT_TRUE_MESSAGE(armed(0, 0), "no SETUP armed");
    TEST_ASSERT_FALSE_MESSAGE(armed(0, 1), "IN armed while waiting for a SETUP");
    bd = current_bd(0, 0);
    TEST_ASSERT_EQUAL_INT(SETUP_SIZE, BD_FLAGS_BC_DEC(bd->flags));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, bd->flags & BD_FLAGS_DATA, "SETUP not armed with DATA0");
    TEST_ASSERT_EQUAL_PTR(&setup_buffer, bd->buffer);
}

static void
host_setup(byte requesttype, byte request, uint16 value, uint16 index, uint16 length)
{
    struct setup setup = { requesttype, request, value, index, length };

    assert_setup_armed();
    memcpy(current_bd(0, 0)->buffer, &setup, sizeof(setup));
    U1CON |= _U1CON_TOKBUSY_MASK;  // the controller stops processing tokens after a SETUP
    complete(0, 0, TOKEN_SETUP, SETUP_SIZE);
    TEST_ASSERT_FALSE_MESSAGE(U1CON & _U1CON_TOKBUSY_MASK, "token processing not resumed after SETUP");
}

static void
host_reset(void)
{
    memset(odd_bd, 0, sizeof(odd_bd));  // the driver's PPBRST
    U1IR = _U1IR_URSTIF_MASK;
    usb_isr();
}

// the last control transfer's data stage and status stage
static struct packet packets[4];
static int npackets;
static byte reply[sizeof(packets)/sizeof(packets[0])*PACKET_SIZE];
static int nreply;
static bool status_data1;

// IN tokens until the device NAKs, the way a host reads a data stage it
// does not know the length of
static void
data_stage_in(void)
{
    npackets = nreply = 0;
    while (armed(0, 1)) {
        TEST_ASSERT_LESS_THAN_INT_MESSAGE(LENGTHOF(packets), npackets, "data stage does not end");
        host_in(0, &packets[npackets]);
        memcpy(reply+nreply, packets[npackets].bytes, (size_t)packets[npackets].length);
        nreply += packets[npackets].length;
        npackets++;
    }
}

static void
status_stage_out(void)
{
    TEST_ASSERT_TRUE_MESSAGE(armed(0, 0), "no OUT status stage armed");
    TEST_ASSERT_EQUAL_INT(0, BD_FLAGS_BC_DEC(current_bd(0, 0)->flags));
    status_data1 = host_out(0, NULL, 0);
    assert_setup_armed();
}

static void
status_stage_in(void)
{
    struct packet packet;

    host_in(0, &packet);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, packet.length, "IN status stage is not a zero-length packet");
    status_data1 = packet.data1;
    assert_setup_armed();
}

static void
control_read(byte requesttype, byte request, uint16 value, uint16 index, uint16 length)
{
    host_setup(requesttype, request, value, index, length);
    data_stage_in();
    status_stage_out();
}

static void
get_descriptor(byte type, byte index, uint16 length)
{
    control_read(0x80, REQUEST_GET_DESCRIPTOR, (uint16)(type << 8 | index), 0, length);
}

// ---- the class callback cdcacm registered, recorded

static usb_control_cbfn cdcacm_control;
static int control_calls;
static int control_length;
static byte control_data[sizeof(endpoints[0].data_buffer)];

static int
record_control(const struct setup *setup, byte *buffer, int length)
{
    control_calls++;
    control_length = length;
    if (! (setup->requesttype & 0x80) && length) {
        memcpy(control_data, buffer, (size_t)length);
    }
    return cdcacm_control(setup, buffer, length);
}

static void
on_reset(void)
{
}

static bool
on_receive(byte *buffer, int length)
{
    (void)buffer;
    (void)length;
    return true;
}

void
setUp(void)
{
    TEST_ASSERT_TRUE_MESSAGE((uintptr)&endpoints < 0x80000000u, "test_usb must be linked without PIE");

    hostname = "Flea-Scope";
    control_calls = 0;

    usb_initialize();
    cdcacm_register(on_reset, on_receive);
    cdcacm_control = control_transfer_cbfn;
    control_transfer_cbfn = record_control;
    host_reset();
}

void
tearDown(void)
{
}

// ---- tests

// a 31-character hostname makes the product string exactly 64 bytes: the
// host asked for more, so a zero-length packet must end the data stage or
// the host waits for its control timeout (0b2bb72)
static void
test_full_packet_reply_shorter_than_wlength_ends_with_zlp(void)
{
    hostname = "abcdefghijklmnopqrstuvwxyz01234";

    get_descriptor(STRING_DESCRIPTOR, 2, 255);

    TEST_ASSERT_EQUAL_INT(2, npackets);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, packets[0].length);
    TEST_ASSERT_EQUAL_INT(0, packets[1].length);
    TEST_ASSERT_TRUE(packets[0].data1);
    TEST_ASSERT_FALSE(packets[1].data1);
    TEST_ASSERT_EQUAL_INT(64, reply[0]);
    TEST_ASSERT_EQUAL_INT(STRING_DESCRIPTOR, reply[1]);
    for (int i = 0; i < 31; i++) {
        TEST_ASSERT_EQUAL_INT(hostname[i], reply[2+2*i]);
        TEST_ASSERT_EQUAL_INT(0, reply[3+2*i]);
    }
}

// the host asked for exactly 64 bytes: it moves to the status stage after
// the full packet, so a zero-length packet would be one it never reads
static void
test_full_packet_reply_of_exactly_wlength_has_no_zlp(void)
{
    hostname = "abcdefghijklmnopqrstuvwxyz01234";

    get_descriptor(STRING_DESCRIPTOR, 2, 64);

    TEST_ASSERT_EQUAL_INT(1, npackets);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, packets[0].length);
}

// the 67-byte configuration descriptor is a full packet and a short one; the
// short packet ends the data stage
static void
test_configuration_descriptor_is_split_into_64_and_3_bytes(void)
{
    get_descriptor(CONFIGURATION_DESCRIPTOR, 0, 255);

    TEST_ASSERT_EQUAL_INT(2, npackets);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, packets[0].length);
    TEST_ASSERT_EQUAL_INT(3, packets[1].length);
    TEST_ASSERT_EQUAL_INT(67, configuration_descriptor_length);
    TEST_ASSERT_EQUAL_INT(67, reply[2]);  // wTotalLength
    TEST_ASSERT_EQUAL_MEMORY(configuration_descriptor, reply, 67);
}

// hosts read the first 8 bytes of the device descriptor and the first 9 of
// the configuration descriptor before the rest; the reply stops at wLength
static void
test_wlength_shorter_than_the_descriptor_truncates_the_reply(void)
{
    get_descriptor(DEVICE_DESCRIPTOR, 0, 8);
    TEST_ASSERT_EQUAL_INT(1, npackets);
    TEST_ASSERT_EQUAL_INT(8, nreply);
    TEST_ASSERT_EQUAL_MEMORY(device_descriptor, reply, 8);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, reply[7]);  // bMaxPacketSize0

    get_descriptor(CONFIGURATION_DESCRIPTOR, 0, 9);
    TEST_ASSERT_EQUAL_INT(1, npackets);
    TEST_ASSERT_EQUAL_INT(9, nreply);
    TEST_ASSERT_EQUAL_MEMORY(configuration_descriptor, reply, 9);
}

// a data stage starts with DATA1 and alternates; the status stage is DATA1;
// the next SETUP is armed with DATA0, whatever the previous transfer's
// packet count left the toggle at
static void
test_data_toggles_restart_for_every_control_transfer(void)
{
    get_descriptor(CONFIGURATION_DESCRIPTOR, 0, 255);
    TEST_ASSERT_EQUAL_INT(2, npackets);
    TEST_ASSERT_TRUE(packets[0].data1);
    TEST_ASSERT_FALSE(packets[1].data1);
    TEST_ASSERT_TRUE(status_data1);

    get_descriptor(DEVICE_DESCRIPTOR, 0, 18);
    TEST_ASSERT_EQUAL_INT(1, npackets);
    TEST_ASSERT_TRUE(packets[0].data1);
    TEST_ASSERT_TRUE(status_data1);

    get_descriptor(CONFIGURATION_DESCRIPTOR, 0, 255);
    TEST_ASSERT_EQUAL_INT(2, npackets);
    TEST_ASSERT_TRUE(packets[0].data1);
    TEST_ASSERT_FALSE(packets[1].data1);
    TEST_ASSERT_TRUE(status_data1);
}

// the SET_ADDRESS status stage still goes to address 0: the new address
// takes effect only once it completes, and only once
static void
test_set_address_takes_effect_after_its_status_stage(void)
{
    host_setup(0x00, REQUEST_SET_ADDRESS, 23, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(0, U1ADDR);

    status_stage_in();
    TEST_ASSERT_TRUE(status_data1);
    TEST_ASSERT_EQUAL_UINT32(23, U1ADDR);

    get_descriptor(DEVICE_DESCRIPTOR, 0, 18);
    TEST_ASSERT_EQUAL_INT(18, nreply);
    TEST_ASSERT_EQUAL_UINT32(23, U1ADDR);
}

// SET_LINE_CODING's 7 bytes reach cdcacm only after the OUT data stage, then
// the device answers with an IN status stage; GET_LINE_CODING reads them back
static void
test_set_line_coding_reaches_cdcacm_after_its_data_stage(void)
{
    static const byte coding[7] = { 0x00, 0xc2, 0x01, 0x00, 1, 2, 7 };  // 115200 2 stop even 7 bit

    host_setup(0x21, 0x20 /*SET_LINE_CODING*/, 0, 0, sizeof(coding));
    TEST_ASSERT_EQUAL_INT(0, control_calls);
    TEST_ASSERT_FALSE(armed(0, 1));

    TEST_ASSERT_TRUE(host_out(0, coding, sizeof(coding)));  // DATA1
    TEST_ASSERT_EQUAL_INT(1, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(coding), control_length);
    TEST_ASSERT_EQUAL_MEMORY(coding, control_data, sizeof(coding));

    status_stage_in();
    TEST_ASSERT_TRUE(status_data1);

    control_read(0xa1, 0x21 /*GET_LINE_CODING*/, 0, 0, sizeof(coding));
    TEST_ASSERT_EQUAL_INT(sizeof(coding), nreply);
    TEST_ASSERT_EQUAL_MEMORY(coding, reply, sizeof(coding));
}

// an OUT data stage longer than a packet is received into one buffer and
// handed over once, after its last packet
static void
test_out_data_stage_over_two_packets_is_handed_over_whole(void)
{
    byte command[70];

    for (int i = 0; i < (int)sizeof(command); i++) {
        command[i] = (byte)(i+1);
    }

    host_setup(0x21, 0x00 /*SEND_ENCAPSULATED_COMMAND*/, 0, 0, sizeof(command));
    TEST_ASSERT_TRUE(host_out(0, command, PACKET_SIZE));  // DATA1
    TEST_ASSERT_EQUAL_INT(0, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(command)-PACKET_SIZE, BD_FLAGS_BC_DEC(current_bd(0, 0)->flags));

    TEST_ASSERT_FALSE(host_out(0, command+PACKET_SIZE, sizeof(command)-PACKET_SIZE));  // DATA0
    TEST_ASSERT_EQUAL_INT(1, control_calls);
    TEST_ASSERT_EQUAL_INT(sizeof(command), control_length);
    TEST_ASSERT_EQUAL_MEMORY(command, control_data, sizeof(command));

    status_stage_in();
    TEST_ASSERT_TRUE(status_data1);
}

static void
test_set_configuration_attaches_cdcacm(void)
{
    uint32 count;

    TEST_ASSERT_FALSE(cdcacm_attached);
    count = cdcacm_attached_count;

    host_setup(0x00, REQUEST_SET_CONFIGURATION, 1, 0, 0);
    status_stage_in();

    TEST_ASSERT_TRUE(cdcacm_attached);
    TEST_ASSERT_EQUAL_UINT32(count+1, cdcacm_attached_count);
}

// a bus reset in the middle of a data stage detaches cdcacm, forgets the
// half-sent reply and the address, and waits for a SETUP again
static void
test_bus_reset_detaches_and_rearms_setup(void)
{
    static const byte command[1] = { '\r' };
    struct packet packet;

    host_setup(0x00, REQUEST_SET_ADDRESS, 23, 0, 0);
    status_stage_in();
    host_setup(0x00, REQUEST_SET_CONFIGURATION, 1, 0, 0);
    status_stage_in();
    host_out(bulk_out_ep, command, sizeof(command));
    TEST_ASSERT_TRUE(cdcacm_attached);
    TEST_ASSERT_TRUE(cdcacm_active);

    host_setup(0x80, REQUEST_GET_DESCRIPTOR, CONFIGURATION_DESCRIPTOR << 8, 0, 255);
    host_in(0, &packet);
    TEST_ASSERT_TRUE(armed(0, 1));  // the last 3 bytes

    host_reset();

    TEST_ASSERT_FALSE(cdcacm_attached);
    TEST_ASSERT_FALSE(cdcacm_active);
    TEST_ASSERT_EQUAL_UINT32(0, U1ADDR);
    assert_setup_armed();

    get_descriptor(DEVICE_DESCRIPTOR, 0, 18);
    TEST_ASSERT_EQUAL_INT(18, nreply);
    TEST_ASSERT_EQUAL_MEMORY(device_descriptor, reply, 18);
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_full_packet_reply_shorter_than_wlength_ends_with_zlp);
    RUN_TEST(test_full_packet_reply_of_exactly_wlength_has_no_zlp);
    RUN_TEST(test_configuration_descriptor_is_split_into_64_and_3_bytes);
    RUN_TEST(test_wlength_shorter_than_the_descriptor_truncates_the_reply);
    RUN_TEST(test_data_toggles_restart_for_every_control_transfer);
    RUN_TEST(test_set_address_takes_effect_after_its_status_stage);
    RUN_TEST(test_set_line_coding_reaches_cdcacm_after_its_data_stage);
    RUN_TEST(test_out_data_stage_over_two_packets_is_handed_over_whole);
    RUN_TEST(test_set_configuration_attaches_cdcacm);
    RUN_TEST(test_bus_reset_detaches_and_rearms_setup);
    return UNITY_END();
}
