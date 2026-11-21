// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for cpustick/fleascope-calc.h (run: make unit-test).
//
// Covers the input-domain edges of the Flea-Scope command math that the
// firmware build never exercises: Timer3 period limits for scope and
// digital_stream, digital_stream packet sizing and bit packing, and stream
// sample packing.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "unity.h"

typedef unsigned int uint;

#include "fleascope-hal.h"
#include "fleascope-calc.h"

#define BUS_MHZ  120  // Flea-Scope bus clock

static char msg[128];

void setUp(void) {}
void tearDown(void) {}

// ---- scope Timer3 -------------------------------------------------------

static void
test_scope_max_interval(void)
{
    TEST_ASSERT_EQUAL_INT(31457, scope_max_interval(BUS_MHZ));
}

// every accepted interval programs a count PR3 (= t - 1) can hold
static void
test_scope_every_accepted_interval_fits_pr3(void)
{
    int number1;
    int ps;
    int t;

    for (number1 = 1; number1 <= scope_max_interval(BUS_MHZ); number1++) {
        t = scope_timer_count(number1, BUS_MHZ, &ps);
        snprintf(msg, sizeof(msg), "number1 %d -> t %d", number1, t);
        TEST_ASSERT_TRUE_MESSAGE(t > 1 && t < 0x10000, msg);
    }
}

static void
test_scope_interval_limits(void)
{
    int ps;
    int t;

    t = scope_timer_count(31457, BUS_MHZ, &ps);
    TEST_ASSERT_EQUAL_INT(16, ps);
    TEST_ASSERT_EQUAL_INT(0xffff, t);
    TEST_ASSERT_EQUAL_INT(-1, scope_timer_count(31458, BUS_MHZ, &ps));
    TEST_ASSERT_EQUAL_INT(-1, scope_timer_count(0x7fffffff, BUS_MHZ, &ps));
}

static void
test_scope_prescaler_switch(void)
{
    int ps;
    int t;

    // 1 -> 18 Msps: 3.6 MHz timer with 5x interleave
    t = scope_timer_count(1, BUS_MHZ, &ps);
    TEST_ASSERT_EQUAL_INT(1, ps);
    TEST_ASSERT_EQUAL_INT(33, t);

    // the prescaler switches from 1:1 to 1:16 above 1000
    t = scope_timer_count(1000, BUS_MHZ, &ps);
    TEST_ASSERT_EQUAL_INT(1, ps);
    TEST_ASSERT_EQUAL_INT(33333, t);
    t = scope_timer_count(1001, BUS_MHZ, &ps);
    TEST_ASSERT_EQUAL_INT(16, ps);
    TEST_ASSERT_EQUAL_INT(2085, t);
}

// ---- scope trigger position ---------------------------------------------

#define RING  (INTERLEAVE*2*128*10)  // adcbuffer
#define LANE  (RING/INTERLEAVE)      // one bitsbuffer lane

static short adc[RING];
static short bits[INTERLEAVE*LANE];

// records absolute samples [from, to) of a signal that is high in [edge, edge+width)
static void
record_pulse(int from, int to, int edge, int width)
{
    int s;
    bool high;

    for (s = from; s < to; s++) {
        high = s >= edge && s < edge+width;
        adc[s%RING] = high ? 3000 : 500;
        // virt b0 (rb4) follows the pulse; rb15 (virt b8) is always set and masked off
        bits[(s%INTERLEAVE)*LANE + (s/INTERLEAVE)%LANE] = (short)(0x8000 | (high ? 0x0010 : 0));
    }
}

// the pending samples between scope_isr's copies and the lane 4 DMA pointer,
// including when the ring wraps between them
static void
test_scope_samples_taken(void)
{
    int copied;
    int taken;

    for (copied = 0; copied <= 3*RING; copied += 5*128) {
        for (taken = copied; taken <= copied + 2*5*128; taken += INTERLEAVE) {
            snprintf(msg, sizeof(msg), "copied %d taken %d", copied, taken);
            TEST_ASSERT_EQUAL_INT_MESSAGE(taken, scope_samples_taken(copied, (taken/INTERLEAVE)%LANE, RING), msg);
        }
    }
}

// the edge is found exactly wherever it lies in the search range, also when the
// range spans the ring wrap and the lane boundaries
static void
test_scope_find_edges(void)
{
    int base;
    int edge;
    int hi;
    int width = 300;
    bool rise;

    for (base = RING - 700; base < RING + 700; base += 97) {
        hi = base + 1000;
        record_pulse(hi - 2000, hi + 1, base, width);
        for (edge = base; edge <= base + width; edge += width) {
            rise = edge == base;
            snprintf(msg, sizeof(msg), "base %d rise %d", base, rise);
            TEST_ASSERT_EQUAL_INT_MESSAGE(edge, scope_find_analog_edge(adc, RING, hi, hi - 1500, 512 << 2, rise), msg);
            TEST_ASSERT_EQUAL_INT_MESSAGE(edge, scope_find_digital_edge(bits, LANE, hi, hi - 1500, 0x0010, 0x0010, rise), msg);
        }
    }
}

// the newest matching edge wins, s = hi and s = lo+1 are searched, s = lo is not
static void
test_scope_find_edge_bounds(void)
{
    record_pulse(0, 3000, 1000, 500);

    // a second rising edge at 2000 is newer than the one at 1000
    record_pulse(1999, 2001, 2000, 1000);
    TEST_ASSERT_EQUAL_INT(2000, scope_find_analog_edge(adc, RING, 2500, 0, 2048, true));
    TEST_ASSERT_EQUAL_INT(2000, scope_find_digital_edge(bits, LANE, 2500, 0, 0x0010, 0x0010, true));

    TEST_ASSERT_EQUAL_INT(2000, scope_find_analog_edge(adc, RING, 2000, 1999, 2048, true));
    TEST_ASSERT_EQUAL_INT(-1, scope_find_analog_edge(adc, RING, 2000, 2000, 2048, true));
    TEST_ASSERT_EQUAL_INT(-1, scope_find_analog_edge(adc, RING, 1999, 1000, 2048, true));
    TEST_ASSERT_EQUAL_INT(2000, scope_find_digital_edge(bits, LANE, 2000, 1999, 0x0010, 0x0010, true));
    TEST_ASSERT_EQUAL_INT(-1, scope_find_digital_edge(bits, LANE, 2000, 2000, 0x0010, 0x0010, true));

    // a level that is already true has no edge in range
    TEST_ASSERT_EQUAL_INT(-1, scope_find_analog_edge(adc, RING, 1400, 1100, 2048, true));
}

// ---- digital_stream Timer3 ----------------------------------------------

static void
test_ds_timer_limits(void)
{
    int max;
    int tckps;
    int pr3;

    max = ds_max_us_per_sample(BUS_MHZ);
    TEST_ASSERT_EQUAL_INT(139808, max);

    // 0 and negative mean free-running (no timer)
    TEST_ASSERT_EQUAL_INT(0, ds_timer_config(0, BUS_MHZ, &tckps, &pr3));
    TEST_ASSERT_EQUAL_INT(0, ds_timer_config(-1, BUS_MHZ, &tckps, &pr3));

    TEST_ASSERT_EQUAL_INT(-1, ds_timer_config(max + 1, BUS_MHZ, &tckps, &pr3));
    TEST_ASSERT_EQUAL_INT(-1, ds_timer_config(1000000, BUS_MHZ, &tckps, &pr3));
    TEST_ASSERT_EQUAL_INT(-1, ds_timer_config(0x7fffffff, BUS_MHZ, &tckps, &pr3));

    TEST_ASSERT_EQUAL_INT(1, ds_timer_config(max, BUS_MHZ, &tckps, &pr3));
    TEST_ASSERT_EQUAL_INT(7, tckps);
    TEST_ASSERT_EQUAL_INT(65534, pr3);
    TEST_ASSERT_EQUAL_INT(1, ds_timer_config(1, BUS_MHZ, &tckps, &pr3));
    TEST_ASSERT_EQUAL_INT(0, tckps);
    TEST_ASSERT_EQUAL_INT(119, pr3);
}

// every reachable period uses the smallest prescaler that fits PR3 and is
// within one prescaler tick of the request
static void
test_ds_timer_every_period_smallest_prescaler(void)
{
    static const int prescale[8] = { 1, 2, 4, 8, 16, 32, 64, 256 };
    int max;
    int us;
    int tckps;
    int pr3;
    int rv;
    int t;
    int ps;

    max = ds_max_us_per_sample(BUS_MHZ);
    for (us = 1; us <= max; us++) {
        rv = ds_timer_config(us, BUS_MHZ, &tckps, &pr3);
        snprintf(msg, sizeof(msg), "us %d -> rv %d tckps %d pr3 %d", us, rv, tckps, pr3);
        TEST_ASSERT_TRUE_MESSAGE(rv == 1 && tckps >= 0 && tckps <= 7 && pr3 >= 1 && pr3 <= 65535, msg);

        t = us * BUS_MHZ;
        ps = prescale[tckps];
        TEST_ASSERT_TRUE_MESSAGE(t - (pr3 + 1) * ps >= 0 && t - (pr3 + 1) * ps < ps, msg);
        TEST_ASSERT_FALSE_MESSAGE(tckps > 0 && t / prescale[tckps - 1] <= 65535, msg);
    }
}

// ---- digital_stream packets ---------------------------------------------

// nchans 1..9 with the default, max and max+1 samples per packet
static void
test_ds_packet_sizing(void)
{
    int nchans;
    int max;
    int bits;

    for (nchans = 1; nchans <= DS_CHANNELS; nchans++) {
        snprintf(msg, sizeof(msg), "nchans %d", nchans);
        max = ds_max_samples_per_packet(nchans);
        TEST_ASSERT_EQUAL_INT_MESSAGE(504 / nchans, max, msg);

        TEST_ASSERT_EQUAL_INT_MESSAGE(max * nchans, ds_packet_bits(nchans, 0), msg);
        bits = ds_packet_bits(nchans, max);
        TEST_ASSERT_EQUAL_INT_MESSAGE(max * nchans, bits, msg);
        TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(DS_PACKET_BYTES_MAX, (bits + 7) >> 3, msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ds_packet_bits(nchans, max + 1), msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(nchans, ds_packet_bits(nchans, 1), msg);
    }
    TEST_ASSERT_EQUAL_INT(-1, ds_packet_bits(1, 0x7fffffff));
}

static void
test_ds_channel_positions(void)
{
    int chan_pos[DS_CHANNELS];

    TEST_ASSERT_EQUAL_INT(9, ds_channel_positions(0x1ff, chan_pos));
    TEST_ASSERT_EQUAL_INT(0, chan_pos[0]);
    TEST_ASSERT_EQUAL_INT(8, chan_pos[8]);

    TEST_ASSERT_EQUAL_INT(3, ds_channel_positions(0x111, chan_pos));
    TEST_ASSERT_EQUAL_INT(0, chan_pos[0]);
    TEST_ASSERT_EQUAL_INT(4, chan_pos[1]);
    TEST_ASSERT_EQUAL_INT(8, chan_pos[2]);

    // bits above b8 are not channels
    TEST_ASSERT_EQUAL_INT(1, ds_channel_positions(0xe01, chan_pos));
    TEST_ASSERT_EQUAL_INT(0, chan_pos[0]);
}

// a full packet of every channel count fills exactly packet_bits bits and
// never touches the bytes past DS_PACKET_BYTES_MAX
static void
test_ds_pack_full_packet_no_overrun(void)
{
    // guard bytes catch writes past DS_PACKET_BYTES_MAX
    unsigned char buffer[DS_PACKET_BYTES_MAX + 4];
    int chan_pos[DS_CHANNELS];
    int nchans;
    int mask;
    int max;
    int bits;
    int bit_pos;
    int s;

    for (nchans = 1; nchans <= DS_CHANNELS; nchans++) {
        snprintf(msg, sizeof(msg), "nchans %d", nchans);
        mask = (1 << nchans) - 1;
        ds_channel_positions(mask, chan_pos);
        max = ds_max_samples_per_packet(nchans);
        bits = ds_packet_bits(nchans, max);

        memset(buffer, 0xa5, sizeof(buffer));
        bit_pos = 0;
        for (s = 0; s < max; s++) {
            bit_pos = ds_pack_sample(buffer, bit_pos, mask, chan_pos, nchans);
        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(bits, bit_pos, msg);
        TEST_ASSERT_EACH_EQUAL_HEX8_MESSAGE(0xff, buffer, bits / 8, msg);
        TEST_ASSERT_EACH_EQUAL_HEX8_MESSAGE(0xa5, buffer + DS_PACKET_BYTES_MAX,
                                            sizeof(buffer) - DS_PACKET_BYTES_MAX, msg);
    }
}

// LSB-first, lowest selected channel first: channels b0, b4, b8
static void
test_ds_pack_bit_order(void)
{
    unsigned char buffer[DS_PACKET_BYTES_MAX];
    int chan_pos[DS_CHANNELS];
    int nchans;
    int bit_pos;

    memset(buffer, 0, sizeof(buffer));
    nchans = ds_channel_positions(0x111, chan_pos);
    bit_pos = 0;
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x101, chan_pos, nchans);  // 1 0 1
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x010, chan_pos, nchans);  // 0 1 0
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x111, chan_pos, nchans);  // 1 1 1
    TEST_ASSERT_EQUAL_INT(9, bit_pos);
    TEST_ASSERT_EQUAL_HEX8(0xd5, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(1, buffer[1] & 1);
}

// a partial packet is zero-padded up to packet_bytes
static void
test_ds_pad_packet(void)
{
    unsigned char buffer[DS_PACKET_BYTES_MAX];

    memset(buffer, 0xff, sizeof(buffer));
    ds_pad_packet(buffer, 9, 4);
    TEST_ASSERT_EQUAL_HEX8(0xff, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0, buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0xff, buffer[4]);
}

// ---- stream -------------------------------------------------------------

// Packs values with stream_pack_sample and returns the bytes produced.
static int
stream_pack_all(const int *values, int nvalues, unsigned int bitwidth,
                unsigned char *bytes)
{
    unsigned int leftover = 0;
    unsigned int leftover_bits = 0;
    unsigned char out[3];
    int nbytes;
    int n;
    int i;

    nbytes = 0;
    for (i = 0; i < nvalues; i++) {
        n = stream_pack_sample(&leftover, &leftover_bits, values[i], bitwidth, out);
        snprintf(msg, sizeof(msg), "bitwidth %u: leftover 0x%x/%u", bitwidth, leftover, leftover_bits);
        TEST_ASSERT_TRUE_MESSAGE(leftover_bits < 8 && leftover >> leftover_bits == 0, msg);
        memcpy(bytes + nbytes, out, n);
        nbytes += n;
    }
    return nbytes;
}

static void
test_stream_samples_pending(void)
{
    // nothing pending when the DMA counter has not moved past sent
    TEST_ASSERT_FALSE(stream_samples_pending(0, 0));
    TEST_ASSERT_TRUE(stream_samples_pending(5, 0));
    TEST_ASSERT_FALSE(stream_samples_pending(5, 5));
    TEST_ASSERT_FALSE(stream_samples_pending(5, 10));

    // counters past INT_MAX (where the old signed counters went negative and
    // halted the stream) and across the unsigned wrap at UINT_MAX
    TEST_ASSERT_TRUE(stream_samples_pending(0x80000000u, 0x7ffffff0u));
    TEST_ASSERT_TRUE(stream_samples_pending(0x80000010u, 0x80000000u));
    TEST_ASSERT_FALSE(stream_samples_pending(0x80000000u, 0x80000000u));
    TEST_ASSERT_TRUE(stream_samples_pending(0x00000010u, 0xfffffff0u));
    TEST_ASSERT_FALSE(stream_samples_pending(0xfffffff0u, 0x00000010u));
}

static void
test_stream_bits_per_sample(void)
{
    TEST_ASSERT_EQUAL_UINT(12, stream_bits_per_sample(1));
    TEST_ASSERT_EQUAL_UINT(13, stream_bits_per_sample(2));
    TEST_ASSERT_EQUAL_UINT(14, stream_bits_per_sample(3));
    TEST_ASSERT_EQUAL_UINT(14, stream_bits_per_sample(4));
    TEST_ASSERT_EQUAL_UINT(15, stream_bits_per_sample(5));
}

// averaging 1..5 against bitwidth 1 and 16
static void
test_stream_scale_sample(void)
{
    unsigned int averaging;
    unsigned int bits;
    int v;

    for (averaging = 1; averaging <= 5; averaging++) {
        snprintf(msg, sizeof(msg), "averaging %u", averaging);
        bits = stream_bits_per_sample(averaging);

        // the sum of `averaging` 12-bit samples fits bits_per_sample
        TEST_ASSERT_LESS_THAN_INT_MESSAGE(1 << bits, (int)(averaging * 4095), msg);

        // widening is exact, so full scale stays in 16 bits
        v = stream_scale_sample(averaging * 4095, bits, 16);
        TEST_ASSERT_EQUAL_INT_MESSAGE((int)(averaging * 4095) << (16 - bits), v, msg);
        TEST_ASSERT_LESS_THAN_INT_MESSAGE(0x10000, v, msg);

        // narrowing to 1 bit rounds to nearest
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, stream_scale_sample(0, bits, 1), msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, stream_scale_sample((1 << (bits - 1)) - 1, bits, 1), msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, stream_scale_sample((1 << (bits - 2)) - 1, bits, 1), msg);
    }
    TEST_ASSERT_EQUAL_INT(4095, stream_scale_sample(4095, 12, 12));
}

// narrowing saturates at the bitwidth maximum instead of rounding the top
// readings up to 1 << bitwidth, whose low bitwidth bits are 0 and whose
// carry would spill into the next packed sample
static void
test_stream_scale_sample_saturates(void)
{
    unsigned int averaging;
    unsigned int bitwidth;
    unsigned int bits;
    int max;
    unsigned char bytes[8];
    int values[3];

    for (averaging = 1; averaging <= 5; averaging++) {
        bits = stream_bits_per_sample(averaging);
        for (bitwidth = 1; bitwidth < bits; bitwidth++) {
            snprintf(msg, sizeof(msg), "averaging %u bitwidth %u", averaging, bitwidth);
            max = (1 << bitwidth) - 1;
            TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(max, stream_scale_sample(averaging * 4095, bits, bitwidth), msg);
            TEST_ASSERT_EQUAL_INT_MESSAGE(max, stream_scale_sample((1 << bits) - 1, bits, bitwidth), msg);
        }
    }
    TEST_ASSERT_EQUAL_INT(255, stream_scale_sample(4095, 12, 8));
    TEST_ASSERT_EQUAL_INT(255, stream_scale_sample(4088, 12, 8));
    TEST_ASSERT_EQUAL_INT(255, stream_scale_sample(4087, 12, 8));
    TEST_ASSERT_EQUAL_INT(1023, stream_scale_sample(16380, 14, 10));

    // on the wire: a full-scale reading followed by two zeros, averaging 1,
    // bitwidth 8 -> ff 00 00 (not 00 01 00)
    {
        static const unsigned char expected[] = { 0xff, 0x00, 0x00 };
        values[0] = stream_scale_sample(4095, 12, 8);
        values[1] = stream_scale_sample(0, 12, 8);
        values[2] = stream_scale_sample(0, 12, 8);
        TEST_ASSERT_EQUAL_INT(3, stream_pack_all(values, 3, 8, bytes));
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, bytes, 3);
    }
}

static void
test_stream_pack_layouts(void)
{
    unsigned char bytes[32];
    int values[8];
    int i;

    // bitwidth 1: eight samples per byte, LSB-first
    for (i = 0; i < 8; i++) {
        values[i] = (0xb2 >> i) & 1;
    }
    TEST_ASSERT_EQUAL_INT(1, stream_pack_all(values, 8, 1, bytes));
    TEST_ASSERT_EQUAL_HEX8(0xb2, bytes[0]);

    // bitwidth 16: two bytes per sample, little-endian
    {
        static const unsigned char expected[] = { 0x34, 0x12, 0xdc, 0xfe };
        values[0] = 0x1234;
        values[1] = 0xfedc;
        TEST_ASSERT_EQUAL_INT(4, stream_pack_all(values, 2, 16, bytes));
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, bytes, 4);
    }

    // bitwidth 12 straddles bytes: 0xabc, 0x123 -> bc 3a 12
    {
        static const unsigned char expected[] = { 0xbc, 0x3a, 0x12 };
        values[0] = 0xabc;
        values[1] = 0x123;
        TEST_ASSERT_EQUAL_INT(3, stream_pack_all(values, 2, 12, bytes));
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, bytes, 3);
    }
}

// every bitwidth: 8 full-scale samples produce bitwidth 0xff bytes
static void
test_stream_pack_every_bitwidth_full_scale(void)
{
    unsigned char bytes[32];
    int values[8];
    unsigned int bitwidth;
    int nbytes;
    int i;

    for (bitwidth = 1; bitwidth <= 16; bitwidth++) {
        snprintf(msg, sizeof(msg), "bitwidth %u", bitwidth);
        for (i = 0; i < 8; i++) {
            values[i] = 0xffff >> (16 - bitwidth);
        }
        nbytes = stream_pack_all(values, 8, bitwidth, bytes);
        TEST_ASSERT_EQUAL_INT_MESSAGE((int)bitwidth, nbytes, msg);
        TEST_ASSERT_EACH_EQUAL_HEX8_MESSAGE(0xff, bytes, nbytes, msg);
    }
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_scope_max_interval);
    RUN_TEST(test_scope_every_accepted_interval_fits_pr3);
    RUN_TEST(test_scope_interval_limits);
    RUN_TEST(test_scope_prescaler_switch);
    RUN_TEST(test_scope_samples_taken);
    RUN_TEST(test_scope_find_edges);
    RUN_TEST(test_scope_find_edge_bounds);
    RUN_TEST(test_ds_timer_limits);
    RUN_TEST(test_ds_timer_every_period_smallest_prescaler);
    RUN_TEST(test_ds_packet_sizing);
    RUN_TEST(test_ds_channel_positions);
    RUN_TEST(test_ds_pack_full_packet_no_overrun);
    RUN_TEST(test_ds_pack_bit_order);
    RUN_TEST(test_ds_pad_packet);
    RUN_TEST(test_stream_samples_pending);
    RUN_TEST(test_stream_bits_per_sample);
    RUN_TEST(test_stream_scale_sample);
    RUN_TEST(test_stream_scale_sample_saturates);
    RUN_TEST(test_stream_pack_layouts);
    RUN_TEST(test_stream_pack_every_bitwidth_full_scale);
    return UNITY_END();
}
