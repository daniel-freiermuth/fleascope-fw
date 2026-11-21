#include "main.h"
#include "fleascope-calc.h"


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
    configure_pmd_once();

    pin_oc_unroute();

    set_waiting_pin(false);
    set_capturing_pin(false);
}

static int
setup_timer_digital_stream(int us_per_sample)
{
    int bus_mhz;
    int tckps;
    int pr3;
    int rv;

    bus_mhz = (int)(bus_frequency / 1000000);

    rv = ds_timer_config(us_per_sample, bus_mhz, &tckps, &pr3);
    if (rv < 0) {
        printf("us_per_sample too large (max %d)\n", ds_max_us_per_sample(bus_mhz));
    }
    if (rv <= 0) {
        return rv;
    }

    stop_T3();
    TMR3 = 0;
    PR3 = pr3;
    T3CON = tckps << _T3CON_TCKPS_POSITION;
    start_T3();

    IFS0bits.T3IF = 0;
    return 1;
}

static void
digital_stream_data(int channel_mask, int use_timer,
                    int packet_bits, int packet_bytes)
{
    int nchans;
    int chan_pos[DS_CHANNELS];
    int bit_pos;
    int vbits;
    byte buffer[DS_PACKET_BYTES_MAX];

    nchans = ds_channel_positions(channel_mask, chan_pos);

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

        bit_pos = ds_pack_sample(buffer, bit_pos, vbits, chan_pos, nchans);

        if (bit_pos >= packet_bits) {
            printf_write((char *)buffer, packet_bytes);
            bit_pos = 0;
        }
    }

    // Flush any partial packet on stop (zero-pad to packet_bytes).
    if (bit_pos > 0) {
        ds_pad_packet(buffer, bit_pos, packet_bytes);
        printf_write((char *)buffer, packet_bytes);
    }
}

static void
restore_board_digital_stream(void)
{
    pin_oc_route();

    set_capturing_pin(false);
    set_waiting_pin(false);
}

// Entrypoint
void
digital_stream(int channel_mask, int us_per_sample, int n_samples_per_packet)
{
    int use_timer;
    int chan_pos[DS_CHANNELS];
    int nchans;
    int packet_bits;
    int packet_bytes;

    nchans = ds_channel_positions(channel_mask, chan_pos);

    // A missing n_samples_per_packet (<= 0) defaults to the maximum that fits
    // in one 63-byte USB FS bulk packet; larger values would overflow the
    // packet buffer and are rejected.
    packet_bits = ds_packet_bits(nchans, n_samples_per_packet);
    if (packet_bits < 0) {
        printf("n_samples_per_packet too large (max %d)\n", ds_max_samples_per_packet(nchans));
        return;
    }
    packet_bytes = (packet_bits + 7) >> 3;

    setup_board_digital_stream();

    use_timer = 0;
    if (us_per_sample > 0) {
        use_timer = setup_timer_digital_stream(us_per_sample);
        if (use_timer < 0) {
            restore_board_digital_stream();
            return;
        }
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
