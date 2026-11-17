// Host test for the digital_stream Timer3 period selection (compute_t3).
// Run via test/digital_stream_timer.sh.

#include <stdio.h>

#include "../cpustick/digital-stream-timer.h"

static int failures;

#define CHECK(cond) \
    do { \
        if (! (cond)) { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++; \
        } \
    } while (0)

// Type B timer prescaler for a TCKPS value.
static int
prescaler(int tckps)
{
    return tckps == 7 ? 256 : 1 << tckps;
}

// Expects success with the given tick count and TCKPS.
static void
expect_ok(int us, int bus_mhz, int want_count, int want_tckps)
{
    int count = -1;
    int tckps = -1;
    int rv;

    rv = compute_t3(us, bus_mhz, &count, &tckps);
    if (rv != 1 || count != want_count || tckps != want_tckps) {
        printf("FAIL us=%d bus_mhz=%d: rv=%d count=%d tckps=%d, want 1 %d %d\n",
               us, bus_mhz, rv, count, tckps, want_count, want_tckps);
        failures++;
    }
}

static void
expect_rv(int us, int bus_mhz, int want_rv)
{
    int count;
    int tckps;
    int rv;

    rv = compute_t3(us, bus_mhz, &count, &tckps);
    if (rv != want_rv) {
        printf("FAIL us=%d bus_mhz=%d: rv=%d, want %d\n", us, bus_mhz, rv, want_rv);
        failures++;
    }
}

// Every accepted us_per_sample must give a PR3 that fits 16 bits with the
// smallest prescaler that fits, and every larger one must be rejected.
static void
check_all(int bus_mhz)
{
    int max = DIGITAL_STREAM_MAX_US(bus_mhz);
    int us;

    for (us = 1; us <= max; us++) {
        int count = -1;
        int tckps = -1;
        int t = us * bus_mhz;
        int rv = compute_t3(us, bus_mhz, &count, &tckps);

        if (t < 2) {
            CHECK(rv == 0);
            continue;
        }
        if (rv != 1 || tckps < 0 || tckps > 7 || count < 2 || count > 65535
            || count != t / prescaler(tckps)
            || (tckps > 0 && t / prescaler(tckps - 1) <= 65535)) {
            printf("FAIL us=%d bus_mhz=%d: rv=%d count=%d tckps=%d\n",
                   us, bus_mhz, rv, count, tckps);
            failures++;
            return;
        }
    }
    expect_rv(max + 1, bus_mhz, -1);
}

int
main(void)
{
    // Max-period guard at the 120 MHz bus: 65535 * 256 / 120 = 139808.
    CHECK(DIGITAL_STREAM_MAX_US(120) == 139808);
    expect_ok(139808, 120, 65535, 7);
    expect_rv(139809, 120, -1);
    expect_rv(0x7fffffff, 120, -1);

    // 1:1 holds through t == 65535; t == 65536 moves to 1:2.
    expect_ok(65535, 1, 65535, 0);
    expect_ok(65536, 1, 32768, 1);

    // t / 64 > 65535 jumps from TCKPS 6 (1:64) to TCKPS 7 (1:256), not 1:128.
    expect_ok(65535 * 64, 1, 65535, 6);
    expect_ok(65536 * 64, 1, 16384, 7);

    // Free-run: non-positive period, or t < 2.
    expect_rv(0, 120, 0);
    expect_rv(-1, 120, 0);
    expect_rv(5, 0, 0);
    expect_rv(1, 1, 0);
    expect_ok(2, 1, 2, 0);

    check_all(120);
    check_all(1);
    check_all(7);

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("digital_stream timer: all tests passed\n");
    return 0;
}
