// Timer3 period selection for digital_stream.
//
// Pure arithmetic with no hardware dependencies, so the host test
// (test/digital_stream_timer_test.c) can include it directly.

#ifndef DIGITAL_STREAM_TIMER_H
#define DIGITAL_STREAM_TIMER_H

// Largest us_per_sample Timer3 can reach with the 1:256 prescaler.
#define DIGITAL_STREAM_MAX_US(bus_mhz)  (65535 * 256 / (bus_mhz))

// Computes the Timer3 tick count (PR3 + 1) and TCKPS for us_per_sample at
// bus_mhz.  Returns 1 with *count in 2..65535 and *tckps in 0..7 on success,
// 0 when the period is too short for the timer (caller free-runs), and -1
// when us_per_sample exceeds DIGITAL_STREAM_MAX_US(bus_mhz).
static inline int
compute_t3(int us_per_sample, int bus_mhz, int *count, int *tckps)
{
    int ps;
    int t;

    if (us_per_sample <= 0) {
        return 0;
    }

    // Reject periods the timer cannot reach even with the 1:256 prescaler.
    // This limit (65535 * 256 / bus_mhz) is far below INT_MAX / bus_mhz, so
    // it also keeps the multiplication below from overflowing.
    if (bus_mhz > 0 && us_per_sample > DIGITAL_STREAM_MAX_US(bus_mhz)) {
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
    *count = t / ps;
    return 1;
}

#endif // DIGITAL_STREAM_TIMER_H
