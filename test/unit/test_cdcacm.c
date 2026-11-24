// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for the bulk-IN ring in src/platform/cdcacm.c: every byte
// the device sends goes through cdcacm_print(), rx_arm() and the bulk-IN
// completion in cdcacm_bulk_transfer().
//
// The usb driver is faked. usb_device_enqueue() on the bulk-IN endpoint arms
// a packet like a buffer descriptor: the test's host side completes armed
// packets in arming order, reads their bytes and calls the completion
// callback cdcacm registered, as the usb isr would. Lowering the interrupt
// level with splx() is where pending interrupts run, so the fake splx() can
// complete packets there and advances msecs, which drives cdcacm_print()'s
// one-second timeout.

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

#define PACKET_SIZE  64
#define NRX          4  // cdcacm.c's ring size

byte bulk_in_ep = 2;
byte bulk_out_ep = 3;
byte int_ep = 1;
bool cdcacm_attached = true;
uint32 cdcacm_attached_count;
volatile int32 msecs;

static usb_reset_cbfn usb_reset;
static usb_bulk_cbfn usb_bulk;

// packets armed on bulk IN and not completed yet, oldest first
static struct armed {
    byte *buffer;
    int length;
    byte bytes[PACKET_SIZE];  // contents when armed
} armed[3];
static int narmed;

// what the host received: the bytes, and each packet's length
static byte received[32768];
static int nreceived;
static int packet_length[2048];
static int npackets;

// what the test printed and expects the host to receive, in order
static byte expected[32768];
static int nexpected;
static byte next_byte;

static int level;
static int completions_on_unmask;  // packets the isr completes at the next splx(0)s
static int ms_per_unmask;  // msecs that pass each time interrupts are enabled

static void
complete_one(void)
{
    struct armed packet;

    TEST_ASSERT_TRUE_MESSAGE(narmed > 0, "completing with nothing armed");
    packet = armed[0];
    narmed--;
    memmove(armed, armed+1, (size_t)narmed*sizeof(armed[0]));

    // the buffer must not change between arming and completion
    if (packet.length) {
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(packet.bytes, packet.buffer, packet.length, "armed buffer modified");
    }
    TEST_ASSERT_TRUE(nreceived+packet.length <= (int)sizeof(received));
    TEST_ASSERT_TRUE(npackets < (int)LENGTHOF(packet_length));
    memcpy(received+nreceived, packet.buffer, (size_t)packet.length);
    nreceived += packet.length;
    packet_length[npackets++] = packet.length;

    usb_bulk(true, packet.buffer, packet.length);
}

static void
drain(void)
{
    while (narmed) {
        complete_one();
    }
}

void
usb_device_enqueue(int endpoint, int tx, byte *buffer, int length)
{
    if (endpoint != bulk_in_ep || tx != 1) {
        return;  // bulk OUT and interrupt endpoints, and re-enables on reset
    }
    TEST_ASSERT_TRUE_MESSAGE(narmed < 2, "more than two bulk IN packets armed");
    TEST_ASSERT_TRUE(length >= 0 && length <= PACKET_SIZE);
    for (int i = 0; i < narmed; i++) {
        TEST_ASSERT_TRUE_MESSAGE(! length || ! armed[i].length || armed[i].buffer != buffer, "buffer armed twice");
    }
    armed[narmed].buffer = buffer;
    armed[narmed].length = length;
    memcpy(armed[narmed].bytes, buffer, (size_t)length);
    narmed++;
}

void
usb_register(usb_reset_cbfn reset, usb_control_cbfn control_transfer, usb_bulk_cbfn bulk_transfer)
{
    (void)control_transfer;
    usb_reset = reset;
    usb_bulk = bulk_transfer;
}

void
usb_device_descriptor(const byte *descriptor, int length)
{
    (void)descriptor;
    (void)length;
}

void
usb_configuration_descriptor(const byte *descriptor, int length)
{
    (void)descriptor;
    (void)length;
}

void
usb_string_descriptor(const byte *descriptor, int length)
{
    (void)descriptor;
    (void)length;
}

int
gpl(void)
{
    return level;
}

int
splx(int new_level)
{
    int old_level;

    old_level = level;
    level = new_level;
    if (! new_level) {
        msecs += ms_per_unmask;
        if (completions_on_unmask && narmed) {
            completions_on_unmask--;
            complete_one();
        }
    }
    return old_level;
}

void
delay(int32 ms)
{
    (void)ms;
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

// a usb bus reset: the driver forgets armed packets, then calls cdcacm
static void
bus_reset(void)
{
    narmed = 0;
    usb_reset();
}

// a command packet from the host
static void
out_packet(void)
{
    byte command[1] = { '\r' };

    usb_bulk(false, command, sizeof(command));
}

// prints length bytes of a running pattern; delivered says whether the host
// should get them
static void
print(int length, bool delivered)
{
    byte buffer[512];

    TEST_ASSERT_TRUE(length <= (int)sizeof(buffer));
    for (int i = 0; i < length; i++) {
        buffer[i] = next_byte++;
    }
    cdcacm_print(buffer, length);
    if (delivered) {
        TEST_ASSERT_TRUE(nexpected+length <= (int)sizeof(expected));
        memcpy(expected+nexpected, buffer, (size_t)length);
        nexpected += length;
    }
}

static void
assert_packets(const int *lengths, int count)
{
    TEST_ASSERT_EQUAL_INT(count, npackets);
    TEST_ASSERT_EQUAL_INT_ARRAY(lengths, packet_length, count);
}

static void
assert_received_expected(void)
{
    TEST_ASSERT_EQUAL_INT(nexpected, nreceived);
    if (nexpected) {
        TEST_ASSERT_EQUAL_MEMORY(expected, received, nexpected);
    }
}

void
setUp(void)
{
    narmed = nreceived = npackets = nexpected = 0;
    next_byte = 0;
    level = 0;
    completions_on_unmask = 0;
    ms_per_unmask = 1;
    msecs = 0;

    cdcacm_register(on_reset, on_receive);
    bus_reset();
    out_packet();  // a host is listening: clears discard left by an earlier test
}

void
tearDown(void)
{
}

static void
test_short_print_is_one_packet_without_zlp(void)
{
    static const int lengths[] = { 10 };

    print(10, true);
    TEST_ASSERT_EQUAL_INT(1, narmed);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// a host read ends at a short packet; without the ZLP a transfer ending on a
// full packet would leave the read pending until more output arrives
static void
test_full_packet_is_followed_by_one_zlp(void)
{
    static const int lengths[] = { PACKET_SIZE, 0 };

    print(PACKET_SIZE, true);
    TEST_ASSERT_EQUAL_INT(1, narmed);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, armed[0].length);

    complete_one();
    TEST_ASSERT_EQUAL_INT(1, narmed);
    TEST_ASSERT_EQUAL_INT(0, armed[0].length);
    complete_one();
    TEST_ASSERT_EQUAL_INT(0, narmed);

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// prints that together exactly fill the open buffer (rx_length+length == 64)
// make one full packet; the ZLP comes once, after the last packet, when the
// endpoint is idle, not while the earlier full packets are in flight
static void
test_prints_exactly_filling_the_open_buffer_end_with_one_zlp(void)
{
    static const int lengths[] = { PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 0 };

    print(PACKET_SIZE, true);
    print(PACKET_SIZE, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);
    print(30, true);
    print(34, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);

    complete_one();  // arms the third full packet; the endpoint is not idle
    TEST_ASSERT_EQUAL_INT(2, narmed);
    TEST_ASSERT_EQUAL_INT(PACKET_SIZE, armed[1].length);
    complete_one();
    TEST_ASSERT_EQUAL_INT(1, narmed);
    complete_one();  // now idle: the ZLP
    TEST_ASSERT_EQUAL_INT(1, narmed);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// a full packet followed by more output before it completes needs no ZLP:
// the short packet after it ends the transfer
static void
test_more_output_after_a_full_packet_cancels_the_zlp(void)
{
    static const int lengths[] = { PACKET_SIZE, 10 };

    print(PACKET_SIZE, true);
    print(10, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// a ZLP holds an armed slot but no buffer: completing it must not release
// one, so a print armed behind it lands in the next buffer and the whole ring
// is free once everything completes
static void
test_zlp_does_not_release_a_buffer(void)
{
    static const int lengths[] = { PACKET_SIZE, 0, 5, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 0 };
    byte *first;

    print(PACKET_SIZE, true);
    first = armed[0].buffer;
    complete_one();
    TEST_ASSERT_EQUAL_INT(1, narmed);  // the ZLP

    print(5, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);
    TEST_ASSERT_EQUAL_PTR(first+PACKET_SIZE, armed[1].buffer);
    drain();

    // all NRX buffers free again: the longest print fits without waiting
    print(NRX*PACKET_SIZE, true);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

static void
test_long_print_arms_two_packets_and_sends_the_rest_in_order(void)
{
    static const int lengths[] = { PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 8 };

    print(200, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);
    TEST_ASSERT_EQUAL_PTR(armed[0].buffer+PACKET_SIZE, armed[1].buffer);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// with every buffer closed or open and full, a print waits with interrupts
// enabled until a completion frees a buffer, then goes out after the rest
static void
test_print_into_a_full_ring_waits_for_a_completion(void)
{
    static const int lengths[] = { PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 10 };

    print(NRX*PACKET_SIZE, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);

    completions_on_unmask = 1;
    print(10, true);
    TEST_ASSERT_EQUAL_INT(0, completions_on_unmask);  // it had to wait
    TEST_ASSERT_EQUAL_INT(2, narmed);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// many prints of varying length with completions in between: the cursors
// wrap around the ring many times without losing, duplicating or reordering
// a byte, and every transfer ends with a short packet or one ZLP
static void
test_wraparound_keeps_every_byte_in_order(void)
{
    uint32 seed;
    int i;

    seed = 1;
    completions_on_unmask = 1000000;  // a print that must wait gets completions
    for (i = 0; i < 300; i++) {
        seed = seed*1103515245u+12345u;
        print(1+(int)((seed>>16)%150), true);
        seed = seed*1103515245u+12345u;
        for (int n = (int)((seed>>16)%3); n && narmed; n--) {
            complete_one();
        }
    }
    drain();

    assert_received_expected();
    TEST_ASSERT_TRUE(nreceived > 20*NRX*PACKET_SIZE);
    TEST_ASSERT_NOT_EQUAL_INT(PACKET_SIZE, packet_length[npackets-1]);
    for (i = 0; i < npackets; i++) {
        if (! packet_length[i]) {
            // a ZLP only ends a transfer whose last packet was full
            TEST_ASSERT_TRUE(i > 0);
            TEST_ASSERT_EQUAL_INT(PACKET_SIZE, packet_length[i-1]);
        }
    }
}

// no reader for a second: output is dropped until the host sends a packet
static void
test_no_reader_for_a_second_discards_output_until_an_out_packet(void)
{
    static const int lengths[] = { PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 0, 7 };

    print(NRX*PACKET_SIZE, true);
    print(10, false);  // nothing completes: times out
    TEST_ASSERT_TRUE(msecs > 1000);
    TEST_ASSERT_EQUAL_INT(2, narmed);

    // the host reads what was queued before the timeout; later output is
    // still dropped
    drain();
    print(5, false);
    TEST_ASSERT_EQUAL_INT(0, narmed);

    out_packet();
    print(7, true);
    TEST_ASSERT_EQUAL_INT(1, narmed);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// the longest print that can ever fit is NRX*64 = 256 bytes into an idle
// ring; anything longer waits for space that cannot appear and is dropped
// after the timeout (callers print at most 143 bytes today)
static void
test_print_longer_than_the_ring_is_dropped(void)
{
    static const int lengths[] = { PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE, 0 };

    print(NRX*PACKET_SIZE, true);
    drain();
    assert_packets(lengths, LENGTHOF(lengths));

    completions_on_unmask = 1000000;
    print(NRX*PACKET_SIZE+1, false);
    TEST_ASSERT_TRUE(msecs > 1000);
    TEST_ASSERT_EQUAL_INT(0, narmed);
    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

// a bus reset mid-transfer starts the ring over: the next print goes to the
// first buffer, nothing is counted as armed, and no stale data is sent
static void
test_reset_mid_transfer_starts_the_ring_over(void)
{
    static const int lengths[] = { 5, PACKET_SIZE, PACKET_SIZE, 0 };
    byte *first;

    print(200, false);
    first = armed[0].buffer;
    complete_one();
    TEST_ASSERT_EQUAL_INT(2, narmed);

    bus_reset();
    nreceived = npackets = 0;

    print(5, true);
    TEST_ASSERT_EQUAL_INT(1, narmed);
    TEST_ASSERT_EQUAL_PTR(first, armed[0].buffer);
    print(PACKET_SIZE, true);
    TEST_ASSERT_EQUAL_INT(2, narmed);
    print(PACKET_SIZE, true);
    drain();

    assert_packets(lengths, LENGTHOF(lengths));
    assert_received_expected();
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_short_print_is_one_packet_without_zlp);
    RUN_TEST(test_full_packet_is_followed_by_one_zlp);
    RUN_TEST(test_prints_exactly_filling_the_open_buffer_end_with_one_zlp);
    RUN_TEST(test_more_output_after_a_full_packet_cancels_the_zlp);
    RUN_TEST(test_zlp_does_not_release_a_buffer);
    RUN_TEST(test_long_print_arms_two_packets_and_sends_the_rest_in_order);
    RUN_TEST(test_print_into_a_full_ring_waits_for_a_completion);
    RUN_TEST(test_wraparound_keeps_every_byte_in_order);
    RUN_TEST(test_no_reader_for_a_second_discards_output_until_an_out_packet);
    RUN_TEST(test_print_longer_than_the_ring_is_dropped);
    RUN_TEST(test_reset_mid_transfer_starts_the_ring_over);
    return UNITY_END();
}
