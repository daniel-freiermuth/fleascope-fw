#define MSPS  18  // approximate target sample rate
#define INTERLEAVE  5  // we interleave results from 5 adcs
#define ADC_CHUNK_SAMPLES  (INTERLEAVE*128)  // samples scope_isr copies into adcbuffer at once
// while rebasable, scope_isr steps current back by this (a multiple of the adcbuffer ring) to
// keep it from wrapping; see scope_same_epoch()
#define CURRENT_REBASE  (INTERLEAVE*2*128*10*65536)

void wave(char *pattern, int hz);
void scope_initialize();
void configure_pmd_once();
void reset_buffers();
uint get_current();
void set_current_rebasable(bool rebasable);
int get_sample_position();
short(* get_bits_buffer())[2*128*10];
short* get_adc_buffer();
void start_T3();
void stop_T3();
void set_capturing_pin(bool state);
void set_waiting_pin(bool state);
int get_bitbuffer_length();
int get_adcbuffer_length();