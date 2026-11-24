// *** adc.h **********************************************************

enum {
    adc_num_channel = 8
};

void
adc_timer_poll(bool debouncing);

void
adc_sleep(void);

void
adc_initialize(void);

