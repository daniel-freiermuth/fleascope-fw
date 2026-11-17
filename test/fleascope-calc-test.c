// Host unit test for cpustick/fleascope-calc.h; run via testcalc.sh.
//
// Covers the input-domain edges of the Flea-Scope command math that the
// firmware-only CI build never exercises: Timer3 period limits for scope and
// digital_stream, digital_stream packet sizing and bit packing, and stream
// sample packing.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int uint;

#include "fleascope-hal.h"
#include "fleascope-calc.h"

#define BUS_MHZ  120  // Flea-Scope bus clock

static int failures;

#define CHECK(cond, ...) \
    do { \
        if (! (cond)) { \
            failures++; \
            printf("FAIL %s:%d: %s: ", __FILE__, __LINE__, #cond); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

static void
test_scope_timer(void)
{
    int number1;
    int ps;
    int t;

    CHECK(scope_max_interval(BUS_MHZ) == 31457, "max %d", scope_max_interval(BUS_MHZ));

    // every accepted interval programs a count PR3 (= t - 1) can hold
    for (number1 = 1; number1 <= scope_max_interval(BUS_MHZ); number1++) {
        t = scope_timer_count(number1, BUS_MHZ, &ps);
        if (! (t > 1 && t < 0x10000)) {
            CHECK(t > 1 && t < 0x10000, "number1 %d -> t %d", number1, t);
            break;
        }
    }

    t = scope_timer_count(31457, BUS_MHZ, &ps);
    CHECK(ps == 16 && t == 0xffff, "31457 -> t %d ps %d", t, ps);
    CHECK(scope_timer_count(31458, BUS_MHZ, &ps) == -1, "31458 accepted");
    CHECK(scope_timer_count(0x7fffffff, BUS_MHZ, &ps) == -1, "INT_MAX accepted");

    // 1 -> 18 Msps: 3.6 MHz timer with 5x interleave
    t = scope_timer_count(1, BUS_MHZ, &ps);
    CHECK(ps == 1 && t == 33, "1 -> t %d ps %d", t, ps);

    // the prescaler switches from 1:1 to 1:16 above 1000
    t = scope_timer_count(1000, BUS_MHZ, &ps);
    CHECK(ps == 1 && t == 33333, "1000 -> t %d ps %d", t, ps);
    t = scope_timer_count(1001, BUS_MHZ, &ps);
    CHECK(ps == 16 && t == 2085, "1001 -> t %d ps %d", t, ps);
}

static void
test_ds_timer(void)
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
    CHECK(max == 139808, "max %d", max);

    CHECK(ds_timer_config(0, BUS_MHZ, &tckps, &pr3) == 0, "0 uses timer");
    CHECK(ds_timer_config(-1, BUS_MHZ, &tckps, &pr3) == 0, "-1 uses timer");
    CHECK(ds_timer_config(max + 1, BUS_MHZ, &tckps, &pr3) == -1, "max+1 accepted");
    CHECK(ds_timer_config(1000000, BUS_MHZ, &tckps, &pr3) == -1, "1000000 accepted");
    CHECK(ds_timer_config(0x7fffffff, BUS_MHZ, &tckps, &pr3) == -1, "INT_MAX accepted");

    // every reachable period uses the smallest prescaler that fits PR3 and
    // is within one prescaler tick of the request
    for (us = 1; us <= max; us++) {
        rv = ds_timer_config(us, BUS_MHZ, &tckps, &pr3);
        t = us * BUS_MHZ;
        if (rv != 1 || tckps < 0 || tckps > 7 || pr3 < 1 || pr3 > 65535) {
            CHECK(false, "us %d -> rv %d tckps %d pr3 %d", us, rv, tckps, pr3);
            break;
        }
        ps = prescale[tckps];
        if (t - (pr3 + 1) * ps < 0 || t - (pr3 + 1) * ps >= ps ||
            (tckps > 0 && t / prescale[tckps - 1] <= 65535)) {
            CHECK(false, "us %d -> tckps %d pr3 %d", us, tckps, pr3);
            break;
        }
    }

    rv = ds_timer_config(max, BUS_MHZ, &tckps, &pr3);
    CHECK(rv == 1 && tckps == 7 && pr3 == 65534, "max -> tckps %d pr3 %d", tckps, pr3);
    rv = ds_timer_config(1, BUS_MHZ, &tckps, &pr3);
    CHECK(rv == 1 && tckps == 0 && pr3 == 119, "1 -> tckps %d pr3 %d", tckps, pr3);
}

static void
test_ds_packet(void)
{
    int chan_pos[DS_CHANNELS];
    int nchans;
    int max;
    int bits;

    // nchans 1..9 with the default, max and max+1 samples per packet
    for (nchans = 1; nchans <= DS_CHANNELS; nchans++) {
        max = ds_max_samples_per_packet(nchans);
        CHECK(max == 504 / nchans, "nchans %d max %d", nchans, max);

        bits = ds_packet_bits(nchans, 0);
        CHECK(bits == max * nchans, "nchans %d default bits %d", nchans, bits);
        bits = ds_packet_bits(nchans, max);
        CHECK(bits == max * nchans && (bits + 7) >> 3 <= DS_PACKET_BYTES_MAX,
              "nchans %d max bits %d", nchans, bits);
        CHECK(ds_packet_bits(nchans, max + 1) == -1, "nchans %d max+1 accepted", nchans);
        CHECK(ds_packet_bits(nchans, 1) == nchans, "nchans %d one sample", nchans);
    }
    CHECK(ds_packet_bits(1, 0x7fffffff) == -1, "INT_MAX samples accepted");

    nchans = ds_channel_positions(0x1ff, chan_pos);
    CHECK(nchans == 9 && chan_pos[0] == 0 && chan_pos[8] == 8, "0x1ff -> %d", nchans);
    nchans = ds_channel_positions(0x111, chan_pos);
    CHECK(nchans == 3 && chan_pos[0] == 0 && chan_pos[1] == 4 && chan_pos[2] == 8,
          "0x111 -> %d", nchans);
    // bits above b8 are not channels
    nchans = ds_channel_positions(0xe01, chan_pos);
    CHECK(nchans == 1 && chan_pos[0] == 0, "0xe01 -> %d", nchans);
}

static void
test_ds_pack(void)
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
    int i;

    // a full packet of every channel count fills exactly packet_bits bits
    // and never touches the bytes past DS_PACKET_BYTES_MAX
    for (nchans = 1; nchans <= DS_CHANNELS; nchans++) {
        mask = (1 << nchans) - 1;
        ds_channel_positions(mask, chan_pos);
        max = ds_max_samples_per_packet(nchans);
        bits = ds_packet_bits(nchans, max);

        memset(buffer, 0xa5, sizeof(buffer));
        bit_pos = 0;
        for (s = 0; s < max; s++) {
            bit_pos = ds_pack_sample(buffer, bit_pos, mask, chan_pos, nchans);
        }
        CHECK(bit_pos == bits, "nchans %d bit_pos %d", nchans, bit_pos);
        for (i = 0; i < bits / 8; i++) {
            CHECK(buffer[i] == 0xff, "nchans %d byte %d = 0x%02x", nchans, i, buffer[i]);
        }
        for (i = DS_PACKET_BYTES_MAX; i < (int)sizeof(buffer); i++) {
            CHECK(buffer[i] == 0xa5, "nchans %d overran byte %d", nchans, i);
        }
    }

    // LSB-first, lowest selected channel first: channels b0, b4, b8
    nchans = ds_channel_positions(0x111, chan_pos);
    bit_pos = 0;
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x101, chan_pos, nchans);  // 1 0 1
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x010, chan_pos, nchans);  // 0 1 0
    bit_pos = ds_pack_sample(buffer, bit_pos, 0x111, chan_pos, nchans);  // 1 1 1
    CHECK(bit_pos == 9, "bit_pos %d", bit_pos);
    CHECK(buffer[0] == 0xd5 && (buffer[1] & 1) == 1, "packed 0x%02x 0x%02x",
          buffer[0], buffer[1]);

    // a partial packet is zero-padded up to packet_bytes
    memset(buffer, 0xff, sizeof(buffer));
    ds_pad_packet(buffer, 9, 4);
    CHECK(buffer[1] == 0xff && buffer[2] == 0 && buffer[3] == 0 && buffer[4] == 0xff,
          "padded %02x %02x %02x %02x", buffer[1], buffer[2], buffer[3], buffer[4]);
}

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
        CHECK(leftover_bits < 8 && leftover >> leftover_bits == 0,
              "bitwidth %u: leftover 0x%x/%u", bitwidth, leftover, leftover_bits);
        memcpy(bytes + nbytes, out, n);
        nbytes += n;
    }
    return nbytes;
}

static void
test_stream_pending(void)
{
    // nothing pending when the DMA counter has not moved past sent
    CHECK(! stream_samples_pending(0, 0), "0/0");
    CHECK(stream_samples_pending(5, 0), "5/0");
    CHECK(! stream_samples_pending(5, 5), "5/5");
    CHECK(! stream_samples_pending(5, 10), "sent ahead of current");

    // counters past INT_MAX (where the old signed counters went negative and
    // halted the stream) and across the unsigned wrap at UINT_MAX
    CHECK(stream_samples_pending(0x80000000u, 0x7ffffff0u), "across INT_MAX");
    CHECK(stream_samples_pending(0x80000010u, 0x80000000u), "past INT_MAX");
    CHECK(! stream_samples_pending(0x80000000u, 0x80000000u), "equal past INT_MAX");
    CHECK(stream_samples_pending(0x00000010u, 0xfffffff0u), "across UINT_MAX");
    CHECK(! stream_samples_pending(0xfffffff0u, 0x00000010u), "sent wrapped ahead");
}

static void
test_stream(void)
{
    static const int ones16[8] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
    unsigned char bytes[32];
    int values[8];
    unsigned int averaging;
    unsigned int bitwidth;
    unsigned int bits;
    int nbytes;
    int v;
    int i;

    CHECK(stream_bits_per_sample(1) == 12, "avg 1");
    CHECK(stream_bits_per_sample(2) == 13, "avg 2");
    CHECK(stream_bits_per_sample(3) == 14, "avg 3");
    CHECK(stream_bits_per_sample(4) == 14, "avg 4");
    CHECK(stream_bits_per_sample(5) == 15, "avg 5");

    test_stream_pending();

    // averaging 1..5 against bitwidth 1 and 16
    for (averaging = 1; averaging <= 5; averaging++) {
        bits = stream_bits_per_sample(averaging);

        // the sum of `averaging` 12-bit samples fits bits_per_sample
        CHECK((int)(averaging * 4095) < (1 << bits), "avg %u overflows %u bits", averaging, bits);

        // widening is exact, so full scale stays in 16 bits
        v = stream_scale_sample(averaging * 4095, bits, 16);
        CHECK(v == (int)(averaging * 4095) << (16 - bits) && v < 0x10000,
              "avg %u -> 16 bits: %d", averaging, v);

        // narrowing to 1 bit rounds to nearest
        v = stream_scale_sample(0, bits, 1);
        CHECK(v == 0, "avg %u 0 -> 1 bit: %d", averaging, v);
        v = stream_scale_sample((1 << (bits - 1)) - 1, bits, 1);
        CHECK(v == 1, "avg %u half -> 1 bit: %d", averaging, v);
        v = stream_scale_sample((1 << (bits - 2)) - 1, bits, 1);
        CHECK(v == 0, "avg %u quarter -> 1 bit: %d", averaging, v);
    }
    CHECK(stream_scale_sample(4095, 12, 12) == 4095, "12 -> 12");

    // bitwidth 1: eight samples per byte, LSB-first
    for (i = 0; i < 8; i++) {
        values[i] = (0xb2 >> i) & 1;
    }
    nbytes = stream_pack_all(values, 8, 1, bytes);
    CHECK(nbytes == 1 && bytes[0] == 0xb2, "1 bit: %d bytes 0x%02x", nbytes, bytes[0]);

    // bitwidth 16: two bytes per sample, little-endian
    values[0] = 0x1234;
    values[1] = 0xfedc;
    nbytes = stream_pack_all(values, 2, 16, bytes);
    CHECK(nbytes == 4 && bytes[0] == 0x34 && bytes[1] == 0x12 &&
          bytes[2] == 0xdc && bytes[3] == 0xfe, "16 bit: %d bytes", nbytes);

    // bitwidth 12 straddles bytes: 0xabc, 0x123 -> bc 3a 12
    values[0] = 0xabc;
    values[1] = 0x123;
    nbytes = stream_pack_all(values, 2, 12, bytes);
    CHECK(nbytes == 3 && bytes[0] == 0xbc && bytes[1] == 0x3a && bytes[2] == 0x12,
          "12 bit: %d bytes %02x %02x %02x", nbytes, bytes[0], bytes[1], bytes[2]);

    // every bitwidth: n full-scale samples produce n*bitwidth/8 0xff bytes
    for (bitwidth = 1; bitwidth <= 16; bitwidth++) {
        for (i = 0; i < 8; i++) {
            values[i] = ones16[i] >> (16 - bitwidth);
        }
        nbytes = stream_pack_all(values, 8, bitwidth, bytes);
        CHECK(nbytes == (int)bitwidth, "bitwidth %u: %d bytes", bitwidth, nbytes);
        for (i = 0; i < nbytes; i++) {
            CHECK(bytes[i] == 0xff, "bitwidth %u byte %d 0x%02x", bitwidth, i, bytes[i]);
        }
    }
}

int
main(void)
{
    test_scope_timer();
    test_ds_timer();
    test_ds_packet();
    test_ds_pack();
    test_stream();

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("fleascope-calc: all tests passed\n");
    return 0;
}
