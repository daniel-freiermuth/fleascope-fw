#include "main.h"
#include "fleascope-calc.h"


// we sample continuously while waiting for the trigger, so the reported window can start before
// the trigger (negative delay); edge triggers are located in the recorded data, see locate_trigger()

// hardware configuration: ra0, ra1, rb0, rb1, ra11 interleaved analog input

// rc10 is wave out from CDAC2 or OC11

#define SAMPLES  (2000*MSPS/20)  // approximate width of display canvas
#define EXTRA  200  // extra samples in case web page wants to fine-scale

// how far back from its detection we look for a trigger edge; scope_isr preempts the trigger poll
// loop for less than one adc chunk (it must copy a chunk faster than the next one arrives)
#define TRIGGER_SEARCH  (2*ADC_CHUNK_SAMPLES)

// command line:
// <sampleinterval> [+-=](<voltage>|0x<bits>) [<slew>|0x<mask>] [[-]<delaysamples>]
//          -10    -> 18  Msps -> 3.6  MHz timer with 5x interleave and 10x magnification
//          -5     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 5x magnification
//          -2     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 2x magnification
// number1 = 1     -> 18  Msps -> 3.6  MHz timer with 5x interleave
//           2     -> 9   Msps -> 1.8  MHz timer with 5x interleave
//           5     -> 3.6 Msps -> 0.72 MHz timer with 5x interleave
//           10    -> 1.8 Msps -> 0.36 MHz timer with 5x interleave
//           20    -> 0.9 Msps -> 0.18 MHz timer with 5x interleave
//           50    -> 360 ksps -> 72   kHz timer with 5x interleave
//           ...
//           10000 -> 1.8 ksps -> 0.36 kHz timer with 5x interleave
//           20000 -> 0.9 ksps -> 0.18 kHz timer with 5x interleave

volatile int g;

// Clock, pin and route configuration
void
setup_board_scope()
{
    configure_pmd_once();
    
    pin_declare(PIN_A0, pin_type_digital_output, 0);
    pin_declare(PIN_A1, pin_type_digital_output, 0);
    pin_declare(PIN_A2, pin_type_digital_output, 0);
    pin_declare(PIN_A3, pin_type_digital_output, 0);
    pin_declare(PIN_A4, pin_type_digital_output, 0);
    pin_declare(PIN_A5, pin_type_digital_output, 0);
    pin_declare(PIN_A6, pin_type_digital_output, 0);
    pin_declare(PIN_A7, pin_type_digital_output, 0);
    pin_declare(PIN_A8, pin_type_digital_output, 0);

    pin_declare(PIN_E2, pin_type_digital_output, 0);
    pin_declare(PIN_E3, pin_type_digital_output, 0);

    pin_set(PIN_E2, pin_type_digital_output, 0, 0);  // we are waiting for trigger
    pin_set(PIN_E3, pin_type_digital_output, 0, 1);  // we are waiting for trigger

    pin_oc_unroute();
}

int setup_capture_timer_scope(int number1) {
    int t;
    int ps;
    int bus_mhz;
    // configure adc capture timer

    // Reject sample intervals whose 1:16 count does not fit the 16-bit PR3
    // (above 31457 at 120 MHz); this also keeps the count from overflowing.
    bus_mhz = (int)(bus_frequency / 1000000);
    t = scope_timer_count(number1, bus_mhz, &ps);
    if (t < 0) {
        printf("sample interval too large (max %d)\n", scope_max_interval(bus_mhz));
        return -1;
    }
    assert(t > 1 && t < 0x10000);  // 16 bit register

    // use 100% of T3 trigger for adc4
    T3CONCLR = _T3CON_ON_MASK;
    g++;
    TMR3 = 0;
    PR3 = t-1;
    assert(ps == 1 || ps == 16);
    T3CON = (ps==16?4:0) <<_T3CON_TCKPS_POSITION;  // 1:ps prescale

    // configure adc trigger output compares

    // use 20% of T3 trigger for adc0
    OC1CONbits.ON = 0;
    g++;
    OC1CONbits.OCTSEL = 1;  // timer 3
    OC1CONbits.OCM = 5;  // initial low, continuous high pulses
    OC1R = t/INTERLEAVE-1;
    OC1RS = t/INTERLEAVE+1;
    OC1CONbits.ON = 1;

    // use 40% of T3 trigger for adc1
    OC2CONbits.ON = 0;
    g++;
    OC2CONbits.OCTSEL = 1;  // timer 3
    OC2CONbits.OCM = 5;  // initial low, continuous high pulses
    OC2R = 2*t/INTERLEAVE-1;
    OC2RS = 2*t/INTERLEAVE+1;
    OC2CONbits.ON = 1;

    // use 60% of T3 trigger for adc2
    OC3CONbits.ON = 0;
    g++;
    OC3CONbits.OCTSEL = 1;  // timer 3
    OC3CONbits.OCM = 5;  // initial low, continuous high pulses
    OC3R = 3*t/INTERLEAVE-1;
    OC3RS = 3*t/INTERLEAVE+1;
    OC3CONbits.ON = 1;

    // use 80% of T3 trigger for adc3
    OC4CONbits.ON = 0;
    g++;
    OC4CONbits.OCTSEL = 1;  // timer 3
    OC4CONbits.OCM = 5;  // initial low, continuous high pulses
    OC4R = 4*t/INTERLEAVE-1;
    OC4RS = 4*t/INTERLEAVE+1;
    OC4CONbits.ON = 1;

    return 0;
}

// prepare for DMA to capture samples imminently -- THIS IS EVERYTHING EXCEPT "T3CONSET = _T3CON_ON_MASK;"
void
prepare_dma_scope()
{
    reset_buffers();

    TMR3 = 0;

    ADCDSTATbits.DMAEN = 1;
    ADCCON3bits.DIGEN0 = 1;      // Enable ADC
    ADCCON3bits.DIGEN1 = 1;      // Enable ADC
    ADCCON3bits.DIGEN2 = 1;      // Enable ADC
    ADCCON3bits.DIGEN3 = 1;      // Enable ADC
    ADCCON3bits.DIGEN4 = 1;      // Enable ADC

    DCH0CONbits.CHEN = 1;
    DCH1CONbits.CHEN = 1;
    DCH2CONbits.CHEN = 1;
    DCH3CONbits.CHEN = 1;
    DCH4CONbits.CHEN = 1;
}

void enable_analog_trigger(int number1, int number2, bool fall) {
    // configure analog comparator voltage reference

    // set dac3 to number2
    DAC3CONbits.REFSEL = 3;  // Positive reference voltage = AVDD
    DAC3CONbits.DACDAT = number2 << 2;  // 10 bit input to 12 bit register

    //DAC3CONbits.DACOE = 1;  // enable DAC3OUT on CDAC3/RA8
    DAC3CONbits.ON = 1;
    CM1CONbits.HYSPOL = fall?0:1;  // 0 -> rising edge; 1 -> falling edge
    CM1CONbits.CFSEL = 0;  // SYSCLK
    CM1CONbits.CFDIV = number1==1?1:4;  // digital filter divide by 16 except at highest sample rate, when we divide by 2
    CM1CONbits.CFLTREN = 1;  // digital filter divides by 3 again
    CM1CONbits.HYSSEL = 2;  // medium hysteresis
    CM1CONbits.CPOL = 1;  // 1 -> inverted (because CMP+ is DAC3)
    CM1CONbits.EVPOL = fall?2:1;  // 1 -> rising edge; 2 -> falling edge
    CM1CONbits.CREF = 1;  // CMP+ is DAC3
    CM1CONbits.CCH = 3;  // CMP- is RPB1
    //RPC10R = 7;  // C1OUT on RC10
    //CM1CONbits.COE = 1;  // enable C1OUT on RC10

    // enable the comparator
    delay(2);
    CM1CONbits.ON = 1;
    delay(2);
}

// virt:   b8,   b7,   b6,   b5,   b4,   b3,   b2,   b1,   b0
// phys: rb15, rb14, rb13, rb12, rb11, rb10,  rb6,  rb5,  rb4

int
ptovbits(int bits)
{
    return ((bits&0xfc00)>>7) | ((bits & 0x0070)>>4);
}

int
vtopbits(int bits)
{
    return ((bits&0x01f8)<<7) | ((bits&0x0007)<<4);
}

void
wait_for_analog_trigger(TriggerEdge edge)
{
    int i;
    int r;
    int loop;
    int loops;

    loops = 1000000;  // XXX -- tune this!

    // delay a sub-ms random time to make a better demo with 1kHz test signal on auto/level trigger!
    r = random_32()%blips_per_ms;
    for (i = 0; i < r; i++) {
        blip();
    }

    loop = 0;

    // wait for trigger
    // start looking for trigger now
    CM1CONbits.COUT = 0;
    CM1CONbits.CEVT = 0;

    switch (edge) {
        case AUTO:
            while (!CM1CONbits.COUT && loop++ <= loops && run_stop) {
            }
            break;
        case LEVEL_AKA_MATCHING:
            while (!CM1CONbits.COUT && run_stop) {
            }
            break;
        case FALL_AKA_STOP:
        case RISE_AKA_START:
            // edge polarity is configured in enable_analog_trigger()
            while (!CM1CONbits.CEVT && run_stop) {
            }
            break;
        default:
            assert(0);
    }
}

void
wait_for_digital_trigger(TriggerEdge edge, int number2, int number3)
{
    int i;
    int r;
    int loop;
    int loops;

    loops = 1000000;  // XXX -- tune this!

    // delay a sub-ms random time to make a better demo with 1kHz test signal on auto/level trigger!
    r = random_32()%blips_per_ms;
    for (i = 0; i < r; i++) {
        blip();
    }

    loop = 0;

    // wait for trigger
    int number2x;
    int number3x;
    bool value_found_now;
    bool value_found_previously = -1;

    number2x = vtopbits(number2);
    number3x = vtopbits(number3);

    switch (edge) {
    case AUTO:
        while ((PORTB & number3x) != number2x && loop++ <= loops && run_stop) {
        }
        break;
    case LEVEL_AKA_MATCHING:
        while ((PORTB & number3x) != number2x && run_stop) {
        }
        break;
    case RISE_AKA_START:
        value_found_now = (PORTB & number3x) == number2x;
        value_found_previously = value_found_now;
        while ((!value_found_now || value_found_previously == value_found_now) && run_stop) {
            value_found_previously = value_found_now;
            value_found_now = (PORTB & number3x) == number2x;
        }
        break;
    case FALL_AKA_STOP:
        value_found_now = (PORTB & number3x) == number2x;
        value_found_previously = value_found_now;
        while ((value_found_now || value_found_previously == value_found_now) && run_stop) {
            value_found_previously = value_found_now;
            value_found_now = (PORTB & number3x) == number2x;
        }
        break;
    default:
        assert(0);
    }
}

// wait until scope_isr has copied all samples before end into adcbuffer
static void
wait_for_samples(int end)
{
    // N.B. current stays below INT_MAX: it is rebased while waiting for the trigger
    while (run_stop && (int)get_current() < end) {
        // XXX -- this causes possible power supply noise on 2.6 hacked to use unregulated +/-5V for op amp
        //__asm__("wait");
    }
}

// generate test pattern for loopback testing from a* to b* right after the trigger
static void
loopback_test_pattern()
{
    int i;

    if (pin_get(PIN_S1, pin_type_digital_input, 0) == 1) {
        return;
    }

    for (i = 0; i < 9; i++) {
        pin_set(PIN_A0+i, pin_type_digital_output, 0, 1);
        pin_set(PIN_A0+i, pin_type_digital_output, 0, 0);
    }

    // resume timer interrupts
    // XXX -- should this be after sample complete???
    IEC0bits.T1IE = 1;
}

// Return the first sample at which the trigger condition became true.  detected is the sample
// position when the poll loop noticed the trigger; scope_isr can preempt the poll loop, so edges
// are searched for in the recorded data with the trigger's own condition.  Level and auto
// triggers have no edge and trigger at detected.
// N.B. samples up to detected+INTERLEAVE-1 must be in adcbuffer; no sample before lowest is used.
static int
locate_trigger(int detected, int lowest, TriggerEdge edge, TriggerChannel channel, int trigger_value, int bits_of_interest)
{
    int hi;
    int lo;
    int s;

    if (edge != RISE_AKA_START && edge != FALL_AKA_STOP) {
        return detected;
    }

    // the newest sample that may have been taken before the poll loop noticed the trigger
    hi = detected+INTERLEAVE-1;
    lo = detected-TRIGGER_SEARCH;
    if (lo < lowest) {
        lo = lowest;
    }

    switch (channel) {
    case ANALOG:
        // CM1 compares rb1 (also sampled by adc3) against dac3, which gets the 10 bit level << 2
        s = scope_find_analog_edge(get_adc_buffer(), get_adcbuffer_length(), hi, lo,
                                   trigger_value << 2, edge == RISE_AKA_START);
        break;
    case DIGITAL:
        s = scope_find_digital_edge(get_bits_buffer()[0], get_bitbuffer_length(), hi, lo,
                                    vtopbits(trigger_value), vtopbits(bits_of_interest), edge == RISE_AKA_START);
        break;
    default:
        assert(0);
        s = -1;
    }

    // no edge in the data (e.g. dac3/adc mismatch): fall back to the poll loop's position
    return s < 0 ? detected : s;
}

void stop_capture_scope() {
    // N.B. THIS STOPS EVERYTHING!
    T3CONCLR = _T3CON_ON_MASK;
    g++;

    ADCCON3bits.DIGEN0 = 0;      // Disable ADC
    ADCCON3bits.DIGEN1 = 0;      // Disable ADC
    ADCCON3bits.DIGEN2 = 0;      // Disable ADC
    ADCCON3bits.DIGEN3 = 0;      // Disable ADC
    ADCCON3bits.DIGEN4 = 0;      // Disable ADC
    ADCDSTATbits.DMAEN = 0;

    CM1CONbits.ON = 0;
    g++;

    DAC3CONbits.ON = 0;
    g++;

    DCH0ECONbits.CABORT = 1;
    DCH1ECONbits.CABORT = 1;
    DCH2ECONbits.CABORT = 1;
    DCH3ECONbits.CABORT = 1;
    DCH4ECONbits.CABORT = 1;
}

void
send_result(int start)
{
    int i;
    int bits;
    short(* bitsbuffer)[2*128*10] = get_bits_buffer();
    short* adcbuffer = get_adc_buffer();

    for (i = start; i < start+SAMPLES+EXTRA; i++) {
        bits = bitsbuffer[i%INTERLEAVE][(i/INTERLEAVE)%get_bitbuffer_length()];
        printf("%d,%x\n", adcbuffer[i%get_adcbuffer_length()], ptovbits(bits));
    }
}

void
undo_board_config()
{
    // N.B. turn OCs off before reassigning to pins!
    OC1CONbits.ON = 0;
    g++;
    OC2CONbits.ON = 0;
    g++;
    OC3CONbits.ON = 0;
    g++;
    OC4CONbits.ON = 0;
    g++;

    pin_oc_route();

    pin_set(PIN_E2, pin_type_digital_output, 0, 0);  // we have no longer triggered
    pin_set(PIN_E3, pin_type_digital_output, 0, 0);  // we are no longer waiting for trigger
}

// Entrypoint
// delay_samples < 0 starts the reported window before the trigger (down to -SCOPE_MAX_PRETRIGGER)
void scope(int sample_length, TriggerEdge edge, TriggerChannel channel, int trigger_value, int bits_of_interest, int delay_samples)
{
    int detected;
    int lowest;
    int start;

    assert(delay_samples >= -SCOPE_MAX_PRETRIGGER);

    setup_board_scope();
    if (setup_capture_timer_scope(sample_length) < 0) {
        undo_board_config();
        return;
    }

    run_stop = 1;
    main_command = NULL;
    terminal_command_ack(false);
    terminal_command_discard(true);

    prepare_dma_scope();

    // N.B. enable_analog_trigger() waits with delay(), which counts timer 1
    // interrupts, so it must run before they are turned off
    if (channel == ANALOG) {
        enable_analog_trigger(sample_length, trigger_value, edge==FALL_AKA_STOP);
    }

    // temporarily turn off the timer interrupt
    IEC0bits.T1IE = 0;

    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
    // we sample while waiting for the trigger, so the ring always holds the history before it;
    // current may be rebased while we hold no sample positions, as the wait is unbounded
    set_current_rebasable(true);
    T3CONSET = _T3CON_ON_MASK;

    // record the history a negative delay reports before arming the trigger
    lowest = delay_samples < 0 ? -delay_samples : 0;
    wait_for_samples(lowest);

    switch (channel) {
    case ANALOG:
        wait_for_analog_trigger(edge);
        break;
    case DIGITAL:
        wait_for_digital_trigger(edge, trigger_value, bits_of_interest);
        break;
    default:
        assert(0);
    }

    set_current_rebasable(false);

    start = 0;
    if (run_stop) {
        detected = get_sample_position();

        pin_set(PIN_E2, pin_type_digital_output, 0, 1);  // we have triggered
        loopback_test_pattern();

        // locate the trigger in the recorded data once it is all in adcbuffer; lowest keeps the
        // window from starting before the history we recorded
        wait_for_samples(detected+INTERLEAVE);
        start = locate_trigger(detected, lowest, edge, channel, trigger_value, bits_of_interest) + delay_samples;
    }
    pin_set(PIN_E3, pin_type_digital_output, 0, 0);  // we are no longer waiting for trigger

    wait_for_samples(start+SAMPLES+EXTRA);
    // resume timer interrupts
    IEC0bits.T1IE = 1;

    stop_capture_scope();

    if (run_stop) {
        send_result(start);
    }

    run_stop = false;
    terminal_command_discard(false);

    undo_board_config();
}