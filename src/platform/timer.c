// *** timer.c ********************************************************
// this file implements the core interval timer used internally.

#include "main.h"

#if SODEBUG
volatile bool timer_in_isr;
static volatile int32 timer_in_ticks;
static volatile int32 timer_out_ticks;
static volatile int32 timer_max_ticks;
#endif

volatile int32 ticks;  // incremented by pit0 isr every tick
volatile int32 msecs;  // incremented by pit0 isr every millisecond
volatile int32 seconds;  // incremented by pit0 isr every second

static volatile byte msecs_in_second;  // incremented by pit0 isr every millisecond
static volatile byte eighths_in_second;  // incremented by pit0 isr 8 times per second

enum { msecs_per_debounce = 4 }; // tunable

// called by pit0 every tick
void
__ISR(_TIMER_1_VECTOR, PIC32IPL4)
timer_isr(void)
{
    bool debouncing;

    assert(! timer_in_isr);
    debug_only(timer_in_isr = true);
    debug_only(timer_in_ticks = ticks);

    // clear the interrupt flag
    IFS0CLR = _IFS0_T1IF_MASK;

    ticks++;

    // If a msec elapsed...
    if ((ticks & (ticks_per_msec - 1)) == 0) {
        msecs++;
        msecs_in_second++;

        debouncing = (msecs & (msecs_per_debounce - 1)) == 0;

        if (debouncing) {
            // poll debounced digital inputs.
            pin_timer_poll();
        }

        // if an an eighth of a second has elapsed...
        if (msecs_in_second == 125) {
            msecs_in_second = 0;

            eighths_in_second++;

            // if a second has elapsed...
            if ((eighths_in_second&7) == 0) {
                seconds++;
            }

            // poll the LEDs 8 times a second
            led_timer_poll();
        }

    } else {
        debouncing = false;
    }

    // poll the adc to occasionally record debouncing data and to record adc data every tick.
    adc_timer_poll(debouncing);

    assert(timer_in_isr);
    debug_only(timer_in_isr = false);
    debug_only(timer_out_ticks = ticks);
    debug_only(timer_max_ticks = MAX(timer_max_ticks, timer_out_ticks-timer_in_ticks));
}


// this function initializes the timer module.
void
timer_initialize(void)
{
    cassert(IS_POWER_OF_2(ticks_per_msec));
    cassert(IS_POWER_OF_2(msecs_per_debounce));

    // configure t1 to interrupt every ticks times per msec.
    T1CONCLR = _T1CON_ON_MASK;
    T1CON = (1 << _T1CON_TCKPS_POSITION);  // 1:8 prescale
    TMR1 = 0;
    PR1 = bus_frequency/8/1000/ticks_per_msec - 1;
    T1CONSET = _T1CON_ON_MASK;

    // set up the timer interrupt with a priority of 4
    IEC0bits.T1IE = 1;
    IPC1bits.T1IP = 4;
    IPC1bits.T1IS = 0;
}

