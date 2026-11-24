// *** adc.c **********************************************************
// this file sets up and polls the analog-to-digital converters coupled
// with the a1-a8 pins (AN12-AN19).

#include "main.h"

static bool adc_initialized;

// poll the analog-to-digital converters
void
adc_timer_poll(void)
{
    int i;

    if (! adc_initialized) {
        return;
    }

    // drain the previous scan's results; nothing consumes them, but the SFR reads are kept as-is.
    for (i = 0; i < adc_num_channel; i++) {
        // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound): ADCDATA12..19 are SFRs 16 bytes apart, not one object
        (void)ReadADC10(i);
    }

    // trigger adc scan now
    ADCCON3bits.GSWTRG = 1;
}

void
adc_sleep(void)
{
}

// this function initializes the adc module.
void
adc_initialize(void)
{
    // calibrate shared adc 7
    ADC7CFG = DEVADC7;

    ADCCON3bits.ADCSEL = 3;  // Select ADC input clock source = SYSCLK 120Mhz
    ADCCON3bits.CONCLKDIV = 0;  // Analog-to-Digital Control Clock (TQ) Divider = SYSCLK Divide by 1 (8.33ns)
    ADCCON1bits.FSSCLKEN = 1;  // Fast synchronous SYSCLK to ADC control clock is enabled
    //ADCCON1bits.FSPBCLKEN = 1;  // ???

    ADCANCONbits.WKUPCLKCNT = 0x9;  // ADC Warm up delay = (512 * TADx)

    // configure shared adc
    ADCCON1bits.SELRES = 2;  // 10 bit conversion for shared
    ADCCON2bits.SAMC = 0xff;  // sample time = 257*TAD for shared
    ADCCON2bits.ADCDIV = 1;  // TAD = 2*TQ for shared

    ADCANCONbits.ANEN7 = 1;  // Enable ADC7 analog bias/logic

    // N.B. we do not enable pullups on any pins scanned by ADC because doing so causes other scanned ADCs
    //      to get the wrong values (if the pullup pins are floating) -- I don't understand why!

    // shared adc scans AN12 thru AN19
    ADCTRG4bits.TRGSRC12 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC13 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC14 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC15 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC16 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC17 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC18 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC19 = 1;  // GSWTRG
    ADCCSS1bits.CSS12 = 1;  // a1
    ADCCSS1bits.CSS13 = 1;  // a2
    ADCCSS1bits.CSS14 = 1;  // a3
    ADCCSS1bits.CSS15 = 1;  // a4
    ADCCSS1bits.CSS16 = 1;  // a5
    ADCCSS1bits.CSS17 = 1;  // a6
    ADCCSS1bits.CSS18 = 1;  // a7
    ADCCSS1bits.CSS19 = 1;  // a8

    ADCCON1bits.ON = 1;  // turn on adc

    while(!ADCCON2bits.BGVRRDY);  // Wait until the reference voltage is ready
    while(ADCCON2bits.REFFLT);  // Wait if there is a fault with the reference voltage

    while(!ADCANCONbits.WKRDY7);  // Wait until ADC is ready

    ADCCON3bits.DIGEN7 = 1;      // Enable ADC


    adc_initialized = true;
}

