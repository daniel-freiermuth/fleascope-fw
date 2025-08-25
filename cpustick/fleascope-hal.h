#define MSPS  18  // approximate target sample rate
#define INTERLEAVE  5  // we interleave results from 5 adcs

void wave(char *pattern, int hz);
void scope_initialize();
void reset_buffers();
int get_current();
short(* get_bits_buffer())[2*128*10];
short* get_adc_buffer();
void start_T3();
void stop_T3();
void set_capturing_pin(bool state);
void set_waiting_pin(bool state);
int get_bitbuffer_length();
int get_adcbuffer_length();