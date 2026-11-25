// *** adc.c **********************************************************
// this file sets up the adc clock that the dedicated scope adcs run from.

#include "main.h"

void
adc_sleep(void)
{
}

// this function initializes the adc module.
void
adc_initialize(void)
{
    ADCCON3bits.ADCSEL = 3;  // Select ADC input clock source = SYSCLK 120Mhz
    ADCCON3bits.CONCLKDIV = 0;  // Analog-to-Digital Control Clock (TQ) Divider = SYSCLK Divide by 1 (8.33ns)
    ADCCON1bits.FSSCLKEN = 1;  // Fast synchronous SYSCLK to ADC control clock is enabled
    //ADCCON1bits.FSPBCLKEN = 1;  // ???

    ADCANCONbits.WKUPCLKCNT = 0x9;  // ADC Warm up delay = (512 * TADx)

    // configure shared adc
    ADCCON1bits.SELRES = 2;  // 10 bit conversion for shared
    ADCCON2bits.SAMC = 0xff;  // sample time = 257*TAD for shared
    ADCCON2bits.ADCDIV = 1;  // TAD = 2*TQ for shared

    ADCCON1bits.ON = 1;  // turn on adc

    while(!ADCCON2bits.BGVRRDY);  // Wait until the reference voltage is ready
    while(ADCCON2bits.REFFLT);  // Wait if there is a fault with the reference voltage
}

