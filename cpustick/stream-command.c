#include "main.h"
#include "fleascope-calc.h"





// XXX -- is there any way to use dma counts to get accurate trigger position mid-capture???

// hardware configuration: ra0, ra1, rb0, rb1, ra11 interleaved analog input

// adcbuffer is an integral dmabuffer size so we don't need to checks for wraps mid-copy


// Ideas for config
// - bitwidth
// - channels
void
setup_board_stream()
{
    // Clock Config
    configure_pmd_once();
    
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
    pin_oc_unroute();
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
    uint sent = 0;
    // post samples; kill time and go easy on the bus
    short* adcbuffer = get_adc_buffer();
    uint leftover = 0; // |•••••••••••••|new_value|older_value|partially_send_even_older_value|
    uint leftover_bits = 0;

    uint bits_per_sample = stream_bits_per_sample(averaging);
    byte out[3];

    while (run_stop) {
        while (run_stop && stream_samples_pending(get_current(), sent)) {
            int v=0;
            // int v= adcbuffer[sent%get_adcbuffer_length()];
            for (int i = 0; i < averaging; i++) {
                v += adcbuffer[sent++%get_adcbuffer_length()];
            }
            sent += 5 - averaging;

            v = stream_scale_sample(v, bits_per_sample, bitwidth);

            int n = stream_pack_sample(&leftover, &leftover_bits, v, bitwidth, out);
            for (int i = 0; i < n; i++) {
                printf("%c", out[i]);
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

    pin_oc_route();

    set_capturing_pin(false);  // we are no longer capturing
    set_waiting_pin(false);  // we are no longer waiting for trigger
}

// Entrypoint
void stream(uint sample_length, uint averaging, uint bitwidth)
{
    // Prepare board
    setup_board_stream();
    setup_capture_timer(sample_length);

    run_stop = 1;
    main_command = NULL;
    terminal_command_ack(false);
    terminal_command_discard(true);

    prepare_dma();

    // just start
    start_T3();

    set_capturing_pin(true);  // we are capturing

    stream_data(averaging, bitwidth);
    stop_capture();

    terminal_command_discard(false);

    restore_board();
}