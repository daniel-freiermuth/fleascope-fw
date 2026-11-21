#define MSPS  18  // approximate target sample rate
#define INTERLEAVE  5  // we interleave results from 5 adcs

// interrupt handler for the adc dma buffers; not called directly
void
scope_isr(void);

void wave(const char *pattern, int hz);
void scope_initialize(void);
void configure_pmd_once(void);
void reset_buffers(void);
uint get_current(void);
short(* get_bits_buffer(void))[2*128*10];
short* get_adc_buffer(void);
void start_T3(void);
void stop_T3(void);
void set_capturing_pin(bool capturing);
void set_waiting_pin(bool waiting);
int get_bitbuffer_length(void);
int get_adcbuffer_length(void);
