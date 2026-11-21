// Pure Flea-Scope computations: timer periods, packet sizing and bit packing
// for the scope, stream and digital_stream commands.
//
// No SFR access and no I/O, so test/unit/test_fleascope_calc.c can compile this
// header with a host compiler (make unit-test).  Keep it that way.
//
// Requires MSPS and INTERLEAVE (fleascope-hal.h).

#ifndef FLEASCOPE_CALC_H
#define FLEASCOPE_CALC_H

// ---------------------------------------------------------------- scope

// Largest scope sample interval whose 1:16 Timer3 count fits the 16-bit
// PR3 (31457 at 120 MHz).
static inline int
scope_max_interval(int bus_mhz)
{
    return ((0x10000*MSPS - MSPS/2)*16 - 1) / (bus_mhz*INTERLEAVE);
}

// Timer3 count (PR3 + 1) for scope sample interval number1 (>= 1), with the
// prescaler stored in *ps (1 or 16).  Returns -1 if number1 exceeds
// scope_max_interval(bus_mhz).
static inline int
scope_timer_count(int number1, int bus_mhz, int *ps)
{
    if (number1 > scope_max_interval(bus_mhz)) {
        return -1;
    }

    // set TMR3 to 3.6 MHz divided by horizontal scale
    if (number1 > 1000) {
        *ps = 16;
    } else {
        *ps = 1;
    }
    return ((unsigned int)bus_mhz*number1*INTERLEAVE / *ps + MSPS/2)/MSPS;  // 1:ps prescale
}

// Samples taken since the capture started.  `copied` is scope_isr's counter
// (whole adc chunks); `periods` is the lane 4 DMA pointer in samples, i.e. the
// Timer3 periods taken modulo the lane length (ring/INTERLEAVE).  Fewer than
// `ring` samples are ever taken but not yet copied.
static inline int
scope_samples_taken(int copied, int periods, int ring)
{
    return copied + (INTERLEAVE*periods - copied%ring + ring) % ring;
}

// Sample positions held across a `current` rebase (by `rebase`, a multiple of
// the ring) are off by `rebase`; this returns x in the same epoch as ref.
// Ring indices are unaffected by rebasing.
static inline int
scope_same_epoch(int x, int ref, int rebase)
{
    if (x - ref > rebase/2) {
        return x - rebase;
    }
    if (ref - x > rebase/2) {
        return x + rebase;
    }
    return x;
}

// Triggers are located in the recorded data, as the poll loop notices them
// late.  Both searches return a sample s in (lo, hi] whose condition is `to`
// while sample s-1's is not, or -1 if there is none.  Absolute sample s is at
// s % ring in the adc ring and at bits[(s % INTERLEAVE)*lane + (s / INTERLEAVE)
// % lane] in the INTERLEAVE digital lanes of length lane.  Ring indices are
// stepped rather than divided per sample: the search runs while sampling
// overwrites the ring.

// Condition adc[s] >= threshold; returns the newest crossing, the one the
// comparator (with its filter and hysteresis) has just reported.
static inline int
scope_find_analog_edge(const short *adc, int ring, int hi, int lo, int threshold, bool to)
{
    int s;
    int o;
    bool now;
    bool before;

    o = hi % ring;
    now = adc[o] >= threshold;
    for (s = hi; s > lo; s--) {
        o = o ? o-1 : ring-1;
        before = adc[o] >= threshold;
        if (now == to && before != to) {
            return s;
        }
        now = before;
    }
    return -1;
}

// Condition (bits[s] & mask) == value; returns the oldest transition, as the
// recorded PORTB samples are exactly what the trigger condition tests.
static inline int
scope_find_digital_edge(const short *bits, int lane, int hi, int lo, int value, int mask, bool to)
{
    int s;
    int l;
    int k;
    bool now;
    bool before;

    l = lo % INTERLEAVE;
    k = (lo / INTERLEAVE) % lane;
    before = (bits[l*lane + k] & mask) == value;
    for (s = lo+1; s <= hi; s++) {
        if (l < INTERLEAVE-1) {
            l++;
        } else {
            l = 0;
            k = k < lane-1 ? k+1 : 0;
        }
        now = (bits[l*lane + k] & mask) == value;
        if (now == to && before != to) {
            return s;
        }
        before = now;
    }
    return -1;
}

// --------------------------------------------------------------- stream

// True when the DMA sample counter `current` is ahead of `sent`.  Both are
// free-running unsigned counters that pass INT_MAX and wrap at UINT_MAX
// during long streams; the signed difference keeps the comparison valid.
static inline int
stream_samples_pending(unsigned int current, unsigned int sent)
{
    return (int)(current - sent) > 0;
}

// Bits needed to hold the sum of `averaging` 12-bit ADC samples.
static inline unsigned int
stream_bits_per_sample(unsigned int averaging)
{
    if (averaging == 1) {
        return 12;
    } else if (averaging == 2) {
        return 13;
    } else if (averaging <= 4) {
        return 14;
    } else {
        return 15;
    }
}

// Rescales the averaged sum v from bits_per_sample to bitwidth bits,
// rounding to nearest when dropping bits.  Rounding saturates at
// (1 << bitwidth) - 1: the top readings would otherwise round up to
// 1 << bitwidth, which does not fit in bitwidth bits.
static inline int
stream_scale_sample(int v, unsigned int bits_per_sample, unsigned int bitwidth)
{
    int bits_to_shift = bits_per_sample - bitwidth;
    int max = (1 << bitwidth) - 1;

    if (bits_to_shift > 0) {
        v = (v + (1 << (bits_to_shift -1))) >> bits_to_shift;
        return v > max ? max : v;
    } else {
        return v << (-bits_to_shift);
    }
}

// Appends the bitwidth-bit value v to the LSB-first bit stream held in
// *leftover / *leftover_bits and moves every completed byte to out.
// Returns the number of bytes written to out (at most 3).
static inline int
stream_pack_sample(unsigned int *leftover, unsigned int *leftover_bits,
                   int v, unsigned int bitwidth, unsigned char out[3])
{
    int n;

    *leftover = *leftover | (v << *leftover_bits);
    *leftover_bits += bitwidth;

    n = 0;
    while (*leftover_bits >= 8) {
        out[n++] = *leftover & 0xFF;
        *leftover >>= 8;
        *leftover_bits -= 8;
    }
    return n;
}

// ------------------------------------------------------- digital_stream

#define DS_PACKET_BYTES_MAX  63  // 1 USB FS bulk packet
#define DS_CHANNELS  9  // virtual digital channels b0-b8

// Largest us_per_sample Timer3 can reach with its 1:256 prescaler
// (139808 at 120 MHz).
static inline int
ds_max_us_per_sample(int bus_mhz)
{
    return 65535 * 256 / bus_mhz;
}

// Timer3 setup for sampling every us_per_sample microseconds.
// Returns -1 if the period is unreachable (us_per_sample above
// ds_max_us_per_sample), 0 to free-run without the timer, or 1 with *tckps
// and *pr3 set to the T3CON prescaler code and the PR3 value.
static inline int
ds_timer_config(int us_per_sample, int bus_mhz, int *tckps, int *pr3)
{
    int ps;
    int t;

    if (us_per_sample <= 0) {
        return 0;
    }

    // Reject periods the timer cannot reach even with the 1:256 prescaler.
    // This limit (65535 * 256 / bus_mhz) is far below INT_MAX / bus_mhz, so
    // it also keeps the multiplication below from overflowing.
    if (bus_mhz > 0 && us_per_sample > ds_max_us_per_sample(bus_mhz)) {
        return -1;
    }
    t = us_per_sample * bus_mhz;

    if (t < 2) {
        return 0;
    }

    // Type B timer prescalers: TCKPS 0..7 -> 1:1,2,4,8,16,32,64,256.
    // Pick the smallest one that fits PR3; the guard above ensures 1:256 does.
    *tckps = 0;
    ps = 1;
    while (t / ps > 65535) {
        (*tckps)++;
        ps = (*tckps == 7) ? 256 : (1 << *tckps);
    }
    *pr3 = t / ps - 1;
    return 1;
}

// Stores the virtual channel numbers selected by channel_mask, lowest
// first, in chan_pos and returns how many there are.
static inline int
ds_channel_positions(int channel_mask, int chan_pos[DS_CHANNELS])
{
    int nchans;
    int i;

    nchans = 0;
    for (i = 0; i < DS_CHANNELS; i++) {
        if (channel_mask & (1 << i)) {
            chan_pos[nchans++] = i;
        }
    }
    return nchans;
}

// Most samples of nchans (>= 1) channels that fit one packet.
static inline int
ds_max_samples_per_packet(int nchans)
{
    return (DS_PACKET_BYTES_MAX * 8) / nchans;
}

// Bits per packet for n_samples_per_packet samples of nchans (>= 1)
// channels; n_samples_per_packet <= 0 selects ds_max_samples_per_packet.
// Returns -1 if the packet would not fit DS_PACKET_BYTES_MAX bytes.
static inline int
ds_packet_bits(int nchans, int n_samples_per_packet)
{
    if (n_samples_per_packet <= 0) {
        n_samples_per_packet = ds_max_samples_per_packet(nchans);
    }
    if (n_samples_per_packet > ds_max_samples_per_packet(nchans)) {
        return -1;
    }
    return n_samples_per_packet * nchans;
}

// Appends the bits of vbits at chan_pos[0..nchans-1] to buffer, LSB-first
// starting at bit bit_pos, and returns the new bit position.
static inline int
ds_pack_sample(unsigned char *buffer, int bit_pos, int vbits,
               const int *chan_pos, int nchans)
{
    int i;
    int bit;
    int byte_idx;
    int bit_idx;

    for (i = 0; i < nchans; i++) {
        bit      = (vbits >> chan_pos[i]) & 1;
        byte_idx = bit_pos >> 3;
        bit_idx  = bit_pos & 7;

        if (bit_idx == 0) {
            buffer[byte_idx] = 0;
        }
        buffer[byte_idx] |= (unsigned char)(bit << bit_idx);

        bit_pos++;
    }
    return bit_pos;
}

// Zero-pads a partial packet holding bit_pos bits up to packet_bytes.
static inline void
ds_pad_packet(unsigned char *buffer, int bit_pos, int packet_bytes)
{
    int i;

    for (i = (bit_pos + 7) >> 3; i < packet_bytes; i++) {
        buffer[i] = 0;
    }
}

#endif  // FLEASCOPE_CALC_H
