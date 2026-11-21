// *** adc.h **********************************************************

enum {
    adc_num_channel = 8
};

void
adc_timer_poll(bool debouncing);

void
adc_sleep(void);

int
adc_get_value(int offset, int pin_qual);

void
adc_initialize(void);

