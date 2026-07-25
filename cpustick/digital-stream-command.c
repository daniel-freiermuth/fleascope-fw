#include "main.h"

#if ! defined(__32MK0512GPK064__) && ! defined(__32MK0512MCM064__)
#error  only __32MK__ is supported for digital stream
#endif

// Command: digital_stream 0x<mask> <us_per_sample> <n_samples_per_packet>
//
// Samples the N = popcount(channel_mask) virtual digital channels (b0-b8,
// mapped from PORTB) and streams binary packets over USB.
//
// Packet size is pre-negotiated: every packet contains exactly
// n_samples_per_packet complete samples packed LSB-first, lowest virtual
// channel first, into ceil(n_samples_per_packet * N / 8) bytes.
// No inline header — the host knows the size from the command parameters.
//
// The host chooses n_samples_per_packet to balance throughput vs latency:
//   max rate  → floor(63 * 8 / N)  samples  (fills one USB FS bulk packet)
//   slow rate → ~50 ms worth of samples      (keeps the host read responsive)
//
// Physical-to-virtual bit mapping (same as scope-command):
//   virt:  b8    b7    b6    b5    b4    b3    b2    b1    b0
//   phys: rb15  rb14  rb13  rb12  rb11  rb10   rb6   rb5   rb4

static void
setup_board_digital_stream(void)
{
    static int once;

    if (!once) {
        once = 1;

        SYSKEY = 0x00000000;
        SYSKEY = 0xAA996655;
        SYSKEY = 0x556699AA;

        CFGCONbits.PMDLOCK = 0;

        PMD1 = 0x00000310;
        PMD2 = 0x0013001a;
        PMD3 = 0xfbc0ffff;
        PMD4 = 0x0fff01f8;
        PMD5 = 0xf20f3f3f;
        PMD6 = 0x0f0d0300;

        CFGCONbits.PMDLOCK = 1;

        SYSKEY = 0x33333333;
    }

    RPE14R = 0;
    RPE15R = 0;
    RPG9R  = 0;
    RPG8R  = 0;
    RPG7R  = 0;
    RPG6R  = 0;

    set_waiting_pin(false);
    set_capturing_pin(false);
}

static int
setup_timer_digital_stream(int us_per_sample)
{
    int bus_mhz;
    int ps;
    int t;

    if (us_per_sample <= 0) {
        return 0;
    }

    bus_mhz = (int)(bus_frequency / 1000000);

    // Cap us_per_sample to prevent signed integer overflow in the
    // multiplication below.  Capped values still exceed the 16-bit timer
    // maximum and will be clamped to 65535 as intended.
    if (bus_mhz > 0 && us_per_sample > 2147483647 / bus_mhz) {
        us_per_sample = 2147483647 / bus_mhz;
    }
    t = us_per_sample * bus_mhz;

    if (t < 2) {
        return 0;
    }

    ps = 1;
    if (t > 65535) {
        ps = 16;
        t = us_per_sample * bus_mhz / 16;
        if (t > 65535) {
            t = 65535;
        }
    }

    stop_T3();
    TMR3 = 0;
    PR3 = (uint16)t - 1;
    T3CON = (ps == 16 ? 4 : 0) << _T3CON_TCKPS_POSITION;
    start_T3();

    IFS0bits.T3IF = 0;
    return 1;
}

static void
digital_stream_data(int channel_mask, int use_timer,
                    int packet_bits, int packet_bytes)
{
    int nchans;
    int chan_pos[9];
    int i;
    int bit_pos;
    int byte_idx;
    int bit_idx;
    int vbits;
    int bit;
    byte buffer[63];  // max 63 bytes = 1 USB FS bulk packet

    nchans = 0;
    for (i = 0; i < 9; i++) {
        if (channel_mask & (1 << i)) {
            chan_pos[nchans++] = i;
        }
    }

    bit_pos = 0;

    while (run_stop) {
        if (use_timer) {
            // If T3IF is already set the timer fired during the last flush —
            // we missed that sample window and should catch up immediately
            // rather than deferring by another full period.
            while (!IFS0bits.T3IF && run_stop) {}
            IFS0bits.T3IF = 0;
        }

        vbits = ptovbits(PORTB) & channel_mask;

        for (i = 0; i < nchans; i++) {
            bit      = (vbits >> chan_pos[i]) & 1;
            byte_idx = bit_pos >> 3;
            bit_idx  = bit_pos & 7;

            if (bit_idx == 0) {
                buffer[byte_idx] = 0;
            }
            buffer[byte_idx] |= (byte)(bit << bit_idx);

            bit_pos++;
        }

        if (bit_pos >= packet_bits) {
            printf_write((char *)buffer, packet_bytes);
            bit_pos = 0;
        }
    }

    // Flush any partial packet on stop (zero-pad to packet_bytes).
    if (bit_pos > 0) {
        int tail;
        tail = (bit_pos + 7) >> 3;
        for (i = tail; i < packet_bytes; i++) {
            buffer[i] = 0;
        }
        printf_write((char *)buffer, packet_bytes);
    }
}

static void
restore_board_digital_stream(void)
{
    RPE14R = 6;
    RPE15R = 6;
    RPG9R  = 5;
    RPG8R  = 5;
    RPG7R  = 5;
    RPG6R  = 5;

    set_capturing_pin(false);
    set_waiting_pin(false);
}

// Entrypoint
void
digital_stream(int channel_mask, int us_per_sample, int n_samples_per_packet)
{
    int use_timer;
    int nchans;
    int packet_bits;
    int packet_bytes;

    nchans = 0;
    {
        int i;
        for (i = 0; i < 9; i++) {
            if (channel_mask & (1 << i)) {
                nchans++;
            }
        }
    }

    // If the host didn't supply n_samples_per_packet, default to the maximum
    // that fits in one 63-byte USB FS bulk packet.
    if (n_samples_per_packet <= 0) {
        n_samples_per_packet = (63 * 8) / nchans;
    }

    // Reject values that would overflow the 63-byte packet buffer.
    if (n_samples_per_packet > (63 * 8) / nchans) {
        printf("n_samples_per_packet too large (max %d)\n", (63 * 8) / nchans);
        return;
    }

    packet_bits  = n_samples_per_packet * nchans;
    packet_bytes = (packet_bits + 7) >> 3;

    setup_board_digital_stream();

    use_timer = 0;
    if (us_per_sample > 0) {
        use_timer = setup_timer_digital_stream(us_per_sample);
    }

    run_stop = 1;
    main_command = NULL;
    terminal_command_ack(false);
    terminal_command_discard(true);

    set_capturing_pin(true);

    digital_stream_data(channel_mask, use_timer, packet_bits, packet_bytes);

    if (use_timer) {
        stop_T3();
    }

    terminal_command_discard(false);

    restore_board_digital_stream();
}
