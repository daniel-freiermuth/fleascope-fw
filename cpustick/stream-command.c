#include "main.h"

#if ! defined(__32MK0512GPK064__) && ! defined(__32MK0512MCM064__)
#error  only __32MK__ interleaved adc dma is supported for now
#endif

// XXX -- is there any way to use dma counts to get accurate trigger position mid-capture???

// hardware configuration: ra0, ra1, rb0, rb1, ra11 interleaved analog input

// rc10 is wave out from CDAC2 or OC11

#define SAMPLES  (2000*MSPS/20)  // approximate width of display canvas
#define EXTRA  200  // extra samples in case web page wants to fine-scale

// command line:
// <sampleinterval> [+-=](<voltage>|0x<bits>) [<slew>|0x<mask>] [<delaysamples>]
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

// adcbuffer is an integral dmabuffer size so we don't need to checks for wraps mid-copy


// Ideas for config
// - bitwidth
// - channels
void
setup_board_stream()
{
    static int once;

    // Clock Config
    if (! once) {
        once = 1;
        /* unlock system for clock configuration */
        SYSKEY = 0x00000000;
        SYSKEY = 0xAA996655;
        SYSKEY = 0x556699AA;

        CFGCONbits.PMDLOCK = 0;

        PMD1 = 0x00000310;  // EEMD CTMUMD DAC1
        PMD2 = 0x0013001a;  // OPA5MD OPA2MD OPA1MD, CMP5MD C4MPMD CMP2MD
        PMD3 = 0xfbc0ffff;  // all OC/IC except OC11MD OC6MD OC5MD OC4MD OC3MD OC2MD OC1MD
        PMD4 = 0x0fff01f8;  // all PWM/T except T3 T2 T1
        PMD5 = 0xf20f3f3f;  // all CAN/USB/I2C/SPI/U except USB1
        PMD6 = 0x0f0d0300;  // all QE/PMP/REFO except REFO3 REFO4

        CFGCONbits.PMDLOCK = 1;

        /* Lock system since done with clock configuration */
        SYSKEY = 0x33333333;
    }
    
    // Setup pins
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

    set_waiting_pin(true);  // we are waiting for trigger
    set_capturing_pin(false);  // we are waiting for trigger

    // ???
    RPE14R = 0;  // undo OC5 -> RE14
    RPE15R = 0;  // undo OC6 -> RE15
    RPG9R = 0;  // undo OC3 -> RG9
    RPG8R = 0;  // undo OC4 -> RG8
    RPG7R = 0;  // undo OC2 -> RG7
    RPG6R = 0;  // undo OC1 -> RG6
}

void setup_capture_timer(uint t) {
    // Configure adc trigger timer

    // set TMR3 to 3.6 MHz divided by horizontal scale
    //           20    ->   180 kHz timer
    //           40    ->   90 kHz timer // not even without averaging
    //           50 // not even without averaging
    //           60 // doesn't work with averaging, but works without
    //           70    ->   51 kHz // works well with averaging
    //           80    ->   45 kHz timer //nice
    //           1.250    ->  2.88 kHz timer
    //           2.500    ->  1.44 kHz timer
    //           5.000    ->  0.72 kHz timer
    //           10.000    -> 0.36 kHz timer
    //           20.000    -> 0.18 kHz timer
    assert(t > 1 && t < 0x10000);  // 16 bit register

    // use 100% of T3 trigger for adc4
    stop_T3();
    TMR3 = 0;
    PR3 = t-1;
    T3CON = 0 <<_T3CON_TCKPS_POSITION;  // 1:ps prescale

    // configure adc trigger output compares

    // use 20% of T3 trigger for adc0
    OC1CONbits.ON = 0;
    while(OC1CONbits.ON);
    OC1CONbits.OCTSEL = 1;  // timer 3
    OC1CONbits.OCM = 5;  // initial low, continuous high pulses
    OC1R = 0;
    OC1RS = 2;
    OC1CONbits.ON = 1;

    // use 40% of T3 trigger for adc1
    OC2CONbits.ON = 0;
    while(OC2CONbits.ON);
    OC2CONbits.OCTSEL = 1;  // timer 3
    OC2CONbits.OCM = 5;  // initial low, continuous high pulses
    OC2R = 0;
    OC2RS = 2;
    OC2CONbits.ON = 1;

    // use 60% of T3 trigger for adc2
    OC3CONbits.ON = 0;
    while(OC3CONbits.ON);
    OC3CONbits.OCTSEL = 1;  // timer 3
    OC3CONbits.OCM = 5;  // initial low, continuous high pulses
    OC3R = 0;
    OC3RS = 2;
    OC3CONbits.ON = 1;

    // use 80% of T3 trigger for adc3
    OC4CONbits.ON = 0;
    while(OC4CONbits.ON);
    OC4CONbits.OCTSEL = 1;  // timer 3
    OC4CONbits.OCM = 5;  // initial low, continuous high pulses
    OC4R = 0;
    OC4RS = 2;
    OC4CONbits.ON = 1;
}

// prepare for DMA to capture samples imminently -- THIS IS EVERYTHING EXCEPT "T3CONSET = _T3CON_ON_MASK;"
void
prepare_dma()
{
    reset_buffers();

    TMR3 = 0;

    ADCDSTATbits.DMAEN = 1;
    ADCCON3bits.DIGEN0 = 1;      // Enable ADC
    ADCCON3bits.DIGEN1 = 1;      // Enable ADC
    ADCCON3bits.DIGEN2 = 1;      // Enable ADC
    ADCCON3bits.DIGEN3 = 1;      // Enable ADC
    ADCCON3bits.DIGEN4 = 1;      // Enable ADC
}

void
stream_data(uint averaging, uint bitwidth)
{
    int sent = 0;
    // wait for trigger and post samples; kill time and go easy on the bus
    short* adcbuffer = get_adc_buffer();
    uint leftover = 0; // |•••••••••••••|new_value|older_value|partially_send_even_older_value|
    uint leftover_bits = 0;

    uint bits_per_sample;
    if (averaging == 1) {
        bits_per_sample = 12;
    } else if (averaging == 2) {
        bits_per_sample = 13;
    } else if (averaging <= 4) {
        bits_per_sample = 14;
    } else {
        bits_per_sample = 15;
    }
    

    while (run_stop) {
        // TODO: Think about all int that could wrap
        while (run_stop && sent < get_current()) {
            int v=0;
            // int v= adcbuffer[sent%get_adcbuffer_length()];
            for (int i = 0; i < averaging; i++) {
                v += adcbuffer[sent++%get_adcbuffer_length()];
            }
            sent += 5 - averaging;

            int bits_to_shift = bits_per_sample - bitwidth;
            if (bits_to_shift > 0) {
                v = (v + (1 << (bits_to_shift -1))) >> bits_to_shift;
            } else {
                v = v << (-bits_to_shift);
            }

            leftover = leftover | (v << leftover_bits);
            leftover_bits += bitwidth;

            while (leftover_bits >= 8) {
                printf("%c", leftover & 0xFF);
                leftover >>= 8;
                leftover_bits -= 8;
            }
        }
        // Testing for writing the data away
        // printf("\n");
    }
}

void stop_capture() {
    // N.B. THIS STOPS EVERYTHING!
    stop_T3();

    ADCCON3bits.DIGEN0 = 0;      // Disable ADC
    ADCCON3bits.DIGEN1 = 0;      // Disable ADC
    ADCCON3bits.DIGEN2 = 0;      // Disable ADC
    ADCCON3bits.DIGEN3 = 0;      // Disable ADC
    ADCCON3bits.DIGEN4 = 0;      // Disable ADC
    ADCDSTATbits.DMAEN = 0;

    CM1CONbits.ON = 0;
    while(CM1CONbits.ON);

    DAC3CONbits.ON = 0;
    while(DAC3CONbits.ON);
}

void
restore_board()
{
    // N.B. turn OCs off before reassigning to pins!
    OC1CONbits.ON = 0;
    while(OC1CONbits.ON);
    OC2CONbits.ON = 0;
    while(OC2CONbits.ON);
    OC3CONbits.ON = 0;
    while(OC3CONbits.ON);
    OC4CONbits.ON = 0;
    while(OC4CONbits.ON);

    RPE14R = 6;  // OC5 -> RE14
    RPE15R = 6;  // OC6 -> RE15
    RPG9R = 5;  // OC3 -> RG9
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    RPG6R = 5;  // OC1 -> RG6

    set_capturing_pin(false);  // we are no longer capturing
    set_waiting_pin(false);  // we are no longer waiting for trigger
}

// Entrypoint
void stream(uint sample_length, uint averaging, uint bitwidth)
{
    // Prepare board
    // Also does something with regard to the analog trigger
    setup_board_stream();
    setup_capture_timer(sample_length);

    run_stop = 1;
    main_command = NULL;
    terminal_command_ack(false);
    terminal_command_discard(true);

    // Does more things with analog trigger
    prepare_dma();

    // just start
    start_T3();

    if (run_stop) {
        set_capturing_pin(true);  // we are capturing
    }

    stream_data(averaging, bitwidth);
    stop_capture();

    terminal_command_discard(false);

    restore_board();
}