// *** pin.c **********************************************************
// this file implements basic I/O pin controls.

#include "main.h"

const char * const pin_assignment_names[] = {
    "heartbeat",
    "safemode*",
    "qspi_cs*",  // zigflea
    "zigflea_rst*",
    "zigflea_attn*",
    "zigflea_rxtxen",
};

byte pin_assignments[pin_assignment_max] = {
    // set our default pin assignments
    PIN_E1, PIN_S1, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED,
};

int pin_last = PIN_LAST;

static byte declared[(PIN_UNASSIGNED+7)/8];

#define DIO  (1<<pin_type_digital_output|1<<pin_type_digital_input)

const struct pin pins[] = {
    {"a0", DIO},
    {"a1", DIO|1<<pin_type_analog_input},
    {"a2", DIO|1<<pin_type_analog_input},
    {"a3", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc5
    {"a4", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc6
    {"a5", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc3
    {"a6", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc4
    {"a7", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc2
    {"a8", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output},  //oc1
    {"b0", DIO},
    {"b1", DIO},
    {"b2", DIO},
    {"b3", DIO},
    {"b4", DIO|1<<pin_type_uart_input},
    {"b5", DIO},
    {"b6", DIO|1<<pin_type_uart_input},
    {"b7", DIO|1<<pin_type_uart_output},
    {"b8", DIO|1<<pin_type_uart_output},
    {"e1", DIO},
    {"e2", DIO},
    {"e3", DIO},
    {"s1", DIO},
    {"none", 0},
};

static
void
pin_declare_internal(IN int pin_number, IN int pin_type, IN bool user)
{
    uint32 offset;

    assert(pin_number != PIN_UNASSIGNED);
    assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
    if (user) {
        declared[pin_number/8] |= 1 << (pin_number%8);
    }

    // N.B. we do not enable pullups on any pins scanned by ADC because doing so causes other scanned ADCs
    //      to get the wrong values (if the pullup pins are floating) -- I don't understand why!

    // configure the PIC32 pin for the requested function
    switch (pin_number) {
        // configure MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = (uint32)(12 + pin_number - PIN_A1);
            assert(offset >= 12 && offset <= 15);
            CNPUECLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELECLR = 1 << offset;
                //CNPUESET = 1 << offset;  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???
                TRISESET = 1 << offset;
            } else {
                // output
                ANSELECLR = 1 << offset;
                TRISECLR = 1 << offset;
            }
            // disable the output compare module driving the pin
            if (pin_number == PIN_A3) {
                OC5CONCLR = _OC5CON_ON_MASK;
            } else if (pin_number == PIN_A4) {
                OC6CONCLR = _OC6CON_ON_MASK;
            }
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = (uint32)(6 + PIN_A8 - pin_number);
            assert(offset >= 6 && offset <= 9);
            CNPUGCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELGCLR = 1 << offset;
                //CNPUGSET = 1 << offset;  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???
                TRISGSET = 1 << offset;
            } else {
                // output
                ANSELGCLR = 1 << offset;
                TRISGCLR = 1 << offset;
            }
            // disable the output compare module driving the pin
            if (pin_number == PIN_A5) {
                OC3CONCLR = _OC3CON_ON_MASK;
            } else if (pin_number == PIN_A6) {
                OC4CONCLR = _OC4CON_ON_MASK;
            } else if (pin_number == PIN_A7) {
                OC2CONCLR = _OC2CON_ON_MASK;
            } else if (pin_number == PIN_A8) {
                OC1CONCLR = _OC1CON_ON_MASK;
            }
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = (uint32)(4 + pin_number - PIN_B0);
            ASSERT(offset >= 4 && offset <= 6);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                // output
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            offset = (uint32)(10 + pin_number - PIN_B3);
            assert(offset >= 10 && offset <= 15);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                // output
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            CNPUCCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELCCLR = 1 << offset;
                CNPUCSET = 1 << offset;
                TRISCSET = 1 << offset;
            } else {
                // output
                ANSELCCLR = 1 << offset;
                TRISCCLR = 1 << offset;
            }
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = (uint32)(4 + (pin_number - PIN_E2)*6);
            assert(offset == 4 || offset == 10);
            CNPUACLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELACLR = 1 << offset;
                CNPUASET = 1 << offset;
                TRISASET = 1 << offset;
            } else {
                // output
                ANSELACLR = 1 << offset;
                TRISACLR = 1 << offset;
            }
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            CNPUCCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELCCLR = 1 << offset;
                CNPUCSET = 1 << offset;
                TRISCSET = 1 << offset;
            } else {
                // output
                ANSELCCLR = 1 << offset;
                TRISCCLR = 1 << offset;
            }
            break;
        default:
            assert(0);
            break;
    }
}

// this function declares a ram, flash, or pin variable!
void
pin_declare(IN int pin_number, IN int pin_type)
{
    pin_declare_internal(pin_number, pin_type, true);
}

// this function sets a pin variable!
void
pin_set(IN int pin_number, IN int pin_type, IN int32 value)
{
    uint32 offset;

    if (pin_number == PIN_UNASSIGNED) {
        return;
    }

    (void)pin_type;  // only checked by assert()
    assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);

    // set the PIC32 pin to value
    switch (pin_number) {
        // set MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            if (value) {
                LATBSET = 1 << offset;
            } else {
                LATBCLR = 1 << offset;
            }
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = (uint32)(12 + pin_number - PIN_A1);
            assert(offset >= 12 && offset <= 15);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATESET = 1 << offset;
            } else {
                LATECLR = 1 << offset;
            }
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = (uint32)(6 + PIN_A8 - pin_number);
            assert(offset >= 6 && offset <= 9);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATGSET = 1 << offset;
            } else {
                LATGCLR = 1 << offset;
            }
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = (uint32)(4 + pin_number - PIN_B0);
            ASSERT(offset >= 4 && offset <= 6);
            if (value) {
                LATBSET = 1 << offset;
            } else {
                LATBCLR = 1 << offset;
            }
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            offset = (uint32)(10 + pin_number - PIN_B3);
            assert(offset >= 10 && offset <= 15);
            if (value) {
                LATBSET = 1 << offset;
            } else {
                LATBCLR = 1 << offset;
            }
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            if (value) {
                LATCSET = 1 << offset;
            } else {
                LATCCLR = 1 << offset;
            }
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = (uint32)(4 + (pin_number - PIN_E2)*6);
            assert(offset == 4 || offset == 10);
            if (value) {
                LATASET = 1 << offset;
            } else {
                LATACLR = 1 << offset;
            }
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            if (value) {
                LATCSET = 1 << offset;
            } else {
                LATCCLR = 1 << offset;
            }
            break;
        default:
            assert(0);
            break;
    }
}

// this function gets a pin variable!
int32
pin_get(IN int pin_number, IN int pin_type)
{
    int32 value;
    uint32 offset;

    value = 0;

    if (pin_number == PIN_UNASSIGNED) {
        return value;
    }

    (void)pin_type;  // only checked by assert()
    assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);

    // get the value of the PIC32 pin
    switch (pin_number) {
        // get MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            value = !! (PORTB & 1 << offset);
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = (uint32)(12 + pin_number - PIN_A1);
            assert(offset >= 12 && offset <= 15);
            value = !! (PORTE & 1 << offset);
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = (uint32)(6 + PIN_A8 - pin_number);
            assert(offset >= 6 && offset <= 9);
            value = !! (PORTG & 1 << offset);
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = (uint32)(4 + pin_number - PIN_B0);
            ASSERT(offset >= 4 && offset <= 6);
            value = !! (PORTB & 1 << offset);
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            offset = (uint32)(10 + pin_number - PIN_B3);
            assert(offset >= 10 && offset <= 15);
            value = !! (PORTB & 1 << offset);
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            value = !! (PORTC & 1 << offset);
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = (uint32)(4 + (pin_number - PIN_E2)*6);
            assert(offset == 4 || offset == 10);
            value = !! (PORTA & 1 << offset);
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            value = !! (PORTC & 1 << offset);
            break;
        default:
            assert(0);
            break;
    }

    return value;
}


void
pin_clear(void)
{
    int i;

    // the pin mapping the removed I2C and QSPI drivers restored here; kept so
    // boot leaves the pins as before (revisit on hardware)
    I2C1CONbits.ON = false;
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    SPI2CON = 0;
    RPB4R = 4;  // SDO2 -> RB4
    SDI2R = 1;  // RB5 -> SDI2
    SS2R = 3;  // RB10 -> SS2

    // reset all explicitly declared pins to digital input
    for (i = 0; i < PIN_UNASSIGNED; i++) {
        if ((declared[i/8] & (1 << (i%8))) && (pins[i].pin_type_mask & (1 << pin_type_digital_input))) {
            pin_declare_internal(i, pin_type_digital_input, false);
        }
    }
}

void
pin_assign(int assign, int pin)
{
    assert(assign < pin_assignment_max);
    assert(pin < PIN_LAST);

// SODEBUG: GCC warns about the assert-failed path, where led_line() returns to a debugger
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    pin_assignments[assign] = (byte)pin;
#pragma GCC diagnostic pop

    if (pin < PIN_UNASSIGNED) {
        if (assign == pin_assignment_safemode) {
            pin_declare_internal(pin, pin_type_digital_input, false);
        } else {
            pin_declare_internal(pin, pin_type_digital_output, false);
        }
    }
}

// route OC1-OC6 to their output pins
void
pin_oc_route(void)
{
    RPE14R = 6;  // OC5 -> RE14
    RPE15R = 6;  // OC6 -> RE15
    RPG9R = 5;  // OC3 -> RG9
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    RPG6R = 5;  // OC1 -> RG6
}

// disconnect OC1-OC6 from their output pins
void
pin_oc_unroute(void)
{
    RPE14R = 0;  // undo OC5 -> RE14
    RPE15R = 0;  // undo OC6 -> RE15
    RPG9R = 0;  // undo OC3 -> RG9
    RPG8R = 0;  // undo OC4 -> RG8
    RPG7R = 0;  // undo OC2 -> RG7
    RPG6R = 0;  // undo OC1 -> RG6
}

extern void
pin_initialize(void)
{
    int i;
    int32 pin;

    cassert(PIN_LAST <= PIN_MAX);
    cassert(pin_type_last < (sizeof(uint16)*8));
    cassert(LENGTHOF(pins) == PIN_LAST);

    // reset declared pins and the boot pin mapping
    pin_clear();
    
    // N.B. we do not enable pullups on any pins scanned by ADC because doing so causes other scanned ADCs
    //      to get the wrong values (if the pullup pins are floating) -- I don't understand why!

    // N.B. ports ABC setup in scope.c; ports EG setup in pin.c
    TRISE = 0xffff;
    ANSELE = 0x0000;
    //CNPUE = ~ANSELE;  // keep inputs at low power  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???

    TRISG = 0xffff;
    ANSELG = 0x0000;
    //CNPUG = ~ANSELG;  // keep inputs at low power  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???

    pin_oc_route();

    // load up our pin assignments
    for (i = 0; i < pin_assignment_max; i++) {
        pin = params_get(PARAM_ASSIGNMENTS_BEGIN+i);
        if (pin >= 0 && pin < PIN_LAST) {
            pin_assignments[i] = (byte)pin;
        }
        pin_assign(i, pin_assignments[i]);
    }
}

