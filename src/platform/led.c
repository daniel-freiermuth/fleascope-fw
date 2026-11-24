// *** led.c **********************************************************
// this file implements basic LED controls.

#include "main.h"

enum led {
    led_red = 0,  // dtin3
    led_blue = 0,  // dtin3
    led_max = 1
};

static int led_count[led_max];
static int last_led_count[led_max];

static enum led led_blink;


void
led_unknown_progress(void)
{
    led_count[led_blue]++;
}

// this function turns an LED on or off.
static
void
led_set(enum led n, int on)  // NOLINT(misc-no-recursion): only via a failed assert() -> led_line(), which passes the valid led_red
{
    (void)n;  // only checked by assert()
    assert (n >= 0 && n < led_max);

    pin_set(pin_assignments[pin_assignment_heartbeat], pin_type_digital_output, on);
}

void
led_timer_poll(void)
{
    static int calls;

    calls++;

    // normal led processing
    if (led_count[led_blink] != last_led_count[led_blink]) {
        led_set(led_blink, calls%2);  // blink fast
        last_led_count[led_blink] = led_count[led_blink];
    } else {
        led_set(led_blink, (calls/4)%2);  // blink slow
    }
}


// this function displays a diagnostic code on a LED, forever, one blink group
// per digit of value in the given base, starting from top_divisor.
static
void
led_blink_code(int value, int base, int top_divisor)  // NOLINT(misc-no-recursion): led_set(led_red, ...) passes its assert(), so it does not recurse back
{
    int i;
    int j;
    int inv;

    if (debugger_attached) {
        asm_halt();
    } else {
        splx(7);

        inv = 0;
        for (;;) {
            for (i = top_divisor; i > 0; i /= base) {
                if (value < i) {
                    continue;
                }
                j = (value/i)%base;
                if (! j) {
                    j = base;
                }
                while (j--) {
                    led_set(led_red, 1^inv);
                    delay(200);
                    led_set(led_red, 0^inv);
                    delay(200);
                }
                delay(1000);
            }
            delay(3000);
            inv = ! inv;
            led_set(led_red, 0^inv);
            delay(3000);
        }
    }
}

// this function displays a decimal diagnostic code on a LED.
void
led_line(int line)  // NOLINT(misc-no-recursion): led_set(led_red, ...) passes its assert(), so it does not recurse back
{
    led_blink_code(line, 10, 1000);
}

// this function displays a hexadecimal diagnostic code on a LED.
void
led_hex(int hex)
{
    led_blink_code(hex, 16, 0x10000000);
}


// this function initializes the led module.
void
led_initialize(void)
{
    pin_assign(pin_assignment_heartbeat, pin_assignments[pin_assignment_heartbeat]);
    pin_set(pin_assignments[pin_assignment_heartbeat], pin_type_digital_output, true);
}

